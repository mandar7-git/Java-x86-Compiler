#include <bits/stdc++.h>
#include "attr.h"

using namespace std;
vector<string> freeHeap;
vector<vector<string>> ForInitVars; // ForInitVars[n-1] stores the popping 3ACs of the vars declared in the current "ForStatement"
map<string,string> funcParamTemp;//maps function parameter list to their corresponding temp
// ClassBody_NodeNumber->Class Name
map<string,int> initVal;
map<int,string>  classNameMap;
string insideClassName;
map<string,string> typeOfNode; // to_string(nodeNum) -> Type of that node
// map<int, pair<string,vector<int>>> arrayInfo;
map<int,int> trueLabel;
map<int,int> falseLabel;
map<int,int> nextLabel;
map<string,string> varToTemp;
int countNodes=0;
int tempNum=0;
int inStatement=0;
int inMethodInvocation=0,inMN=0;
map<string,int> typeSize;
int labelNum=0;
vector<string> nodeType;
map<int,int> lineNum;
map<int, vector<int>> adj;      
map<int,int> prodNum;
string switchExpAddr;
string someExpAddr;
int isWhile=0;string whileContinueLabel,whileBreakLabel;
int isFor=0;string forContinueLabel,forBreakLabel;
int isDo=0;string doContinueLabel,doBreakLabel;
map<string, string> functionParameterMap;
map<pair<int,int>, pair<int,int>> mapParentScope;
vector<string> integerTypes = {"double", "float", "long", "int", "byte", "short"};

typedef struct localtableparams{
	string name="";
	string type="";
    vector<int> arraySize;
    int line;
	pair<int,int> scope;
	pair<int,int> parentScope;
	int offset;
	vector<localtableparams>* functionTablePointer;
	vector<string> functionParams;
	string functionReturnType;
    int useOffset;
} localTableParams ;

typedef struct globaltableparams{
	string name="";
	string type="";
    int classNum;
    int parentClassNum=0;
    int line;
	vector<localTableParams>* localTablePointer; 
} globalTableParams;



vector<globalTableParams> globalTable;
vector<localTableParams>* currSymTab;
map< int, pair< pair<int,int>,vector<localTableParams>* > > scopeAndTable;

int classCount=0;
int funcInClass=0;
vector<int> scopeHelper;
// scopeHelper[0] is the number of 4 level stuffs
// scopeHelper[1] is the number of 5 level stuffs ...

vector<attr1> attrSymTab;
vector<attr> attr3AC;

stack<pair<int,int>> currScope;
stack<pair<int,int>> parentScope;
map <string,bool> isFinalField;
bool isInAssignment=false;
bool inFormalParameterList=false;
bool inForInit=false;
map<vector<localTableParams>*,vector<localTableParams>*> parentTable;
string getArg (string t){

    string arg1;

            if (t[0]=='t'){
                int of1 = -8*stoi(t.substr(1))-8;
                arg1 = to_string(of1);
                arg1+= "(%rbp)";
            }
            // else if t[0] is integer character
            else if (t[0] >= '0' && t[0] <= '9'){
                arg1 = "$"+t;
            }
            else{
                t=varToTemp[t];
                int of1 = -8*stoi(t.substr(1))-8;
                arg1 = to_string(of1);
                arg1+= "(%rbp)";
            }
    return arg1 ;
}


string addFuncParamsToReg(string p, string reg, int c3, int fcall){
    if(p[0]>='0' && p[0]<='9'){
        string temp = "movq $" + p + ", " + reg;
        return temp;
    }
    if(p[0]=='t'){
        int pOffset = -8*(stoi(p.substr(1)))-8;
        string temp = "movq " + to_string(pOffset) + "(%rbp), " + reg;
        return temp;
    }else{
        p = varToTemp[attr3AC[c3].params[fcall]];
        int pOffset = -8*(stoi(p.substr(1)))-8;
        string temp = "movq " + to_string(pOffset) + "(%rbp), " + reg;
        return temp;
    }
}

string getFuncRet(int calleNodeNum, string funcName, string className){
    for(int i=0;i<globalTable.size();i++){
        if(globalTable[i].type=="class" && globalTable[i].name==className){
            vector<localTableParams> *classtab = globalTable[i].localTablePointer;
            for(int j=0;j<(*classtab).size();j++){
                if((*classtab)[j].type=="method" && (*classtab)[j].name==funcName){
                    if(lineNum[calleNodeNum]>=(*classtab)[j].line){
                        // cout << "idhar  " << (*classtab)[j].functionReturnType << " " << funcName << " " << className << endl;
                        return (*classtab)[j].functionReturnType;
                    }
                }
            }
        }
    }
    cout<<"[Compilation Error]: Function not declared but called on line "<<lineNum[calleNodeNum]<<"\nFunction '"<<  funcName << "' in class '"<< className << "' !\nAborting...\n";
    // exit(0);
    return "";
}

string removeLastChar(string _type){
    string ret="";
    for (int i=0;i<_type.size()-1;i++)
        ret+=_type[i];
    
    return ret;
}
string getArrayType(string _type){
    
    string lType_rev;
    for (int i=_type.length()-1; ;i--){

        if (_type[i]!=')'){
            lType_rev="";
            while(_type[i]!='('){
                lType_rev+=_type[i];
                i--;
            }
            break;
        }

    }

    string lType="";

    for (int i=lType_rev.length()-1;i>=0;i--)
        lType+=lType_rev[i];
    
    return lType;
}


void fillFunctionOffsets(vector<localTableParams>* _funcTablePtr){
    int funcOffset=0;
    int m4Offset=0;
    for(auto &funcRow: *_funcTablePtr){

            funcRow.offset=funcOffset;
            funcRow.useOffset = m4Offset;

            if (funcRow.arraySize.size()==0)
                funcOffset+=typeSize[funcRow.type];
            else{
                //it's an array
                int jump=1;
                for (int i=0;i<funcRow.arraySize.size();i++)
                    jump=jump*funcRow.arraySize[i];
                jump*=typeSize[getArrayType(funcRow.type)];
                funcOffset+=jump;
            }

            m4Offset+=8;
    } 
}

void fillClassOffsets(vector<localTableParams>* _classTablePtr){
    int classOffset=0;
    int m4Offset=0;
    for(auto &classRow: *_classTablePtr){

        if (classRow.type=="method"){
            fillFunctionOffsets(classRow.functionTablePointer);
        }
        else {
            classRow.offset=classOffset;
            classRow.useOffset= m4Offset;

            if (classRow.arraySize.size()==0){
                classOffset+=typeSize[classRow.type];
                m4Offset+=8;
            }
            else{
                //it's an array
                int jump=1;
                int m4jump;
                for (int i=0;i<classRow.arraySize.size();i++){
                    jump=jump*classRow.arraySize[i];
                }
                m4jump=jump*8;
                jump*=typeSize[getArrayType(classRow.type)];
                classOffset+=jump;
                m4Offset+=m4jump;

            }

        }
    }
}

void fillOffsets(){

    for (auto & globRow:globalTable){
        if (globRow.type=="class"){
            fillClassOffsets(globRow.localTablePointer);
        }
    }

}

int getOffset(int _nodeNum){
    
    vector<localTableParams>* funTabPtr = scopeAndTable[_nodeNum].second ;
    vector<localTableParams>* classTabPtr = parentTable[funTabPtr];
    string varName=nodeType[_nodeNum];

    for (auto cRow: *classTabPtr){

        if (cRow.name==varName && cRow.type=="method"){
            return -1;
        }else if(cRow.name==varName){
            return cRow.offset;
        }
    }

    for (auto fRow: *funTabPtr){

        if (fRow.name==varName){
            return fRow.offset;
        }
    }

    return 0;
}

int getClassSize(string class_name){
    vector<localTableParams> classTab;
    for (auto &globRow:globalTable){

        if (globRow.type=="class" && globRow.name==class_name){
            classTab = *(globRow.localTablePointer);
        }

    }
    int count=0;
    for (auto &classRow:classTab){
        if (classRow.type!="method"){
            count++;
        }
    }
    return 8*count;
}

int useOffset(int _nodeNum){

    vector<localTableParams>* funTabPtr = scopeAndTable[_nodeNum].second ;
    vector<localTableParams>* classTabPtr = parentTable[funTabPtr];
    string varName=nodeType[_nodeNum];

    for (auto cRow: *classTabPtr){

        if (cRow.name==varName){
            return cRow.useOffset;
        }
    }

    for (auto fRow: *funTabPtr){

        if (fRow.name==varName){
            return fRow.useOffset;
        }
    }

    return 0;

}

localTableParams* checkInScope(string _varName, pair<int,int> _scope, vector<localTableParams>* _tablePointer){
    // cout<<"Hi\n";
    for (auto &locRow:*(_tablePointer)){
        // cout<<locRow.name<<_varName<<" "<<locRow.scope.first<<_scope.first<<" "<<locRow.scope.second<<_scope.second<<endl;
        if (locRow.name==_varName && locRow.scope.first==_scope.first && locRow.scope.second==_scope.second)
            return &locRow;
    }

    return NULL;
}

string getType(string _varName, int _nodeNum){

    //return type

    vector<localTableParams>* primaryTable=scopeAndTable[_nodeNum].second;

    pair<int,int> startScope=scopeAndTable[_nodeNum].first;

    // cout<<startScope.first<<" "<<startScope.second<<endl;

    // check in current table and it's parent table (class table's parent is mapped to itself so no worries)
    
    while(startScope.first>1){

        auto rowPtr=checkInScope(_varName,startScope,primaryTable);

        if (rowPtr!=NULL){

            // got the row

            return rowPtr->type;

        }

        startScope=mapParentScope[startScope];
    }

    startScope=scopeAndTable[_nodeNum].first;

    while(startScope.first>1){

        auto rowPtr=checkInScope(_varName,startScope,(parentTable[primaryTable]));

        if (rowPtr!=NULL){

            // got the row

            return rowPtr->type;
        }

        startScope=mapParentScope[startScope];
    }

    return "long";   

}

string fillHelper(string _nodeNum){
        
        int node_number=atoi(_nodeNum.c_str());
        // cout<<"from FillHeper: "<<nodeType[node_number]<<endl;

        if (find(integerTypes.begin(),integerTypes.end(),typeOfNode[_nodeNum])!=integerTypes.end() || typeOfNode[_nodeNum]=="char" || typeOfNode[_nodeNum]=="String" || typeOfNode[_nodeNum]=="true" || typeOfNode[_nodeNum]=="false")
            return typeOfNode[_nodeNum];
        
        string ret;
        ret=getType(nodeType[node_number],node_number);
        // cout<<ret<<endl;
        if (ret[ret.size()-1]==')'){
            string og_type=ret;
            ret=getArrayType(ret);
            int i=og_type.size()-1;
            int dims=0;
            while(og_type[i]==')'){
                dims++ ;
                i--;
            }
            ret=ret+to_string(dims)+";";
        }
        
        typeOfNode[_nodeNum]=ret;
        return ret;
}

void filltypeOfNode(){

    for (auto &elem:typeOfNode){
        // cout << "filltypeofnode " << elem.first << " " << elem.second << endl;
        
        if (elem.second=="fillMe"){

            elem.second=fillHelper(elem.first);
        }
    }
}

void printfilltypeOfNode(){
    for (auto elem:typeOfNode)
        cout<<nodeType[stoi(elem.first)]<<" "<<elem.first<<"->"<<elem.second<<endl;
}

void fillClassSizes(){

    for (auto &globRow: globalTable){

        if (globRow.type=="class"){
            vector<localTableParams>* classTabPtr = globRow.localTablePointer ;
            int classSize=0;

            for (auto &cRow: *classTabPtr){

                if (cRow.scope.first ==2){

                    if (cRow.arraySize.size()==0){
                        // normal var
                        classSize+=typeSize[cRow.type];

                    }
                    else{
                        //array
                        int size=1;

                        for (int i=0;i<cRow.arraySize.size();i++){
                            size*=cRow.arraySize[i];
                        }
                        classSize+=(size*typeSize[getArrayType(cRow.type)]);
                    }
                }
            }

            typeSize[globRow.name]=classSize;
            // cout<<globRow.name<<" "<<classSize<<endl;
        }
    }
}

void storeParseTree(int flag){

    if(!flag)freopen("output.dot","w",stdout);
	cout << "// dot -Tps output.dot -o out.ps\n\n"
		<< "graph \"Tree\"\n"
		<< "{\n"
		<< "\tfontname=\"Helvetica,Arial,sans-serif\"\n"
    	 << "\tnode [fontsize=10, width=\".2\", height=\".2\", margin=0]\n"
		 << "\tedge [fontsize=6]\n"
    	 << "\tgraph[fontsize=8];\n\n"
    	 << "\tlabel=\"Abstract Syntax Tree\"\n\n";

	for(int i=0;i<nodeType.size();i++){
		cout << "\tn" << i << ";\n";
		cout << "\tn" << i << "[label=\"" ;
		for(int t=0;t<nodeType[i].length();t++){
			if(nodeType[i][t]=='"'){
				if(t>0){
					if(nodeType[i][t]!='\\'){
						cout << "\\" << nodeType[i][t];
					}else cout << nodeType[i][t];
				}else{
					cout << "\\" << nodeType[i][t];
				}
			}else cout << nodeType[i][t];
		}
		cout <<"\"];\n";
		auto child = adj[i];
		for(int j=0;j<child.size();j++){
			cout << "\tn" << i << "--" << 'n' << child[j] << ";\n";
		}
		cout << endl;
	}
	cout << "}" << endl;
	
}

string getLabel(int nodeNum, int type){
    switch(type){
        case 1: //True label
        {
            if(trueLabel.find(nodeNum)!=trueLabel.end()){
                string r = "L" + to_string(trueLabel[nodeNum]);
                return r;
            }else{
                labelNum++;
                string r = "L" + to_string(labelNum);
                trueLabel[nodeNum]=labelNum;
                return r;
            }
        }
        break;
        case 2: //False label
        {
            if(falseLabel.find(nodeNum)!=falseLabel.end()){
                string r = "L" + to_string(falseLabel[nodeNum]);
                return r;
            }else{
                labelNum++;
                string r = "L" + to_string(labelNum);
                falseLabel[nodeNum]=labelNum;
                return r;
            }
        }
        break;
        case 3: //Next label
        {
            if(nextLabel.find(nodeNum)!=nextLabel.end()){
                string r = "L" + to_string(nextLabel[nodeNum]);
                return r;
            }else{
                labelNum++;
                string r = "L" + to_string(labelNum);
                nextLabel[nodeNum]=labelNum;
                return r;
            }
        }
        break;
        default:
        {
            labelNum++;
            string r = "L" + to_string(labelNum);
            return r;
        }
    }
}

int getStackOffset(string t1){
    return -8*stoi(t1.substr(1, t1.size()-1))-8;
}

vector<string> getAddAssemblyCode(string t1, string t2, string t3){
    vector<string> ret;//###t3 assumed to be temporary always
    //t1=t2+t3
    
    //get address of t2 and t3 using their offsets from stack pointer
    //load t2 and t3 in registers r8, r9
    //add them using addq. stores result in r9
    //store r9 contents to stack location of t1 (get offset)

    // cout<<"inside hereen\n";
    // cout<<t1.size()<<"\n";
    // cout<<t1.substr(1, t1.size()-1);
    // cout<<t1<<" "<<t2<<" "<<t3<<"\n";
    
    // cout<<"calc val :"<<8*stoi(t1.substr(1, t1.size()-1));
    // cout << "inside the add expresion " << t1 << " " << t2 << " " << t3 << endl;
    
    int of1, of2, of3;
    if(t1[0]=='t') of1 = -8*stoi(t1.substr(1, t1.size()-1))-8;
    if(t2[0]=='t') of2 = -8*stoi(t2.substr(1, t2.size()-1))-8;
    if(t3[0]=='t') of3 = -8*stoi(t3.substr(1, t3.size()-1))-8;

    // cout<<"after offset : "<<of1<<"\n";

    if(t2[0]=='t') ret.push_back("movq " + to_string(of2) + "(%rbp), %r8");
    else ret.push_back("movq $"+(t2)+", %r8");  //if t2 is a constant

    if(t3[0]=='t') ret.push_back("movq " + to_string(of3) + "(%rbp), %r9");
    else ret.push_back("movq $"+(t3)+", %r9");

    ret.push_back("addq %r9, %r8");

    ret.push_back("movq %r8, " + to_string(of1) + "(%rbp)");  

    // cout<<ret.size()<<endl;
    return ret;
}

vector<string> getSubAssemblyCode(string t1 , string t2, string t3){
    vector<string> ret;
    //t1=t2-t3
    // cout<<"inside hereen\n";
    // cout << "inside here " << t1 << " " << t2 << " " << t3 << endl;
    int of1, of2, of3;
    if(t1[0]=='t') of1 = -8*stoi(t1.substr(1, t1.size()-1))-8;
    if(t2[0]=='t') of2 = -8*stoi(t2.substr(1, t2.size()-1))-8;
    if(t3[0]=='t') of3 = -8*stoi(t3.substr(1, t3.size()-1))-8;

    if(t2[0]=='t') ret.push_back("movq " + to_string(of2) + "(%rbp), %r8");
    else ret.push_back("movq $"+(t2)+", %r8");  //if t2 is a constant

    if(t3[0]=='t') ret.push_back("movq " + to_string(of3) + "(%rbp), %r9");
    else ret.push_back("movq $"+(t3)+", %r9");

    ret.push_back("subq %r9, %r8");

    ret.push_back("movq %r8, " + to_string(of1) + "(%rbp)");  

    return ret;
}

vector<string> getMulAssemblyCode(string t1, string t2, string t3){
    vector<string> ret;
    //t1=t2*t3

    // cout<<"inside hereen\n";

    int of1, of2, of3;
    if(t1[0]=='t') of1 = -8*stoi(t1.substr(1, t1.size()-1))-8;
    if(t2[0]=='t') of2 = -8*stoi(t2.substr(1, t2.size()-1))-8;
    if(t3[0]=='t') of3 = -8*stoi(t3.substr(1, t3.size()-1))-8;

    if(t2[0]=='t') ret.push_back("movq " + to_string(of2) + "(%rbp), %r8");
    else ret.push_back("movq $"+(t2)+", %r8");  //if t2 is a constant

    if(t3[0]=='t') ret.push_back("movq " + to_string(of3) + "(%rbp), %r9");
    else ret.push_back("movq $"+(t3)+", %r9");

    ret.push_back("imulq %r9, %r8");

    ret.push_back("movq %r8, " + to_string(of1) + "(%rbp)");  

    // cout<<"returnedddd";
    return ret;
}

vector<string> getDivAssemblyCode(string t1, string t2, string t3){
    vector<string> ret;
    //t1=t2/t3
    int of1, of2, of3;
    if(t1[0]=='t') of1 = -8*stoi(t1.substr(1, t1.size()-1))-8;
    if(t2[0]=='t') of2 = -8*stoi(t2.substr(1, t2.size()-1))-8;
    if(t3[0]=='t') of3 = -8*stoi(t3.substr(1, t3.size()-1))-8;

    if(t2[0]=='t') ret.push_back("movq " + to_string(of2) + "(%rbp), %rax");
    else ret.push_back("movq $"+(t2)+", %rax");  //if t2 is a constant

    if(t3[0]=='t') ret.push_back("movq " + to_string(of3) + "(%rbp), %r8");
    else ret.push_back("movq $"+(t3)+", %r8");

    //check divide by zero exception
    ret.push_back("movq %r8, %r10");
    ret.push_back("xor %r9, %r9");
    ret.push_back("cmp %r8, %r9");
    ret.push_back("je divide_by_zero");
    //Sign extend rax to rdx:rax by cqto
    ret.push_back("cqto");
    ret.push_back("idivq %r8");

    ret.push_back("movq %rax, " + to_string(of1) + "(%rbp)");  

    return ret;
}

vector<string> getPerAssemblyCode(string t1, string t2, string t3){
    vector<string> ret;
    //t1=t2/t3
    int of1, of2, of3;
    if(t1[0]=='t') of1 = -8*stoi(t1.substr(1, t1.size()-1))-8;
    if(t2[0]=='t') of2 = -8*stoi(t2.substr(1, t2.size()-1))-8;
    if(t3[0]=='t') of3 = -8*stoi(t3.substr(1, t3.size()-1))-8;

    if(t2[0]=='t') ret.push_back("movq " + to_string(of2) + "(%rbp), %rax");
    else ret.push_back("movq $"+(t2)+", %rax");  //if t2 is a constant

    if(t3[0]=='t') ret.push_back("movq " + to_string(of3) + "(%rbp), %r8");
    else ret.push_back("movq $"+(t3)+", %r8");

    ret.push_back("xorq %rdx, %rdx");

    //Sign extend rax to rdx:rax by cqto
    ret.push_back("cqto");
    ret.push_back("idivq %r8");

    ret.push_back("movq %rdx, " + to_string(of1) + "(%rbp)");  

    return ret;
}

vector<string> getRightShiftAssemblyCode(string t1, string t2, string t3){
    vector<string> ret;
    //t1=t2/t3
    int of1, of2, of3;
    if(t1[0]=='t') of1 = -8*stoi(t1.substr(1, t1.size()-1))-8;
    if(t2[0]=='t') of2 = -8*stoi(t2.substr(1, t2.size()-1))-8;
    if(t3[0]=='t') of3 = -8*stoi(t3.substr(1, t3.size()-1))-8;

    if(t2[0]=='t') ret.push_back("movq " + to_string(of2) + "(%rbp), %r8");
    else ret.push_back("movq $"+(t2)+", %r8");  //if t2 is a constant

    if(t3[0]=='t') ret.push_back("movq " + to_string(of3) + "(%rbp), %r9");
    else ret.push_back("movq $"+(t3)+", %r9");

    ret.push_back("sarq %r8, %r9");

    ret.push_back("movq %r8, " + to_string(of1) + "(%rbp)");  

    return ret;
}

vector<string> getLeftShiftAssemblyCode(string t1, string t2, string t3){
    vector<string> ret;
    //t1=t2/t3
    int of1, of2, of3;
    if(t1[0]=='t') of1 = -8*stoi(t1.substr(1, t1.size()-1))-8;
    if(t2[0]=='t') of2 = -8*stoi(t2.substr(1, t2.size()-1))-8;
    if(t3[0]=='t') of3 = -8*stoi(t3.substr(1, t3.size()-1))-8;

    if(t2[0]=='t') ret.push_back("movq " + to_string(of2) + "(%rbp), %r8");
    else ret.push_back("movq $"+(t2)+", %r8");  //if t2 is a constant

    if(t3[0]=='t') ret.push_back("movq " + to_string(of3) + "(%rbp), %r9");
    else ret.push_back("movq $"+(t3)+", %r9");

    ret.push_back("salq %r8, %r9");

    ret.push_back("movq %r8, " + to_string(of1) + "(%rbp)");  

    return ret;
}

vector<string> getUnsignedRightShiftAssemblyCode(string t1, string t2, string t3){
    vector<string> ret;
    //t1=t2/t3
    int of1, of2, of3;
    if(t1[0]=='t') of1 = -8*stoi(t1.substr(1, t1.size()-1))-8;
    if(t2[0]=='t') of2 = -8*stoi(t2.substr(1, t2.size()-1))-8;
    if(t3[0]=='t') of3 = -8*stoi(t3.substr(1, t3.size()-1))-8;

    if(t2[0]=='t') ret.push_back("movq " + to_string(of2) + "(%rbp), %r8");
    else ret.push_back("movq $"+(t2)+", %r8");  //if t2 is a constant

    if(t3[0]=='t') ret.push_back("movq " + to_string(of3) + "(%rbp), %r9");
    else ret.push_back("movq $"+(t3)+", %r9");

    ret.push_back("shrq %r8, %r9");

    ret.push_back("movq %r8, " + to_string(of1) + "(%rbp)");  

    return ret;
}

vector<string> getMinusUnaryExpressionAssemblyCode(string t1){
    vector<string> ret;
    //t1=-t2
    // cout<<"t1 "<<t1<<" t2 "<<t2<<endl;

    int of1;
    if(t1[0]=='t') of1 = -8*stoi(t1.substr(1, t1.size()-1))-8;

    ret.push_back("movq " + to_string(of1) + "(%rbp), %r8");

    ret.push_back("negq %r8");
    ret.push_back("movq %r8, " + to_string(of1) + "(%rbp)"); 

    return ret;
}

vector<string> getPreandPostIncrementAssemblyCode(string t1){
    vector<string> ret;
    //t1=++t2

    int of1;
    of1 = -8*stoi(t1.substr(1, t1.size()-1))-8;

    ret.push_back("movq " + to_string(of1) + "(%rbp), %r8"); 

    ret.push_back("incq %r8");
    ret.push_back("movq %r8, " + to_string(of1) + "(%rbp)"); 
    // ret.push_back("movq %r8, " + to_string(of1) + "(%rbp)"); 

    return ret;
}

vector<string> getPreandPostDecrementAssemblyCode(string t1){
    vector<string> ret;
    //t1=++t2
    // cout<<"t1 "<<t1<<" t2 "<<t2<<endl;
    int of1;
    of1 = -8*stoi(t1.substr(1, t1.size()-1))-8;

    ret.push_back("movq " + to_string(of1) + "(%rbp), %r8"); 

    ret.push_back("decq %r8");
    ret.push_back("movq %r8, " + to_string(of1) + "(%rbp)"); 

    return ret;
}

int getLabelNumber(string s){
    string r = "";
    for(int i=1;i<s.length();i++)r+=s[i];
    return stoi(r);
}

string getTypeNode(int c){
    // cout << "GET TYPE " << attr3AC[c].nodeno << " " << typeOfNode[to_string(attr3AC[c].nodeno)] << endl;
    string tp;
    if(typeOfNode.find(to_string(attr3AC[c].nodeno))!=typeOfNode.end()){
        tp = typeOfNode[to_string(attr3AC[c].nodeno)];
    }
    if(attr3AC[c].addrName.size()>0 && typeOfNode.find(attr3AC[c].addrName)!=typeOfNode.end()){
        tp = typeOfNode[attr3AC[c].addrName];
    }
    return tp;
}

void widenNode(int a , int b){
    string p = getTypeNode(a);
    string q = getTypeNode(b);
    // cout << "choda karna he " << p << " " << q << endl;
    if(p=="int" && q=="float"){
        tempNum++;
        string t = "t" + to_string(tempNum);
        string temp = t + " = " + "cast_to_float " + attr3AC[a].addrName;
        attr3AC[a].threeAC.push_back(temp);
        attr3AC[a].addrName = t;
        attr3AC[a].type = "float";
        typeOfNode[t] = "float";
        //call table update
    }else if(p=="int" && q=="double"){
        tempNum++;
        string t = "t" + to_string(tempNum);
        string temp = t + " = " + "cast_to_double " + attr3AC[a].addrName;
        attr3AC[a].threeAC.push_back(temp);
        attr3AC[a].addrName = t;
        attr3AC[a].type = "double";
        typeOfNode[t] = "double";
        //call table update
    }else if(p=="float" && q=="double"){
        tempNum++;
        string t = "t" + to_string(tempNum);
        string temp = t + " = " + "cast_to_double " + attr3AC[a].addrName;
        attr3AC[a].threeAC.push_back(temp);
        attr3AC[a].addrName = t;
        attr3AC[a].type = "double";
        typeOfNode[t] = "double";
        //call table update
    }else if(p=="float" && q=="int"){
        tempNum++;
        string t = "t" + to_string(tempNum);
        string temp = t + " = " + "cast_to_float " + attr3AC[b].addrName;
        attr3AC[b].threeAC.push_back(temp);
        attr3AC[b].addrName = t;
        attr3AC[b].type = "float";
        typeOfNode[t] = "float";
        //call table update
    }else if(p=="double" && q=="float"){
        tempNum++;
        string t = "t" + to_string(tempNum);
        string temp = t + " = " + "cast_to_double " + attr3AC[b].addrName;
        attr3AC[b].threeAC.push_back(temp);
        attr3AC[b].addrName = t;
        attr3AC[b].type = "double";
        typeOfNode[t] = "double";
        //call table update
    }else if(p=="double" && q=="int"){
        tempNum++;
        string t = "t" + to_string(tempNum);
        string temp = t + " = " + "cast_to_double " + attr3AC[b].addrName;
        attr3AC[b].threeAC.push_back(temp);
        attr3AC[b].addrName = t;
        attr3AC[b].type = "double";
        typeOfNode[t] = "double";
        //call table update
    }else if(p=="int" && q=="long"){
        tempNum++;
        string t = "t" + to_string(tempNum);
        string temp = t + " = " + "cast_to_long " + attr3AC[a].addrName;
        attr3AC[a].threeAC.push_back(temp);
        attr3AC[a].addrName = t;
        attr3AC[a].type = "long";
        typeOfNode[t] = "long";
        //call table update
    }else if(p=="long" && q=="float"){
        tempNum++;
        string t = "t" + to_string(tempNum);
        string temp = t + " = " + "cast_to_float " + attr3AC[a].addrName;
        attr3AC[a].threeAC.push_back(temp);
        attr3AC[a].addrName = t;
        attr3AC[a].type = "float";
        typeOfNode[t] = "float";
        //call table update
    }else if(p=="long" && q=="double"){
        tempNum++;
        string t = "t" + to_string(tempNum);
        string temp = t + " = " + "cast_to_double " + attr3AC[a].addrName;
        attr3AC[a].threeAC.push_back(temp);
        attr3AC[a].addrName = t;
        attr3AC[a].type = "double";
        typeOfNode[t] = "double";
        //call table update
    }else if(q == "int" && p=="long"){
        tempNum++;
        string t = "t" + to_string(tempNum);
        string temp = t + " = " + "cast_to_long " + attr3AC[b].addrName;
        attr3AC[b].threeAC.push_back(temp);
        attr3AC[b].addrName = t;
        attr3AC[b].type = "long";
        typeOfNode[t] = "long";
        //call table update
    }else if(p=="float" && q=="long"){
        tempNum++;
        string t = "t" + to_string(tempNum);
        string temp = t + " = " + "cast_to_float " + attr3AC[b].addrName;
        attr3AC[b].threeAC.push_back(temp);
        attr3AC[b].addrName = t;
        attr3AC[b].type = "float";
        typeOfNode[t] = "float";
        //call table update
    }else if(p == "double" && q=="long"){
        tempNum++;
        string t = "t" + to_string(tempNum);
        string temp = t + " = " + "cast_to_double " + attr3AC[b].addrName;
        attr3AC[b].threeAC.push_back(temp);
        attr3AC[b].addrName = t;
        attr3AC[b].type = "double";
        typeOfNode[t] = "double";
        //call table update
    }
    return;
}

void pushLabelUp(int par,int chld){
    if(trueLabel.find(chld)!=trueLabel.end())trueLabel[par]=trueLabel[chld];
    if(falseLabel.find(chld)!=falseLabel.end())falseLabel[par]=falseLabel[chld];
    if(nextLabel.find(chld)!=nextLabel.end())nextLabel[par]=nextLabel[chld];
    return;
}

void initializeAttributeVectors(){
    for(int i=0;i<nodeType.size();i++){
        attr temp;
        attr1 temp2;
        attrSymTab.push_back(temp2);
        attr3AC.push_back(temp);
    }
    currScope.push(make_pair(1,1));
    typeSize["int"]=4;
    typeSize["long"]=8;
    typeSize["float"]=4;
    typeSize["bool"]=1;
    typeSize["double"]=8;
    typeSize["char"]=2;
    typeSize["short"]=2;
    typeSize["boolean"]=1;
    return;
}


// map<int,pair<string,vector<int>>> arrayInfo;
// nodeNum -> {type,sizes[]}


// Given the array_name and nodeNumber, need to provide the array type and sizes
//    1. When the array is declared, store the sizes in the table [done]
//    2. Make a map nodeNum -> {Scope,tablePointer} [done]
//    3. Check for array_name in *tablePointer and it's parent scope tables to send out {type,sizes[]} stored in the table

// map nodeNum -> {Scope,tablePointer}

pair<string,vector<int>> getArrayInfo(string _arrayName, int _nodeNum){
    
    //return {type,arraySizes}

    // cout<<"inside getarrayinfo\n";

    vector<localTableParams>* primaryTable=scopeAndTable[_nodeNum].second;
    pair<int,int> startScope=scopeAndTable[_nodeNum].first;

    // check in current table and it's parent table (class table's parent is mapped to itself so no worries)
    pair<string,vector<int>> retObj;
    
    while(startScope.first>1){

        auto rowPtr=checkInScope(_arrayName,startScope,primaryTable);

        if (rowPtr!=NULL){

            // got the row

            retObj.first=getArrayType(rowPtr->type);
            retObj.second=rowPtr->arraySize;
            // cout<<"GOT IT: "<<retObj.first<<" "<<retObj.second.size()<<endl;
            return retObj;
        }

    startScope=mapParentScope[startScope];
    }

    startScope=scopeAndTable[_nodeNum].first;

    while(startScope.first>1){

        auto rowPtr=checkInScope(_arrayName,startScope,(parentTable[primaryTable]));

        if (rowPtr!=NULL){

            // got the row

            retObj.first=getArrayType(rowPtr->type);
            retObj.second=rowPtr->arraySize;

            // cout<<"GOT IT: "<<retObj.first<<" "<<retObj.second.size()<<endl;
            return retObj;
        }

        startScope=mapParentScope[startScope];
    }

    // cout<<"DIDN'T GET\n";
    return retObj;   
}

int checkIfValidSuperClass(string _superClass, int _line){

    for (auto globRow:globalTable){

        if (globRow.type=="class" && globRow.name==_superClass){
            return globRow.classNum;
        }
    }

    // Super class not found
    cout<<"[Compilation Error]: Undeclared class on line "<<_line<<"\nSuper Class '"<<_superClass<<"' must be declared before inheriting!\nAborting...\n";
    // exit(0);

    return 0;
}

bool checkIfValidArrayDeclaration(string _typeLHS, string _typeRHS, int _rDims,  int _line){
    
    int lDims=0;
    string lType_rev;
    for (int i=_typeLHS.length()-1; ;i--){

        if (_typeLHS[i]!=')'){
            lType_rev="";
            while(_typeLHS[i]!='('){
                lType_rev+=_typeLHS[i];
                i--;
            }
            break;
        }

        lDims++;
    }

    string lType="";

    for (int i=lType_rev.length()-1;i>=0;i--)
        lType+=lType_rev[i];

    if (lDims!=_rDims){
        cout<<"[Compilation Error]: Dimension mismatch on line "<<_line<<"\nLHS dimensions "<<lDims<<" do not match "<<_rDims<<"!\nAborting...\n";
        // exit(0);
    }


    if (lType==_typeRHS){
        return true;
    }
    else{
        cout<<"[Compilation Error]: Type mismatch on line "<<_line<<"\nArray Type "<<lType<<" does not match "<<_typeRHS<<"!\nAborting...\n";
        // exit(0);
    }
    return false;
}

bool checkIfTypeOkay(string _t1, string _t2){

    bool t1isInt=find(integerTypes.begin(),integerTypes.end(),_t1)!=integerTypes.end();
    bool t2isInt=find(integerTypes.begin(),integerTypes.end(),_t2)!=integerTypes.end();

    if (t1isInt && t2isInt){
        return true;
    }
    else if (!t1isInt && !t2isInt){
        return _t1==_t2;
    }
    else{
        return false;
    }
}

void checkRedeclaration(int n, string x){
    auto data = scopeAndTable[n];
    auto scope = data.first;
    auto table = data.second;
    auto partable = parentTable[table];
    while(scope.first>1){

        auto rowPtr=checkInScope(x,scope,table);

        if (rowPtr!=NULL){

            // got the row
            cout<<"[Compilation Error]: Variable Redeclaration on line "<<lineNum[n]<<"\nVariable '"<<  x << "' !\nAborting...\n";
            // exit(0);
        }

    scope=mapParentScope[scope];
    }
    scope=data.first;
    while(scope.first>1){

        auto rowPtr=checkInScope(x,scope,partable);

        if (rowPtr!=NULL){

            // got the row
            cout<<"[Compilation Error]: Variable Redeclaration on line "<<lineNum[n]<<"\nVariable '"<<  x << "' !\nAborting...\n";
            // exit(0);

        }

    scope=mapParentScope[scope];
    }

    return;
}

void checkIfDeclared(int n,string x){

    auto data = scopeAndTable[n];
    auto scope = data.first;
    auto table = data.second;
    auto partable = parentTable[table];
    int flag=0;
    int useLineNumber = lineNum[n];
    while(scope.first>1){

        auto rowPtr=checkInScope(x,scope,table);

        if (rowPtr!=NULL){
            if((*rowPtr).line <= useLineNumber)
            flag=1;
        }

    scope=mapParentScope[scope];
    }
    scope=data.first;
    while(scope.first>1){

        auto rowPtr=checkInScope(x,scope,partable);

        if (rowPtr!=NULL){

            // got the row
            if((*rowPtr).line <= useLineNumber)
            flag=1;

        }

    scope=mapParentScope[scope];
    }
    if(!flag){
        cout<<"[Compilation Error]: Variable Not Declared on line "<<lineNum[n]<<"\nVariable '"<<  x << "' !\nAborting...\n";
        // exit(0);
    }
    return;
}

void checkFunctionParameterTypes(int n, vector<int> paramNodeNo){
    // cout << "inside check function " << n << " " << paramNodeNo.size() << endl;
    // for(int i=0;i<paramNodeNo.size();i++)cout << i << " " << nodeType[paramNodeNo[i]] << " " << typeOfNode[to_string(paramNodeNo[i])] << endl;
    auto data = scopeAndTable[n];
    auto scope = data.first;
    auto table = data.second;
    auto partable = parentTable[table];
    string x = nodeType[n];
    while(scope.first>1){

        auto rowPtr=checkInScope(x,scope,table);

        if (rowPtr!=NULL){
            if((rowPtr->functionParams).size()!=paramNodeNo.size()){
                cout<<"[Compilation Error]: Incompatible function parameters on line "<<lineNum[n]<<"\nFunction'"<<  x << "' takes " <<
                (rowPtr->functionParams).size() <<" parameters, but you passed "<< paramNodeNo.size() << " parameters!\nAborting...\n";
                // exit(0);
            }
            for(int i=0;i<(rowPtr->functionParams).size();i++){
                if((rowPtr->functionParams)[i]!=typeOfNode[to_string(paramNodeNo[i])]){
                    if(typeOfNode[to_string(paramNodeNo[i])]=="notfound"){
                        cout<<"[Compilation Error]: Function parameter not declared on line "<<lineNum[n]<<"\nVariable '"<<  nodeType[paramNodeNo[i]] << "' !\nAborting...\n";
                        // exit(0);
                    }
                        cout<<"[Compilation Error]: Incompatible function parameter types on line "<<lineNum[n]<<"\n Parameter No. '"<<  i
                         << "' Expected type: "<< (rowPtr->functionParams)[i] <<" but got type: "<< typeOfNode[to_string(paramNodeNo[i])] <<"!\nAborting...\n";
                        // exit(0);
                }
            }
            // got the row
            // cout<<"[Compilation Error]: Variable Redeclaration on line "<<lineNum[n]<<"\nVariable '"<<  x << "' !\nAborting...\n";
            // exit(0);
        }

    scope=mapParentScope[scope];
    }
    scope=data.first;
    while(scope.first>1){

        auto rowPtr=checkInScope(x,scope,partable);

        if (rowPtr!=NULL){
            if((rowPtr->functionParams).size()!=paramNodeNo.size()){
                cout<<"[Compilation Error]: Incompatible function parameters on line "<<lineNum[n]<<"\nFunction'"<<  x << "' takes " <<
                (rowPtr->functionParams).size() <<" parameters, but you passed "<< paramNodeNo.size() << " parameters!\nAborting...\n";
                // exit(0);
            }
            for(int i=0;i<(rowPtr->functionParams).size();i++){
                if((rowPtr->functionParams)[i]!=typeOfNode[to_string(paramNodeNo[i])]){
                    if(typeOfNode[to_string(paramNodeNo[i])]=="notfound"){
                        cout<<"[Compilation Error]: Function parameter not declared on line "<<lineNum[n]<<"\nVariable '"<<  nodeType[paramNodeNo[i]] << "' !\nAborting...\n";
                        // exit(0);
                    }
                        cout<<"[Compilation Error]: Incompatibe function parameter types on line "<<lineNum[n]<<"\n Parameter No. '"<<  i
                         << "' Expected type: "<< (rowPtr->functionParams)[i] <<" but got type: "<< typeOfNode[to_string(paramNodeNo[i])] <<"!\nAborting...\n";
                        // exit(0);
                }
            }

        }

    scope=mapParentScope[scope];
    }

    return;
}

void preOrderTraversal(int nodeNum){

    // cout<<"Visiting: "<<nodeType[nodeNum]<<endl;
    if (nodeType[nodeNum]=="ClassDeclaration"){ 
        
        
        globalTableParams globRow ;
        
        // new class being formed (subclass is also a new class)
        classCount++;
        funcInClass=0;
        currSymTab= new vector<localTableParams>();
        globRow.localTablePointer = currSymTab;
        parentTable[currSymTab]=currSymTab;
        // update scopes before going inside the class 
        parentScope.push(currScope.top());
        currScope.push(make_pair(2,classCount));
        mapParentScope[currScope.top()]=parentScope.top();
        globRow.classNum=classCount;
        // cout<<currSymTab<<" From ClassDeclaration\n";

        //call the children from left to right
        
        // ClassDeclaration: Modifiersopt class Identifier Superopt Interfacesopt ClassBody
        // We are only interested in ClassBody which is always at the last and compulsory and Identifier
        
        // call Identifier
        switch (prodNum[nodeNum])
        {
            case 1:
            case 6:
            case 7:
            case 8:
                preOrderTraversal(adj[nodeNum][2]);
                globRow.name=attrSymTab[adj[nodeNum][2]].name;
                globRow.line=lineNum[adj[nodeNum][2]];
                break;
            default:
                preOrderTraversal(adj[nodeNum][1]);
                globRow.name=attrSymTab[adj[nodeNum][1]].name;
                globRow.line=lineNum[adj[nodeNum][1]];
                
        }


        // after Identifier before uper in ClassDeclaration
        // ->
        //put the class name in the classBody map
        int cb=adj[nodeNum][adj[nodeNum].size()-1];
        classNameMap[cb]=globRow.name;
        // cout<<lineNum[cb]<<" "<<globRow.name<<endl;
        // cout << "hax hax " << globRow.name << " " << lineNum[cb] << endl;

        // call Super 
        switch (prodNum[nodeNum])
        {   
            case 1:
            case 7:{ //Super at 4th
                int c3=adj[nodeNum][2];
                int c4=adj[nodeNum][3];
                preOrderTraversal(c4);
                globRow.parentClassNum=checkIfValidSuperClass(attrSymTab[c4].name,lineNum[c3]);
                // valid inheritence
                break;

            }
            case 2:
            case 4:{ //Super at 3rd
                int c2=adj[nodeNum][1];
                int c3=adj[nodeNum][2];
                preOrderTraversal(c3);
                globRow.parentClassNum=checkIfValidSuperClass(attrSymTab[c3].name,lineNum[c2]);
                // valid inheritence
                break;
            }
            default:{ // no Super
                
                break;
            }
        }

        // call ClassBody
        preOrderTraversal(adj[nodeNum][adj[nodeNum].size()-1]);
        
        // class ended, change scopes and complete the global table entry
        currScope.pop();
        parentScope.pop();
        
        globRow.type="class";
        globalTable.push_back(globRow);
        return;

    }
    else if (nodeType[nodeNum]=="Identifier"){

        // push the lexeme into Identifier's attributes
        attrSymTab[nodeNum].funcParams.push_back(nodeType[adj[nodeNum][0]]);

        attrSymTab[nodeNum].name=(nodeType[adj[nodeNum][0]]);
        // cout<<(nodeType[adj[nodeNum][0]])<<"\n";
        attrSymTab[nodeNum].type=(nodeType[adj[nodeNum][0]]);
        // attrSymTab[nodeNum].decLine=lineNum[nodeNum];
        int c1=adj[nodeNum][0];
        scopeAndTable[c1].first=currScope.top();
        // cout<<c1<<" "<<scopeAndTable[c1].first.first<<" "<<scopeAndTable[c1].first.second<<endl;
        scopeAndTable[c1].second=currSymTab;
        attrSymTab[nodeNum].leafNodeNum=c1;
        // cout << "identiifer " << c1 << " " <<  << endl;
        // cout<<"[From Identifier]"<<endl;
        typeOfNode[to_string(c1)]="fillMe";
        // cout<<nodeType[adj[nodeNum][0]]<<endl;
        if (attrSymTab[nodeNum].name=="true" || attrSymTab[nodeNum].name=="false"){
            typeOfNode[to_string(c1)]=attrSymTab[nodeNum].name;
            // cout<<to_string(c1)<<" "<<attrSymTab[nodeNum].name<<endl;
        }
        if(functionParameterMap.size()!=0 && functionParameterMap.find(attrSymTab[nodeNum].name)!=functionParameterMap.end()){
            typeOfNode[to_string(c1)] = functionParameterMap[attrSymTab[nodeNum].name];
            // cout << "assign map " << nodeType[adj[nodeNum][0]] << " " << c1 << " " << typeOfNode[to_string(c1)] << endl;
        }
        // cout << "exit identifier " << typeOfNode[to_string(c1)] << endl;
        return;
    }
    else if (nodeType[nodeNum]=="Super"){
        int c2=adj[nodeNum][1];
        preOrderTraversal(c2);
        attrSymTab[nodeNum].name=attrSymTab[c2].name;
        return;
    }
    else if (nodeType[nodeNum]=="ClassType" || nodeType[nodeNum]=="ClassOrInterfaceType"){
        int c1=adj[nodeNum][0];
        preOrderTraversal(c1);
        attrSymTab[nodeNum].name=attrSymTab[c1].name;
        attrSymTab[nodeNum].type=attrSymTab[c1].type; 
        return;
    }
    else if (nodeType[nodeNum]=="ClassBody"){

        // ClassBody:{ ClassBodyDeclarationsopt }
        // check if there is ClassBodyDeclarations and call it

        if (adj[nodeNum].size()==3){
            preOrderTraversal(adj[nodeNum][1]);
        }

        // else
        return;
    }
    else if (nodeType[nodeNum]=="FieldDeclaration"){
        // FieldDeclaration: Modifiersopt Type VariableDeclarators ;

        // new variable(s) being declared
        // localTableParams localRow ;

        if (prodNum[nodeNum]==1){
            // Modifiersopt is present
            // call Modifiers
            int cb4=adj[nodeNum][0];
            preOrderTraversal(cb4);

        }

        // call Type (always 3rd from the back)
        int cb3=adj[nodeNum][adj[nodeNum].size()-3];
        preOrderTraversal(cb3);

        // call VariableDeclarators (always 2nd from the back)
        int cb2=adj[nodeNum][adj[nodeNum].size()-2];
        preOrderTraversal(cb2);
        // cout<<"hey"<<endl;
        // cout<<attrSymTab[cb2].intParams.size()<<endl;

        //check if array is being declared
        if (attrSymTab[cb2].intParams.size()!=0){
            // cout<<"Hi bhai array declare hua\n";
            //array it is

            // check if initialized with correct type
            bool flag=checkIfValidArrayDeclaration(attrSymTab[cb3].type,attrSymTab[cb2].type,attrSymTab[cb2].intParams.size(),lineNum[nodeNum]);
            
            localTableParams locRow ;
            
            locRow.type=attrSymTab[cb3].type;

            locRow.name=attrSymTab[cb2].otherParams[0];
            locRow.line=lineNum[nodeNum];
            locRow.scope=currScope.top();
            locRow.parentScope=parentScope.top();
            locRow.arraySize=attrSymTab[cb2].intParams;
            // cout<<attrSymTab[cb2].intParams.size()<<endl;

            // cout<<"From FieldDeclaration:"<<locRow.arraySize.size()<<endl;
            (*currSymTab).push_back(locRow);
            return;

        }

        // save variable names in localRow along with their common type
        // add localRow(s) to currTable
        for (int i=0;i<attrSymTab[cb2].otherParams.size();i++){
            localTableParams locRow ;
            
            locRow.type=attrSymTab[cb3].type;

            locRow.name=attrSymTab[cb2].otherParams[i];
            locRow.line=lineNum[nodeNum];
            locRow.scope=currScope.top();
            locRow.parentScope=parentScope.top();
            (*currSymTab).push_back(locRow);
        }

        int c = adj[nodeNum][0];
        if (prodNum[nodeNum]==1){
            if (attrSymTab[c].name=="final"){
                isFinalField[attrSymTab[cb2].name]=true;
            }
        }

        return;

    }
    else if (nodeType[nodeNum]=="Type"){
        // Type: PrimitiveType | ReferenceType
        // call the child
        preOrderTraversal(adj[nodeNum][0]);
        
        // type is filled
        attrSymTab[nodeNum].type=attrSymTab[adj[nodeNum][0]].type;
        // cout<<"from Type: "<<attrSymTab[nodeNum].type<<endl;
        return;
    }
    else if (nodeType[nodeNum]=="PrimitiveType"){

        if (nodeType[adj[nodeNum][0]]=="boolean"){
            attrSymTab[nodeNum].type="boolean";
        }
        else{
            preOrderTraversal(adj[nodeNum][0]);
            attrSymTab[nodeNum].type=attrSymTab[adj[nodeNum][0]].type;
        }
        return;
    }
    else if (nodeType[nodeNum]=="NumericType" || nodeType[nodeNum]=="ReferenceType" || nodeType[nodeNum]=="ClassOrInterfaceType" || nodeType[nodeNum]=="Name" || nodeType[nodeNum]=="SimpleName"){

        preOrderTraversal(adj[nodeNum][0]);
        attrSymTab[nodeNum].type=attrSymTab[adj[nodeNum][0]].type;
        
        // cout<<nodeType[nodeNum]<<" from "<<attrSymTab[nodeNum].type<<endl;
        attrSymTab[nodeNum].name=attrSymTab[adj[nodeNum][0]].name;
        attrSymTab[nodeNum].leafNodeNum=attrSymTab[adj[nodeNum][0]].leafNodeNum;
        return;
    }
    else if (nodeType[nodeNum]=="IntegralType" || nodeType[nodeNum]=="FloatingPointType"){
        
        attrSymTab[nodeNum].type=nodeType[adj[nodeNum][0]];
        return;
    }
    else if (nodeType[nodeNum]=="QualifiedName"){
        //QualifiedName: Name . Identifier
        // saving type as the attribute 'type' in Identifier:
        int c1=adj[nodeNum][0];
        int c3=adj[nodeNum][2];
        preOrderTraversal(c1);
        preOrderTraversal(c3);
        attrSymTab[nodeNum].type=attrSymTab[c3].type ;
        attrSymTab[nodeNum].name=attrSymTab[c1].name+"."+attrSymTab[c3].name;
        // cout<<attrSymTab[nodeNum].name;
        return;
    }
    else if (nodeType[nodeNum]=="ArrayType"){
        
        for (auto child: adj[nodeNum]){
            preOrderTraversal(child);
        }

        int c1=adj[nodeNum][0];

        attrSymTab[nodeNum].type="array(" + attrSymTab[c1].type + ")";
        
    }
    else if (nodeType[nodeNum]=="VariableDeclarators"){
        
        for (auto child: adj[nodeNum]){
            preOrderTraversal(child);
        }

        int c1=adj[nodeNum][0];
        
        if (prodNum[nodeNum]==1){
            attrSymTab[nodeNum].otherParams.push_back(attrSymTab[c1].name);
            attrSymTab[nodeNum].type=attrSymTab[c1].type;
            attrSymTab[nodeNum].name=attrSymTab[c1].name;
            attrSymTab[nodeNum].intParams=attrSymTab[c1].intParams;
            attrSymTab[nodeNum].leafNodeNum=attrSymTab[adj[nodeNum][0]].leafNodeNum;
            // cout<<"In vardecls: "<<attrSymTab[nodeNum].leafNodeNum<<endl;
            // cout<<attrSymTab[nodeNum].intParams.size()<<endl;
        }
        else{
            int c3=adj[nodeNum][2];
            
            for (int i=0;i<attrSymTab[c1].otherParams.size();i++){
                attrSymTab[nodeNum].otherParams.push_back(attrSymTab[c1].otherParams[i]);
            }
            attrSymTab[nodeNum].otherParams.push_back(attrSymTab[c3].name);
        }
        return;
    }
    else if (nodeType[nodeNum]=="VariableDeclarator"){

        for(auto child:adj[nodeNum]){
            preOrderTraversal(child);
        }
        
        int c1=adj[nodeNum][0];

        attrSymTab[nodeNum].name=attrSymTab[c1].name;
        attrSymTab[nodeNum].leafNodeNum = attrSymTab[c1].leafNodeNum;
        if (prodNum[nodeNum]==2){
            int c3=adj[nodeNum][2];
            attrSymTab[nodeNum].type=attrSymTab[c3].type;
            attrSymTab[nodeNum].leafNodeNum=attrSymTab[adj[nodeNum][2]].leafNodeNum;
            initVal[attrSymTab[c1].name]=atoi(nodeType[attrSymTab[nodeNum].leafNodeNum].c_str());
            // cout<<"In vardec: "<< attrSymTab[nodeNum].leafNodeNum<<endl;
            attrSymTab[nodeNum].intParams=attrSymTab[c3].intParams;
        }
        return;
    }
    else if (nodeType[nodeNum]=="VariableDeclaratorId"){
        
        for (auto child: adj[nodeNum]){
            preOrderTraversal(child);
        }

        int c1=adj[nodeNum][0];
        attrSymTab[nodeNum].name=attrSymTab[c1].name;
        attrSymTab[nodeNum].leafNodeNum = attrSymTab[c1].leafNodeNum;
        attrSymTab[nodeNum].funcParams.push_back(attrSymTab[c1].funcParams[0]);
        checkRedeclaration(attrSymTab[c1].leafNodeNum,attrSymTab[c1].name);
        attrSymTab[nodeNum].funcParams.push_back(attrSymTab[c1].funcParams[0]);
        
        // cout << "in vdid " << attrSymTab[nodeNum].leafNodeNum << " " << attrSymTab[nodeNum].name << endl;
        if (prodNum[nodeNum]==2){
            attrSymTab[nodeNum].type=attrSymTab[c1].type;
            attrSymTab[nodeNum].intParams=attrSymTab[c1].intParams;
        }
        return;
    }
    else if (nodeType[nodeNum]=="PackageDeclaration"){
        //PackageDeclaration: package Name ;
        int c2=adj[nodeNum][1];
        preOrderTraversal(c2);

        globalTableParams globRow ;
        globRow.type="package";
        globRow.name=attrSymTab[c2].name;
        globRow.line=lineNum[nodeNum];
        globalTable.push_back(globRow);
        return;

    }
    else if (nodeType[nodeNum]=="TypeImportOnDemandDeclaration"){
        // TypeImportOnDemandDeclaration:import Name . * ;
        int c2=adj[nodeNum][1];
        preOrderTraversal(c2);
        globalTableParams globRow;
        globRow.name=attrSymTab[c2].name;
        globRow.type="import_on_demand";
        globRow.line=lineNum[nodeNum];
        globalTable.push_back(globRow);
        return;
    }
    else if (nodeType[nodeNum]=="SingleTypeImportDeclaration"){
        // SingleTypeImportDeclaration:import Name ;
        int c2=adj[nodeNum][1];
        preOrderTraversal(c2);
        globalTableParams globRow;
        globRow.name=attrSymTab[c2].name;
        globRow.type="import";
        globRow.line=lineNum[nodeNum];
        globalTable.push_back(globRow);
        return;
    }
    else if (nodeType[nodeNum]=="ConstructorDeclarator"){

        
        for (auto &child:adj[nodeNum]){
            preOrderTraversal(child);
        }

        int c1=adj[nodeNum][0];
        attrSymTab[nodeNum].name=attrSymTab[c1].name;

        switch (prodNum[nodeNum])
        {
        case 1:{
            int c3=adj[nodeNum][2];
            attrSymTab[nodeNum].otherParams=attrSymTab[c3].otherParams;
            int i=0;
            for (auto paramType:attrSymTab[c3].otherParams){
                
                localTableParams locRow ;
            
                locRow.type=paramType;

                locRow.name=attrSymTab[c3].funcParams[i];
                locRow.line=lineNum[nodeNum];
                locRow.scope=currScope.top();
                locRow.parentScope=parentScope.top();
                // cout<<attrSymTab[cb2].intParams.size()<<endl;

                // cout<<"From FieldDeclaration:"<<locRow.arraySize.size()<<endl;
                (*currSymTab).push_back(locRow);
                i++;
            }
        }
            
        break;
        
        default:
            break;
        }

        return;

    }
    else if (nodeType[nodeNum]=="ConstructorDeclaration"){
        
        funcInClass++ ;
        localTableParams locRow;
        locRow.scope=currScope.top();
        locRow.parentScope=parentScope.top();
            
        parentScope.push(currScope.top());
        currScope.push(make_pair(3,funcInClass));
        mapParentScope[currScope.top()]=parentScope.top();

        locRow.functionTablePointer=new vector<localTableParams>();
        locRow.type="method";
        vector<localTableParams>* saveClassSymTab=currSymTab;
        currSymTab=locRow.functionTablePointer;

        parentTable[currSymTab]=saveClassSymTab;

        for (auto &child:adj[nodeNum]){
            preOrderTraversal(child);
        }

        parentScope.pop();
        currScope.pop();
            
        currSymTab=saveClassSymTab;
        locRow.functionReturnType="none";

        switch (prodNum[nodeNum])
        {
        case 1:
        case 4:{
            locRow.name=attrSymTab[adj[nodeNum][1]].name;
            locRow.functionParams=attrSymTab[adj[nodeNum][1]].otherParams;

        }
        break;
        case 2:
        case 3:{
            locRow.name=attrSymTab[adj[nodeNum][0]].name;
            locRow.functionParams=attrSymTab[adj[nodeNum][0]].otherParams;
           break; 
        }
        default:
            break;

        }

        locRow.line=lineNum[nodeNum];
        
        (*currSymTab).push_back(locRow);
    }
    else if (nodeType[nodeNum]=="MethodDeclaration"){

            funcInClass++;

            localTableParams locRow;
            locRow.scope=currScope.top();
            locRow.parentScope=parentScope.top();
            
            parentScope.push(currScope.top());
            currScope.push(make_pair(3,funcInClass));
            mapParentScope[currScope.top()]=parentScope.top();
            
            locRow.functionTablePointer=new vector<localTableParams>();
            locRow.type="method";
            vector<localTableParams>* saveClassSymTab=currSymTab;
            currSymTab=locRow.functionTablePointer;

            parentTable[currSymTab]=saveClassSymTab;

            int c1=adj[nodeNum][0];
            int c2=adj[nodeNum][1];

            preOrderTraversal(c1);
            preOrderTraversal(c2);
            
            parentScope.pop();
            currScope.pop();
            
            currSymTab=saveClassSymTab;

            locRow.name=attrSymTab[c1].name;
            // cout<<locRow.name<<" From MethodDeclaration\n";
            locRow.functionReturnType=attrSymTab[c1].type;
            locRow.functionParams=attrSymTab[c1].otherParams;
            locRow.line=lineNum[nodeNum];

            (*currSymTab).push_back(locRow);
            // functionParameterMap.clear();
            return;
      
    }
    else if (nodeType[nodeNum]=="MethodHeader_"){
        int c1=adj[nodeNum][0];
        preOrderTraversal(c1);
        attrSymTab[nodeNum].type=attrSymTab[c1].type;
        attrSymTab[nodeNum].name=attrSymTab[c1].name;
        attrSymTab[nodeNum].otherParams=attrSymTab[c1].otherParams;
        return;
    }
    else if (nodeType[nodeNum]=="MethodHeader"){
            // MethodHeader: Modifiersopt Type/void MethodDeclarator Throwsopt
            
            // cout<<currSymTab<<" From MethodHeader\n";

            // get name,type and otherParams
            switch (prodNum[nodeNum])
            {
            case 1:
            case 3:{
    
                int c2=adj[nodeNum][1];
                int c3=adj[nodeNum][2];
                preOrderTraversal(c2);
                attrSymTab[nodeNum].type=attrSymTab[c2].type;
                preOrderTraversal(c3);// will give name and function params
                attrSymTab[nodeNum].name=attrSymTab[c3].name;
                for (auto fparam:attrSymTab[c3].otherParams){
                    attrSymTab[nodeNum].otherParams.push_back(fparam);
                }
                break;
            }
            case 2:
            case 4:{

                int c1=adj[nodeNum][0];
                int c2=adj[nodeNum][1];
                preOrderTraversal(c1);
                attrSymTab[nodeNum].type=attrSymTab[c1].type;
                preOrderTraversal(c2);
                attrSymTab[nodeNum].name=attrSymTab[c2].name;
                for (auto fparam:attrSymTab[c2].otherParams){
                    attrSymTab[nodeNum].otherParams.push_back(fparam);
                }
                break;
            }
            case 5:
            case 6:{
                
                int c3=adj[nodeNum][2];
                attrSymTab[nodeNum].type="void";
                preOrderTraversal(c3);
                attrSymTab[nodeNum].name=attrSymTab[c3].name;
                for (auto fparam:attrSymTab[c3].otherParams){
                    attrSymTab[nodeNum].otherParams.push_back(fparam);
                }
                break;
            }
            case 7:
            case 8:{
                int c2=adj[nodeNum][1];
                attrSymTab[nodeNum].type="void";
                preOrderTraversal(c2);
                attrSymTab[nodeNum].name=attrSymTab[c2].name;
                for (auto fparam:attrSymTab[c2].otherParams){
                    attrSymTab[nodeNum].otherParams.push_back(fparam);
                }
                break;
            }
            default:
                break;
            }
            
            
    }
    else if (nodeType[nodeNum]=="MethodDeclarator"){
        // MethodDeclarator: Identifier ( FormalParameterListopt ) 
        
        switch (prodNum[nodeNum])
        {
        case 1:{
            //call Identifier
            int c1=adj[nodeNum][0];
            preOrderTraversal(c1);
            attrSymTab[nodeNum].name=attrSymTab[c1].name;
            //call FormalParameterList
            int c3=adj[nodeNum][2];
            preOrderTraversal(c3);
            // update Formal Parameters' type
            int i=0;
            for (auto paramType:attrSymTab[c3].otherParams){
                attrSymTab[nodeNum].otherParams.push_back(paramType);
                localTableParams locRow ;
            
                locRow.type=paramType;

                locRow.name=attrSymTab[c3].funcParams[i];
                locRow.line=lineNum[nodeNum];
                locRow.scope=currScope.top();
                locRow.parentScope=parentScope.top();
                // cout<<attrSymTab[cb2].intParams.size()<<endl;

                // cout<<"From FieldDeclaration:"<<locRow.arraySize.size()<<endl;
                (*currSymTab).push_back(locRow);
                i++;
            }

            
            break;
        }
        case 2:{
            //call Identifier
            int c1=adj[nodeNum][0];
            preOrderTraversal(c1);
            attrSymTab[nodeNum].name=attrSymTab[c1].name;
            //no FormalParameterList
            break;
        }
        default:
            break;
        }
    }
    else if (nodeType[nodeNum]=="FormalParameterList"){

        switch (prodNum[nodeNum])
        {
        case 1:{
            int c1=adj[nodeNum][0];
            preOrderTraversal(c1);
            attrSymTab[nodeNum].funcParams.push_back(attrSymTab[c1].funcParams[0]);
            attrSymTab[nodeNum].otherParams.push_back(attrSymTab[c1].type);
            break;
        }
        case 2:{
            int c1=adj[nodeNum][0];
            int c3=adj[nodeNum][2];
            preOrderTraversal(c1);
            preOrderTraversal(c3);
            for (auto param:attrSymTab[c1].otherParams){
                attrSymTab[nodeNum].otherParams.push_back(param);
            }
            attrSymTab[nodeNum].otherParams.push_back(attrSymTab[c3].type);

            for (auto fp:attrSymTab[c1].funcParams){
                attrSymTab[nodeNum].funcParams.push_back(fp);

            }
            
            attrSymTab[nodeNum].funcParams.push_back(attrSymTab[c3].funcParams[0]);

        }
        default:
            break;
        }
        return;
    }
    else if (nodeType[nodeNum]=="FormalParameter"){
        // only need the Type
        int c1=adj[nodeNum][0];
        int c2 = adj[nodeNum][1];
        preOrderTraversal(c1);
        preOrderTraversal(c2);
        attrSymTab[nodeNum].funcParams.push_back(attrSymTab[c2].funcParams[0]);
        attrSymTab[nodeNum].type=attrSymTab[c1].type ;
        attrSymTab[nodeNum].leafNodeNum = attrSymTab[c2].leafNodeNum;
        // cout <<"inside formal parameter " << attrSymTab[c2].leafNodeNum << " " << attrSymTab[c2].name  << " " << attrSymTab[c1].type << endl;
        typeOfNode[to_string(attrSymTab[c2].leafNodeNum)] = attrSymTab[c1].type;
        // cout << "typeOfNode after " << typeOfNode[to_string(attrSymTab[c2].leafNodeNum)] << endl;
        functionParameterMap[attrSymTab[c2].name] = attrSymTab[nodeNum].type;
        // cout << functionParameterMap[attrSymTab[c2].name] << " " << attrSymTab[nodeNum].type << endl;
        return;
    }
    else if (nodeType[nodeNum]=="BlockStatement"){

        // if (prodNum[nodeNum]==1){

        //     int c1=adj[nodeNum][0];
        //     preOrderTraversal(c1);
        //     // attrSymTab[nodeNum].type=attrSymTab[c1].type;
        //     // attrSymTab[nodeNum].otherParams=attrSymTab[c1].otherParams;

        //     for (auto varName:attrSymTab[c1].otherParams){

        //         localTableParams locRow ;
        //         locRow.type=attrSymTab[c1].type;
        //         // cout<<locRow.type<<" From BlockStatement\n";
        //         locRow.name=varName;
        //         locRow.line=lineNum[nodeNum];
        //         locRow.scope=currScope.top();
        //         locRow.parentScope=parentScope.top();
        //         // cout<<"From BlockStatement: "<<varName<<endl; //works
        //         // cout<<locRow.type<<" "<<locRow.name<<endl; //works
        //         (*currSymTab).push_back(locRow);
        //         cout<<currSymTab<<" in blockstat "<<(*currSymTab).size()<<"\n";
        //         // cout<<currSymTab<<" From BlockStatement\n";
        //     }
        // }
        
        int c1=adj[nodeNum][0];
        preOrderTraversal(c1);
        

        return;
    }
    else if (nodeType[nodeNum]=="LocalVariableDeclarationStatement"){
        
            int c1=adj[nodeNum][0];
            preOrderTraversal(c1);
            attrSymTab[nodeNum].type=attrSymTab[c1].type;
            attrSymTab[nodeNum].otherParams=attrSymTab[c1].otherParams;
 
    }
    else if (nodeType[nodeNum]=="LocalVariableDeclaration"){
        int c1=adj[nodeNum][0];
        int c2=adj[nodeNum][1];
        // cout<<"In LocalVariableDeclaration\n";
        preOrderTraversal(c1);
        preOrderTraversal(c2);

        attrSymTab[nodeNum].type=attrSymTab[c1].type;
        attrSymTab[nodeNum].otherParams=attrSymTab[c2].otherParams;

        //check if array is being declared
        if (attrSymTab[c2].intParams.size()!=0){
            // cout<<"Hi bhai array declare hua\n";
            //array it is

            // check if initialized with correct type
            bool flag=checkIfValidArrayDeclaration(attrSymTab[c1].type,attrSymTab[c2].type,attrSymTab[c2].intParams.size(),lineNum[nodeNum]);
            localTableParams locRow ;
            
            locRow.type=attrSymTab[c1].type;

            locRow.name=attrSymTab[c2].otherParams[0];
            locRow.line=lineNum[nodeNum];
            locRow.scope=currScope.top();
            locRow.parentScope=parentScope.top();
            // cout<<attrSymTab[c2].intParams.size()<<endl;
            locRow.arraySize=attrSymTab[c2].intParams;
            if(locRow.type=="void"){
                cout<<"[Compilation Error]: Type mismatch on line "<<lineNum[nodeNum]<<"\nVariable '"<<  locRow.name << "' cannot have a type 'void'!\nAborting...\n";
                // exit(0);
            }
            
            // cout<<"From FieldDeclaration:"<<locRow.arraySize.size()<<endl;
            (*currSymTab).push_back(locRow);
            // cout<<currSymTab<<" in locvaldec "<<(*currSymTab).size()<<"\n";
            
            // for (auto ro:*(currSymTab))
            //     cout<<ro.name<<" "<<ro.arraySize.size()<<endl;
            return;

        }
        else{

            if (initVal.find(attrSymTab[c2].name)!=initVal.end()){
                
                // var is initialized as well

                int c1=adj[nodeNum][0];
                int c2=adj[nodeNum][1];
                initVal[attrSymTab[c2].name]=atoi(nodeType[attrSymTab[c2].leafNodeNum].c_str());          
                // cout<<typeOfNode[to_string(attrSymTab[c1].leafNodeNum)]<<" "<<typeOfNode[to_string(attrSymTab[c3].leafNodeNum)]<<endl;
                
                string t3=fillHelper(to_string(attrSymTab[c2].leafNodeNum));
                
                // cout<<"[In LocVarDec]: "<<" "<<t3<<endl;
                string t1=attrSymTab[c1].type;
                t3=typeOfNode[to_string(attrSymTab[c2].leafNodeNum)];

                if (t1[t1.size()-1]>'0' && t1[t1.size()-1]<'9'){
                            t1=removeLastChar(t1);
                }

                if (t3[t3.size()-1]>'0' && t3[t3.size()-1]<'9'){
                            t3=removeLastChar(t3);
                            
                        }
                // cout<<typeOfNode[to_string(attrSymTab[c1].leafNodeNum)]<<" "<<typeOfNode[to_string(attrSymTab[c2].leafNodeNum)]<<endl;                     

                if (checkIfTypeOkay(t1,t3))
                        attrSymTab[nodeNum].leafNodeNum=attrSymTab[c1].leafNodeNum;
                else{
                  
                    string var1=nodeType[attrSymTab[c1].leafNodeNum];
                    string var2=nodeType[attrSymTab[c2].leafNodeNum];

                    // cout<<"[Compilation Error]: Type mismatch on line "<<lineNum[nodeNum]<<"\nType '"<<t1<<"' does not match type '"<<t3<<"' of '"<<var2<<"'!\nAborting...\n";
                    // exit(0);
                }
            }

            // attrSymTab[nodeNum].type=attrSymTab[c1].type;
            // attrSymTab[nodeNum].otherParams=attrSymTab[c1].otherParams;
            // cout<<attrSymTab[c2].otherParams.size()<<endl;
            for (auto varName:attrSymTab[c2].otherParams){

                localTableParams locRow ;
                locRow.type=attrSymTab[c1].type;

                // cout<<locRow.type<<" From BlockStatement\n";
                locRow.name=varName;
                locRow.line=lineNum[nodeNum];
                locRow.scope=currScope.top();
                locRow.parentScope=parentScope.top();
                if(locRow.type=="void"){
                cout<<"[Compilation Error]: Type mismatch on line "<<lineNum[nodeNum]<<"\nVariable '"<<  locRow.name << "' cannot have a type 'void'!\nAborting...\n";
                // exit(0);
            }
                // cout<<"From BlockStatement: "<<varName<<endl; //works
                // cout<<locRow.type<<" "<<locRow.name<<endl; //works
                // checkRedeclaration(attrSymTab[c2].leafNodeNum,locRow.name);
                (*currSymTab).push_back(locRow);
                // cout<<currSymTab<<" in locvardec "<<(*currSymTab).size()<<"\n";
                // cout<<currSymTab<<" From BlockStatement\n";
            }
        }

        return;
    }
    else if (nodeType[nodeNum]=="ForStatement" || nodeType[nodeNum]=="IfThenStatement" || nodeType[nodeNum]=="WhileStatement"){
        //change scopes
        if (scopeHelper.size()==(currScope.top().first-3)){
            scopeHelper.push_back(0);
        }

        // currScope.top().first={3,4,5} 
        scopeHelper[currScope.top().first-3]++;
        parentScope.push(currScope.top());
        currScope.push(make_pair(currScope.top().first+1,scopeHelper[currScope.top().first-3]));
        mapParentScope[currScope.top()]=parentScope.top();
        for (auto child:adj[nodeNum]){
            preOrderTraversal(child);
        }
        scopeAndTable[nodeNum].first=currScope.top();
        scopeAndTable[nodeNum].second=currSymTab;
        parentScope.pop();
        currScope.pop();
        return;
    }
    else if (nodeType[nodeNum]=="IfThenElseStatement"){
        
        int c3=adj[nodeNum][2];
        int c5=adj[nodeNum][4];
        int c7=adj[nodeNum][6];

        //change scopes for IF body
        if (scopeHelper.size()==(currScope.top().first-3)){
            scopeHelper.push_back(0);
        }
        
        // currScope.top().first={3,4,5} 
        scopeHelper[currScope.top().first-3]++;
        parentScope.push(currScope.top());
        currScope.push(make_pair(currScope.top().first+1,scopeHelper[currScope.top().first-3]));
        mapParentScope[currScope.top()]=parentScope.top();
        preOrderTraversal(c3);
        preOrderTraversal(c5);

        parentScope.pop();
        currScope.pop();

        //change scopes for ELSE body
        if (scopeHelper.size()==(currScope.top().first-3)){
            scopeHelper.push_back(0);
        }
        
        // currScope.top().first={3,4,5} 
        scopeHelper[currScope.top().first-3]++;
        parentScope.push(currScope.top());
        currScope.push(make_pair(currScope.top().first+1,scopeHelper[currScope.top().first-3]));
        mapParentScope[currScope.top()]=parentScope.top();
        preOrderTraversal(c7);

        parentScope.pop();
        currScope.pop();

        return;
    }
    else if (nodeType[nodeNum]=="ForInit"){
        
        int c1=adj[nodeNum][0];
        preOrderTraversal(c1);

        // for (auto var:attrSymTab[c1].otherParams){
        //     localTableParams locRow ;
        //     locRow.name=var;
        //     locRow.type=attrSymTab[c1].type;
        //     locRow.scope=currScope.top();
        //     locRow.parentScope=parentScope.top();
        //     locRow.line=lineNum[nodeNum];
        //     currSymTab->push_back(locRow);

        // }
        return;
        
    }
    else if (nodeType[nodeNum]=="ArrayCreationExpression"){

        for(auto child:adj[nodeNum])
            preOrderTraversal(child);

        int c2=adj[nodeNum][1];
        int c3=adj[nodeNum][2];

        attrSymTab[nodeNum].type=attrSymTab[c2].type;
        attrSymTab[nodeNum].intParams=attrSymTab[c3].intParams;
        // cout<<"[PreOrderTraversal] From ArrayCreationExpression: "<<attrSymTab[nodeNum].intParams.size()<<endl;
        // for (auto siz:attrSymTab[nodeNum].intParams)
        //     cout<<siz<<" ";
        // cout<<endl;
        // cout<< attrSymTab[nodeNum].type<<endl;

        return;
    }
    else if (nodeType[nodeNum]=="DimExprs"){

        for (auto child:adj[nodeNum])
            preOrderTraversal(child);
        
        if (prodNum[nodeNum]==1){
            int c1=adj[nodeNum][0];
            attrSymTab[nodeNum].intParams.push_back(attrSymTab[c1].num);
        }
        else{
            int c1=adj[nodeNum][0];
            int c2=adj[nodeNum][1];

            for (auto intParam:attrSymTab[c1].intParams)
                attrSymTab[nodeNum].intParams.push_back(intParam);
            
            attrSymTab[nodeNum].intParams.push_back(attrSymTab[c2].num);

        }

        // for (auto siz:attrSymTab[nodeNum].intParams)
        //     cout<<siz<<" ";
        // cout<<endl;
        return;
        
    }
    else if (nodeType[nodeNum]=="DimExpr"){
        
        int c2=adj[nodeNum][1];
        preOrderTraversal(c2);

        attrSymTab[nodeNum].num=attrSymTab[c2].num;    
        
        return;
    }
    else if (nodeType[nodeNum]=="Expression"){
        
        int c1=adj[nodeNum][0];
        preOrderTraversal(c1);

        attrSymTab[nodeNum].num=attrSymTab[c1].num;    
        attrSymTab[nodeNum].type=attrSymTab[c1].type;
        attrSymTab[nodeNum].intParams=attrSymTab[c1].intParams;
        attrSymTab[nodeNum].leafNodeNum=attrSymTab[c1].leafNodeNum;
        
        return;
    }
    else if (nodeType[nodeNum]=="UnaryExpression"){
        
        for (auto child:adj[nodeNum])
            preOrderTraversal(child);

        if (prodNum[nodeNum]==5){
            
            int c1=adj[nodeNum][0];
            attrSymTab[nodeNum].num=attrSymTab[c1].num;  
            // cout<<"From Uexp: "<< attrSymTab[nodeNum].num<<endl;
            attrSymTab[nodeNum].type=attrSymTab[c1].type;
            attrSymTab[nodeNum].intParams=attrSymTab[c1].intParams;
            // cout<<"From Uexp: ";
            // for (auto siz:attrSymTab[nodeNum].intParams)
            //     cout<<siz<<" ";
            // cout<< attrSymTab[nodeNum].type<<endl;
            // cout<<endl;
            attrSymTab[nodeNum].leafNodeNum=attrSymTab[adj[nodeNum][0]].leafNodeNum;
        }

        switch (prodNum[nodeNum])
        {
        case 1:
        case 2:{
            attrSymTab[nodeNum].leafNodeNum=attrSymTab[adj[nodeNum][0]].leafNodeNum;
            break;
        }
        case 3:
        case 4:{
            attrSymTab[nodeNum].leafNodeNum=attrSymTab[adj[nodeNum][1]].leafNodeNum;
            break;
        }
        default:
            break;
        }
        return;
    }
    else if (nodeType[nodeNum]=="PostIncrementExpression"|| nodeType[nodeNum]=="PostDecrementExpression"){
        int c1=adj[nodeNum][0];
        preOrderTraversal(c1);

        attrSymTab[nodeNum].leafNodeNum=attrSymTab[adj[nodeNum][0]].leafNodeNum;
    }
    else if (nodeType[nodeNum]=="PostfixExpression" ){
        
        int c1=adj[nodeNum][0];
        preOrderTraversal(c1);
        if (prodNum[nodeNum]==1){
            
            attrSymTab[nodeNum].num=attrSymTab[c1].num;   
            attrSymTab[nodeNum].type=attrSymTab[c1].type;
            attrSymTab[nodeNum].leafNodeNum=attrSymTab[adj[nodeNum][0]].leafNodeNum;
            attrSymTab[nodeNum].intParams=attrSymTab[c1].intParams;
            // for (auto siz:attrSymTab[nodeNum].intParams)
            //     cout<<siz<<" ";
            
            // cout<<endl;
        }
        else if (prodNum[nodeNum]==2){

            attrSymTab[nodeNum].num=initVal[attrSymTab[c1].name];
        }
        attrSymTab[nodeNum].leafNodeNum=attrSymTab[adj[nodeNum][0]].leafNodeNum;
        return;
    }
    else if (nodeType[nodeNum]=="Primary"){
        
        int c1=adj[nodeNum][0];
        preOrderTraversal(c1);

        if (prodNum[nodeNum]==1){
            attrSymTab[nodeNum].name=attrSymTab[c1].name;
            attrSymTab[nodeNum].num=attrSymTab[c1].num;    
            attrSymTab[nodeNum].leafNodeNum=attrSymTab[c1].leafNodeNum;
            typeOfNode[to_string(attrSymTab[nodeNum].leafNodeNum)]=fillHelper(to_string(attrSymTab[nodeNum].leafNodeNum));
            // cout<<"From Primary: "<<nodeType[attrSymTab[nodeNum].leafNodeNum]<<" "<<attrSymTab[nodeNum].leafNodeNum<<endl;
            // cout<<typeOfNode[to_string(attrSymTab[nodeNum].leafNodeNum)]<<endl;
            // cout<<"From Primary: "<<attrSymTab[nodeNum].num<<endl;
        }
        else{

            attrSymTab[nodeNum].type=attrSymTab[c1].type;
            attrSymTab[nodeNum].intParams=attrSymTab[c1].intParams;
            attrSymTab[nodeNum].leafNodeNum=attrSymTab[c1].leafNodeNum;
            // cout<<"From Primary: "<<attrSymTab[nodeNum].intParams.size()<<endl;
        }
        return;
    }
    else if (nodeType[nodeNum]=="AdditiveExpression"){
        
        for (auto child:adj[nodeNum])
            preOrderTraversal(child);

        if (prodNum[nodeNum]==1){
            
            int c1=adj[nodeNum][0];
            attrSymTab[nodeNum].num=attrSymTab[c1].num; 
            // cout<<"FRom Additive Expression:"<<attrSymTab[nodeNum].num<<endl;
            attrSymTab[nodeNum].type=attrSymTab[c1].type;
            attrSymTab[nodeNum].intParams=attrSymTab[c1].intParams;
            // cout<<"From  AddExpre"<<   attrSymTab[nodeNum].intParams.size()<<endl;
            // cout<<attrSymTab[nodeNum].type<<"\n";
            attrSymTab[nodeNum].leafNodeNum=attrSymTab[adj[nodeNum][0]].leafNodeNum;
        }
        else {

            int c1=adj[nodeNum][0];
            int c3=adj[nodeNum][2];
            // cout<<typeOfNode[to_string(attrSymTab[c1].leafNodeNum)]<<" "<<typeOfNode[to_string(attrSymTab[c3].leafNodeNum)]<<endl;
            // cout<<t1<<" "<<t3<<endl;
            // cout<<typeOfNode[to_string(attrSymTab[c1].leafNodeNum)]<<" "<<typeOfNode[to_string(attrSymTab[c3].leafNodeNum)]<<endl;
            string t1=fillHelper(to_string(attrSymTab[c1].leafNodeNum));
            string t3=fillHelper(to_string(attrSymTab[c3].leafNodeNum));

            // cout<<t1<<" from additive expression "<<t3<<endl;
            t1=typeOfNode[to_string(attrSymTab[c1].leafNodeNum)];
            t3=typeOfNode[to_string(attrSymTab[c3].leafNodeNum)];

            if (t1[t1.size()-1]>'0' && t1[t1.size()-1]<'9'){
                    t1=removeLastChar(t1);
                }

            if (t3[t3.size()-1]>'0' && t3[t3.size()-1]<'9'){
                    t3=removeLastChar(t3);
                    
                }
            // cout<<typeOfNode[to_string(attrSymTab[c1].leafNodeNum)]<<" "<<typeOfNode[to_string(attrSymTab[c3].leafNodeNum)]<<endl;
            
            if (checkIfTypeOkay(t1,t3))
                attrSymTab[nodeNum].leafNodeNum=attrSymTab[c1].leafNodeNum;
            else{

                string var1=nodeType[attrSymTab[c1].leafNodeNum];
                string var2=nodeType[attrSymTab[c3].leafNodeNum];
                if(t1=="notfound"){
                    cout<<"[Compilation Error]: Variable Not Declared on line "<<lineNum[nodeNum]<<"\nVariable '"<<  var1 << "' !\nAborting...\n";
                    // exit(0);
                }
                if(t3=="notfound"){
                    cout<<"[Compilation Error]: Variable Not Declared on line "<<lineNum[nodeNum]<<"\nVariable '"<<  var2 << "' !\nAborting...\n";
                    // exit(0);
                }
                cout<<"[Compilation Error]: Type mismatch on line "<<lineNum[nodeNum]<<"\nType '"<<t1<<"' of '"<<var1<<"' does not match type '"<<t3<<"' of '"<<var2<<"'!\nAborting...\n";
                // exit(0);
            }
        }
        return;


    }
    else if (nodeType[nodeNum]=="Modifier"){
        preOrderTraversal(adj[nodeNum][0]);
        attrSymTab[nodeNum].name=nodeType[adj[nodeNum][0]];
    }
    else if (nodeType[nodeNum]=="Modifiers"){
        for (auto child:adj[nodeNum])
            preOrderTraversal(child);
        if (prodNum[nodeNum]==1){
           attrSymTab[nodeNum].name = attrSymTab[adj[nodeNum][0]].name;
        }
    }
    else if (nodeType[nodeNum]=="FieldAccess"){
        for (auto child:adj[nodeNum])
            preOrderTraversal(child);
        
        if (prodNum[nodeNum]==1 && isInAssignment){

            if (attrSymTab[adj[nodeNum][0]].name=="this"){
                // cout<<isFinalField["m"]<<endl;
                if ((isFinalField.find(attrSymTab[adj[nodeNum][2]].name))!=isFinalField.end()){
                    cout<<"[Compilation Error]: Cannot assign a value to final field on line "<<lineNum[nodeNum]<<" to '"<<attrSymTab[adj[nodeNum][2]].name<<"'!\nAborting...\n";
                    // exit(0);
                }
            }
        }
    }
    else if (nodeType[nodeNum]=="Assignment"){
        isInAssignment=true;
        for (auto child: adj[nodeNum])
            preOrderTraversal(child);
        
        int c1=adj[nodeNum][0];
        int c3=adj[nodeNum][2];
        // cout<<typeOfNode[to_string(attrSymTab[c1].leafNodeNum)]<<" "<<typeOfNode[to_string(attrSymTab[c3].leafNodeNum)]<<endl;
        string t1=fillHelper(to_string(attrSymTab[c1].leafNodeNum));
        string t3=fillHelper(to_string(attrSymTab[c3].leafNodeNum));
        // cout<<"[In Assignment]: "<<t1<<" "<<t3<<endl;
        t1=typeOfNode[to_string(attrSymTab[c1].leafNodeNum)];
        t3=typeOfNode[to_string(attrSymTab[c3].leafNodeNum)];

        if (t1[t1.size()-1]>'0' && t1[t1.size()-1]<'9'){
                    t1=removeLastChar(t1);
                }

        if (t3[t3.size()-1]>'0' && t3[t3.size()-1]<'9'){
                    t3=removeLastChar(t3);
                    
                }
            // cout<<typeOfNode[to_string(attrSymTab[c1].leafNodeNum)]<<" "<<typeOfNode[to_string(attrSymTab[c3].leafNodeNum)]<<endl;
            
        if (checkIfTypeOkay(t1,t3))
                attrSymTab[nodeNum].leafNodeNum=attrSymTab[c1].leafNodeNum;
        else{

            string var1=nodeType[attrSymTab[c1].leafNodeNum];
            string var2=nodeType[attrSymTab[c3].leafNodeNum];

            // cout<<"[Compilation Error]: Type mismatch on line "<<lineNum[nodeNum]<<"\nType '"<<t1<<"' of '"<<var1<<"' does not match type '"<<t3<<"' of '"<<var2<<"'!\nAborting...\n";
            // exit(0);
        }
        isInAssignment=false;

    }
    else if (nodeType[nodeNum]=="LeftHandSide"){

        preOrderTraversal(adj[nodeNum][0]);

        attrSymTab[nodeNum].leafNodeNum=attrSymTab[adj[nodeNum][0]].leafNodeNum;
        
        return;
    }
    else if (nodeType[nodeNum]=="MultiplicativeExpression"){
        for (auto child:adj[nodeNum])
            preOrderTraversal(child);
        
        switch (prodNum[nodeNum])
        {   
            case 1:{
                attrSymTab[nodeNum].leafNodeNum=attrSymTab[adj[nodeNum][0]].leafNodeNum;
                attrSymTab[nodeNum].num=attrSymTab[adj[nodeNum][0]].num;
                attrSymTab[nodeNum].intParams=attrSymTab[adj[nodeNum][0]].intParams;
                attrSymTab[nodeNum].type=attrSymTab[adj[nodeNum][0]].type;

                break;
            }
            case 2:
            case 3:
            case 4:{
                int c1=adj[nodeNum][0];
                int c3=adj[nodeNum][2];

                string t1=fillHelper(to_string(attrSymTab[c1].leafNodeNum));
                string t3=fillHelper(to_string(attrSymTab[c3].leafNodeNum));
                    
                    // cout<<typeOfNode[to_string(attrSymTab[c1].leafNodeNum)]<<" "<<typeOfNode[to_string(attrSymTab[c3].leafNodeNum)]<<endl;
                t1=typeOfNode[to_string(attrSymTab[c1].leafNodeNum)];
                t3=typeOfNode[to_string(attrSymTab[c3].leafNodeNum)];

                if (t1[t1.size()-1]>'0' && t1[t1.size()-1]<'9'){
                    t1=removeLastChar(t1);
                }

                if (t3[t3.size()-1]>'0' && t3[t3.size()-1]<'9'){
                    t3=removeLastChar(t3);
                    
                }   
                if (checkIfTypeOkay(t1,t3))
                        attrSymTab[nodeNum].leafNodeNum=attrSymTab[c1].leafNodeNum;
                else{

                    string var1=nodeType[attrSymTab[c1].leafNodeNum];
                    string var2=nodeType[attrSymTab[c3].leafNodeNum];
                    if(t1=="notfound"){
                        cout<<"[Compilation Error]: Variable Not Declared on line "<<lineNum[nodeNum]<<"\nVariable '"<<  var1 << "' !\nAborting...\n";
                        // exit(0);
                    }
                    if(t3=="notfound"){
                        cout<<"[Compilation Error]: Variable Not Declared on line "<<lineNum[nodeNum]<<"\nVariable '"<<  var2 << "' !\nAborting...\n";
                        // exit(0);
                    }
                    cout<<"[Compilation Error]: Type mismatch on line "<<lineNum[nodeNum]<<"\nType '"<<t1<<"' of '"<<var1<<"' does not match type '"<<t3<<"' of '"<<var2<<"'!\nAborting...\n";
                    // exit(0);
                }

                break;
            }
            default: 
                break;
        }
    }
    else if (nodeType[nodeNum]=="CastExpression"){

        for (auto child:adj[nodeNum])
            preOrderTraversal(child);
        
        switch (prodNum[nodeNum])
        {
        case 1:{
            int c2=adj[nodeNum][1];
            typeOfNode[to_string(nodeNum)]=attrSymTab[c2].type;
            attrSymTab[nodeNum].leafNodeNum=nodeNum;
            break;
        }
        case 2:{
            int c4=adj[nodeNum][3];
            typeOfNode[to_string(nodeNum)]=attrSymTab[c4].type;
            attrSymTab[nodeNum].leafNodeNum=nodeNum;
            break;
        }
        case 3:{
            int c2=adj[nodeNum][1];
            typeOfNode[to_string(nodeNum)]=attrSymTab[c2].type;
            attrSymTab[nodeNum].leafNodeNum=nodeNum;
            break;
        }
        default:
            break;
        }

    }
    else if (nodeType[nodeNum]=="UnaryExpressionNotPlusMinus"){
        
        for (auto child:adj[nodeNum])
            preOrderTraversal(child);

        if (prodNum[nodeNum]==1){
            
            int c1=adj[nodeNum][0];
            attrSymTab[nodeNum].num=attrSymTab[c1].num; 
            attrSymTab[nodeNum].type=attrSymTab[c1].type;
            attrSymTab[nodeNum].intParams=attrSymTab[c1].intParams;  
            attrSymTab[nodeNum].leafNodeNum=attrSymTab[adj[nodeNum][0]].leafNodeNum;
        }

        switch (prodNum[nodeNum])
        {
        case 1:{
            int c1=adj[nodeNum][0];
            attrSymTab[nodeNum].num=attrSymTab[c1].num; 
            attrSymTab[nodeNum].type=attrSymTab[c1].type;
            attrSymTab[nodeNum].intParams=attrSymTab[c1].intParams;  
            attrSymTab[nodeNum].leafNodeNum=attrSymTab[adj[nodeNum][0]].leafNodeNum;
            break;
        }
        case 2:
        case 3:{
            int c2=adj[nodeNum][1];
            // attrSymTab[nodeNum].num=attrSymTab[c2].num; 
            attrSymTab[nodeNum].type=attrSymTab[c2].type;
            // attrSymTab[nodeNum].intParams=attrSymTab[c2].intParams;  
            attrSymTab[nodeNum].leafNodeNum=attrSymTab[c2].leafNodeNum;
            break;
        }
        case 4:{
            int c1=adj[nodeNum][0];
            attrSymTab[nodeNum].leafNodeNum=attrSymTab[c1].leafNodeNum;

        }
        default:
            break;
        }

    }
    else if (nodeType[nodeNum]=="ArrayAccess"){
        
        int c1=adj[nodeNum][0];
        int c3=adj[nodeNum][2];
        preOrderTraversal(c1);
        preOrderTraversal(c3);
        attrSymTab[nodeNum].leafNodeNum=attrSymTab[c1].leafNodeNum;
    }
    else if (nodeType[nodeNum]=="AssignmentExpression"){
        
        int c1=adj[nodeNum][0];
        preOrderTraversal(c1);
            attrSymTab[nodeNum].num=attrSymTab[c1].num; 
            
            // cout<<nodeType[nodeNum]<<" "<<attrSymTab[nodeNum].num<<endl;
            attrSymTab[nodeNum].type=attrSymTab[c1].type;
            // cout<<"From ExclusiveOrExpression: "<<attrSymTab[c1].intParams.size()<<endl;
            attrSymTab[nodeNum].intParams=attrSymTab[c1].intParams;  
            attrSymTab[nodeNum].leafNodeNum=attrSymTab[adj[nodeNum][0]].leafNodeNum; 
            // cout<<"assexrp: "<<attrSymTab[nodeNum].leafNodeNum<<endl;
    }
    else if (nodeType[nodeNum]=="PrimaryNoNewArray" || nodeType[nodeNum]=="ShiftExpression" || nodeType[nodeNum]=="RelationalExpression" || nodeType[nodeNum]=="EqualityExpression" || nodeType[nodeNum]=="AndExpression" || nodeType[nodeNum]=="ExclusiveOrExpression" || nodeType[nodeNum]=="InclusiveOrExpression"|| nodeType[nodeNum]=="ConditionalAndExpression" || nodeType[nodeNum]=="ConditionalOrExpression" || nodeType[nodeNum]=="ConditionalExpression" ){
        
        for (auto child:adj[nodeNum])
            preOrderTraversal(child);


        switch (prodNum[nodeNum])
        {
        case 1:{
            int c1=adj[nodeNum][0];
            attrSymTab[nodeNum].num=attrSymTab[c1].num; 
            
            // cout<<nodeType[nodeNum]<<" "<<attrSymTab[nodeNum].num<<endl;
            attrSymTab[nodeNum].type=attrSymTab[c1].type;
            // cout<<"From ExclusiveOrExpression: "<<attrSymTab[c1].intParams.size()<<endl;
            attrSymTab[nodeNum].intParams=attrSymTab[c1].intParams;  
            attrSymTab[nodeNum].leafNodeNum=attrSymTab[adj[nodeNum][0]].leafNodeNum; 
            if (nodeType[nodeNum]=="InclusiveOrExpression"){
                // cout<<"From IncOrEx:";
                // for (auto siz:attrSymTab[nodeNum].intParams)
                //     cout<<siz<<" ";
                // cout<<endl;
                // cout<<attrSymTab[nodeNum].type<<endl;
            }

            break;
        }
        case 2:{

            int c1=adj[nodeNum][0];
            attrSymTab[nodeNum].name=nodeType[c1];
            break;
        }
        case 3:{
            attrSymTab[nodeNum].leafNodeNum=attrSymTab[adj[nodeNum][1]].leafNodeNum; 
            // cout<<attrSymTab[nodeNum].leafNodeNum<<": from PNNA"<<endl;
            
            break;
        }
        case 6:{
            attrSymTab[nodeNum].leafNodeNum=attrSymTab[adj[nodeNum][0]].leafNodeNum; 

            // cout<<attrSymTab[nodeNum].leafNodeNum<<endl;
            break;
        }
        default:
            break;
        }
        return;
    }
    else if (nodeType[nodeNum]=="Literal"){
        int c1=adj[nodeNum][0];
        preOrderTraversal(c1);
        attrSymTab[nodeNum].num=attrSymTab[c1].num;

        scopeAndTable[c1].first=currScope.top();
        // cout<<c1<<" "<<scopeAndTable[c1].first.first<<" "<<scopeAndTable[c1].first.second<<endl;
        scopeAndTable[c1].second=currSymTab;
        attrSymTab[nodeNum].leafNodeNum=adj[c1][0];

        switch (prodNum[nodeNum])
        {
        case 1:{
            typeOfNode[to_string(adj[c1][0])]="int";
            break;
        }
        case 2:
            typeOfNode[to_string(adj[c1][0])]="float";
            break;
        case 3:
            typeOfNode[to_string(adj[c1][0])]="double";
            break;
        case 4:
            typeOfNode[to_string(adj[c1][0])]="boolean";
            // cout<<"Hi\n";
            // cout<<to_string(adj[c1][0])<<endl;
            break;
        case 5:
            typeOfNode[to_string(adj[c1][0])]="char";
            break;
        case 6:
            typeOfNode[to_string(adj[c1][0])]="String";
            break;
        case 7:
            typeOfNode[to_string(adj[c1][0])]="null";
            break;
        default:
            break;
        }
        return;
    }
    else if (nodeType[nodeNum]=="IntegerLiteral"  ){
        int c1=adj[nodeNum][0];
        attrSymTab[nodeNum].num=stoi(nodeType[c1]);
        return;
    }
    else if (nodeType[nodeNum]=="VariableInitializer"){
        
        int c1=adj[nodeNum][0];
        preOrderTraversal(c1);

        if (prodNum[nodeNum]==1){
            attrSymTab[nodeNum].type=attrSymTab[c1].type;
            attrSymTab[nodeNum].intParams=attrSymTab[c1].intParams;
            attrSymTab[nodeNum].leafNodeNum=attrSymTab[adj[nodeNum][0]].leafNodeNum;
            // cout<<"[PreOrderTraversal] From VariableInitializer: "<<attrSymTab[nodeNum].leafNodeNum<<endl;
            // for (auto siz:attrSymTab[nodeNum].intParams)
            //     cout<<siz<<" ";
            // cout<<endl;

        }

    }
    else if (nodeType[nodeNum]=="PreIncrementExpression"){
        int c2=adj[nodeNum][1];
        preOrderTraversal(c2);
        attrSymTab[nodeNum].leafNodeNum=attrSymTab[c2].leafNodeNum;
        
    }
    else if (nodeType[nodeNum]=="PreDecrementExpression"){
        int c2=adj[nodeNum][1];
        preOrderTraversal(c2);
        
        attrSymTab[nodeNum].leafNodeNum=attrSymTab[c2].leafNodeNum;
    
    }
    
    else {

        // simple visiting without any action
        for (auto child: adj[nodeNum]){
            preOrderTraversal(child);
        }
        return;
    }
    
}

void printTables(){

    freopen("global_symbol_table.csv","w",stdout);

    // print global table
    cout<<"Name,Type,Class Number,Parent Class Number,Line Number,Local Table Pointer\n";
    for (auto row: globalTable){

        if (row.type!="class"){
            cout<<row.name<<","<<row.type<<",-,-,"<<row.line<<",null\n";
        }
        else{
            cout<<row.name<<","<<row.type<<","<<row.classNum<<","<<row.parentClassNum<<","<<row.line<<","<<row.localTablePointer<<"\n";
        }
 
    }
    std::fclose(stdout);

    //print class tables
    int cc=0;
    for (auto row: globalTable){
        
        if (row.type=="class"){
            int fc=0;
            cc++;
            string cCount=to_string(cc);
            string fname="class_table_";
            fname+=cCount;
            fname+=".csv";
            freopen(fname.c_str(),"w",stdout);

            cout<<"Name,Type,Array Size,Return Type,Function Param Types,Scope,Parent Scope,Offset,Line Number,Function Table Pointer\n";
            for(int j=0;j<(*(row.localTablePointer)).size();j++){
                
                cout<<(*(row.localTablePointer))[j].name<<",";
                cout<<(*(row.localTablePointer))[j].type;
                cout<<",";
                if ((*(row.localTablePointer))[j].arraySize.size()!=0){
                    
                    for (auto siz:(*(row.localTablePointer))[j].arraySize)
                        cout<<siz<<" ";
                    cout<<",";
                }
                else{
                    cout<<"-,";
                }
                    
                cout<<(*(row.localTablePointer))[j].functionReturnType<<",";

                for (auto param:(*(row.localTablePointer))[j].functionParams)
                    cout<<param<<" ";

                cout<<","<<(*(row.localTablePointer))[j].scope.first<<" "<<(*(row.localTablePointer))[j].scope.second ;

                cout<<","<<(*(row.localTablePointer))[j].parentScope.first<<" "<<(*(row.localTablePointer))[j].parentScope.second ;
                if ((*(row.localTablePointer))[j].type=="method")
                    cout<<",-";
                else{
                    cout<<","<<(*(row.localTablePointer))[j].offset;
                }

                cout<<","<<(*(row.localTablePointer))[j].line ;
                if ((*(row.localTablePointer))[j].type=="method")
                    cout<<","<<(*(row.localTablePointer))[j].functionTablePointer<<"\n";
                else
                    cout<<",null\n";

		    }

            std::fclose(stdout);

        }
    }

    // print function tables

    cc=0;
    for (auto row: globalTable){
        
        if (row.type=="class"){
            int fc=0;
            cc++;

            for(int j=0;j<(*(row.localTablePointer)).size();j++){
                
                    if ((*(row.localTablePointer))[j].type=="method"){
                        fc++;
                    
                        vector<localTableParams> funcTable=*((*(row.localTablePointer))[j].functionTablePointer);
                        string ffname="class_"+to_string(cc)+"_function_"+to_string(fc)+"_table.csv";
                        freopen(ffname.c_str(),"w",stdout);

                        cout<<"Name,Type,Array Size,Scope,Parent Scope,Offset,Line Number\n";

                        for (auto fRow:funcTable){
                            cout<<fRow.name<<",";
                            cout<<fRow.type<<",";
                            if (fRow.arraySize.size()!=0){
                    
                                for (auto siz:(fRow.arraySize))
                                    cout<<siz<<" ";
                                cout<<",";
                            }
                            else{
                                cout<<"-,";
                            }
                            cout<<fRow.scope.first<<" "<<fRow.scope.second<<",";
                            cout<<fRow.parentScope.first<<" "<<fRow.parentScope.second<<",";
                            cout<<fRow.offset<<",";
                            cout<<fRow.line<<"\n";
                        }

                        std::fclose(stdout);

                    }
		    }

        }
    }

    // freopen("log.log","w",stdout);

    return;
}

void execCastExpression(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c2 = adj[nodeNum][1];
            int c4 = adj[nodeNum][3];
            attr3AC[nodeNum] = attr3AC[c4];
            attr3AC[nodeNum].type = attr3AC[c2].type;
        }
        break;
    }
    return;
}

void execIntegralType(int nodeNum){
    attr3AC[nodeNum].type = nodeType[adj[nodeNum][0]];
    return;
}

void execWhileStatementNoShortIf(int nodeNum){
    string beg = getLabel(-1,0);
    int c3 = adj[nodeNum][2];
    int c5 = adj[nodeNum][4];
    // cout << "whilestatementnoshortif " << attr3AC[c3].threeAC.size() << endl;
    string c3true = getLabel(c3,1);
    // cout << "B true: " << c3true << endl;
    nextLabel[c5]= getLabelNumber(beg);
    string temp = beg + ":";
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum].assemblyCode.push_back(temp);
    attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c3];
    temp = c3true + ":";
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum].assemblyCode.push_back(temp);
    attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c5];
    temp = "goto " + beg;
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum].assemblyCode.push_back("jmp "+beg);

    falseLabel[c3] = nextLabel[nodeNum];

    return;
}

void execVariableInitializers(int nodeNum){
    return;
}

void execVariableDeclarators(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            // pushLabelUp(nodeNum,c);
        }
        break;
    }
    return;
}

void execVariableDeclaratorId(int nodeNum){
    int c = adj[nodeNum][0];
    attr3AC[nodeNum] = attr3AC[c];
    pushLabelUp(nodeNum,c);
    return;
}

void execVariableDeclarator(int nodeNum){

    
    // start Stack Allocation 3AC

    if (!inFormalParameterList){
        
        int c1=adj[nodeNum][0];
        
        string varType=typeOfNode[to_string(attr3AC[c1].nodeno)];

        if (varType[varType.size()-1] == ';'){
            // type is array // do nothing
        }
        else{
            string temp3AC1="pushonstack " + nodeType[attr3AC[c1].nodeno] ;
            string temp3AC2="stackpointer + " + to_string(typeSize[typeOfNode[to_string(attr3AC[c1].nodeno)]]) ;
            attr3AC[nodeNum].threeAC.push_back(temp3AC1);
            attr3AC[nodeNum].threeAC.push_back(temp3AC2);
        }
        
    }

    if (inForInit){
        int c1=adj[nodeNum][0];

        string temp3AC="stackpointer -"+ to_string(typeSize[typeOfNode[to_string(attr3AC[c1].nodeno)]]) +"\npopfromstack "+nodeType[attr3AC[c1].nodeno];
        ForInitVars[ForInitVars.size()-1].push_back(temp3AC);
        
    }

    // end Stack Allocation 3AC

    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            // start Stack Allocation 3AC
            // doing this again because 3AC gets overwritten on the above line
            if (!inFormalParameterList){
        
                int c1=adj[nodeNum][0];
                
                string temp3AC1="pushonstack " + nodeType[attr3AC[c1].nodeno] ;
                string temp3AC2="stackpointer + " + to_string(typeSize[typeOfNode[to_string(attr3AC[c1].nodeno)]]) ;
                attr3AC[nodeNum].threeAC.push_back(temp3AC1);
                attr3AC[nodeNum].threeAC.push_back(temp3AC2);
            }
            // end Stack Allocation 3AC
            pushLabelUp(nodeNum,c);
            
            tempNum++;
            string temp = "t" + to_string(tempNum);
            varToTemp[nodeType[attr3AC[c].nodeno]] = temp;

        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            string tp = getTypeNode(c);
            if(tp[tp.size()-1]==';'){
                //for array initialization
                // cout << "sfadsfsdafsdafasdfdsfa" << endl;
                string temp = attr3AC[c].addrName + " = popparam";
                attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c];
                attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c3];
                attr3AC[nodeNum].threeAC.push_back(temp);

                //Get GAS code for this line
                tempNum++;
                temp = "t" + to_string(tempNum);
                varToTemp[nodeType[attr3AC[c].nodeno]] = temp;
                // cout << "assigning temp " << nodeType[attr3AC[c].nodeno] << " " << temp << endl;

                int tempOffset = -8*(stoi(temp.substr(1)))-8;
                string temp3 = "movq %rax, " + to_string(tempOffset) + "(%rbp)";
                attr3AC[nodeNum].assemblyCode.push_back(temp3);

                //Add location to freeHeap array so that we can free it in the end
                freeHeap.push_back("movq "+ to_string(tempOffset) + "(%rbp), %rdi");


            }
            else if(tp==insideClassName){
                //for array initialization
                // cout << "sfadsfsdafsdafasdfdsfa" << endl;
                string temp = attr3AC[c].addrName + " = popparam";
                attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c];
                attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c3];
                attr3AC[nodeNum].threeAC.push_back(temp);

                //Get GAS code for this line
                tempNum++;
                temp = "t" + to_string(tempNum);
                varToTemp[nodeType[attr3AC[c].nodeno]] = temp;
                // cout << "assigning temp " << nodeType[attr3AC[c].nodeno] << " " << temp << endl;

                int tempOffset = -8*(stoi(temp.substr(1)))-8;
                string temp3 = "movq %r15, " + to_string(tempOffset) + "(%rbp)";
                attr3AC[nodeNum].assemblyCode.push_back(temp3);

                //Add location to freeHeap array so that we can free it in the end
                // freeHeap.push_back("movq "+ to_string(tempOffset) + "(%rbp), %rdi");


            }
            else{
                string temp = attr3AC[c].addrName + " = " + attr3AC[c3].addrName;
                attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c];
                attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c3];
                // cout << "variabledeclarator " << temp << endl;
                typeOfNode[attr3AC[nodeNum].addrName]=getTypeNode(c3);
                attr3AC[nodeNum].threeAC.push_back(temp);

                // cout << "variabledeclarator me " << attr3AC[c3].addrName << " " << typeOfNode[attr3AC[nodeNum].addrName] << endl;
                attr3AC[nodeNum].addrName = attr3AC[c].addrName;

                //For GAS x86_64
                tempNum++;
                temp = "t" + to_string(tempNum);
                varToTemp[nodeType[attr3AC[c].nodeno]] = temp;
                // cout << "assigning temp " << nodeType[attr3AC[c].nodeno] << " " << temp << endl;

                string arg1 = getArg(attr3AC[c].addrName);
                string arg2 = getArg(attr3AC[c3].addrName);
                // string tempac = "movq " + arg2 + ", " + arg1;
                string tempac1 = "movq " + arg2 + ", %r10";
                string tempac2 = "movq %r10, " + arg1;
                attr3AC[nodeNum].assemblyCode.push_back(tempac1);
                attr3AC[nodeNum].assemblyCode.push_back(tempac2);
                // cout << "assigning temp2 " << nodeType[attr3AC[c].nodeno] << " " << temp << endl;
            }
        }
        break;
    }
    return;
}

void execTypeImportOnDemandDeclaration(int nodeNum){
    return;
}

void execType(int nodeNum){
    int c = adj[nodeNum][0];
    attr3AC[nodeNum] = attr3AC[c];
    // cout<<getTypeNode(nodeNum)<<endl;
    return;
}

void execTryStatement(int nodeNum){
    return;
}

void execThrowStatement(int nodeNum){
    return;
}

void execThrows(int nodeNum){
    return;
}

void execSynchronizedStatement(int nodeNum){
    int c3 = adj[nodeNum][2];
    int c5 = adj[nodeNum][4];
    attr3AC[nodeNum] = attr3AC[c3] + attr3AC[c5];
    return;
}

void execSwitchStatement(int nodeNum){
    // cout << "switch statemnt karna he" << endl;
    int c3 = adj[nodeNum][2];
    int c5 = adj[nodeNum][4];
    attr3AC[nodeNum] = attr3AC[c3] + attr3AC[c5];
    return;
}

void execSwitchLabels(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            // cout << "label switch labels " << getLabel(nodeNum,1) << " " << getLabel(nodeNum,2) << " " << getLabel(c,1) << " " << getLabel(c,2) << endl;
            // for(int i=0;i<attr3AC[c].threeAC.size();i++){
            //     cout << attr3AC[c].threeAC[i] << endl;
            // }
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            int c1 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c] + attr3AC[c1];
        }
        break;
    }
    return;
}

void execSwitchLabel(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c2 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c2];
            string c2true = getLabel(c2,1);
            string c2false =getLabel(c2,2);
            string temp = "if " + switchExpAddr + " == " + attr3AC[c2].addrName + " goto " + c2true;
            // cout << "sdfadfsadf as temp " << temp << endl;
            attr3AC[nodeNum].threeAC.push_back(temp);
            temp = "goto " + c2false;
            attr3AC[nodeNum].threeAC.push_back(temp);
            pushLabelUp(nodeNum,c2);
            // cout << "over here  " << c2true << " " << c2false << endl;
        }
        break;
        case 2:{
            attr3AC[nodeNum].addrName = "default";
        }
        break;
    }
    return;
}

void execSwitchBlockStatementGroups(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            // for(int i=0;i<attr3AC[nodeNum].threeAC.size();i++){
            //     cout << "asdfadsfasdfasdfasf1 " << attr3AC[nodeNum].threeAC[i] << endl;
            // }
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            int c2 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c] + attr3AC[c2];
            // for(int i=0;i<attr3AC[nodeNum].threeAC.size();i++){
            //     cout << "asdfadsfasdfasdfasf2 " << attr3AC[nodeNum].threeAC[i] << endl;
            // }
        }
        break;
    }
    return;
}

void execSwitchBlockStatementGroup(int nodeNum){
    int c = adj[nodeNum][0];
    int c2 = adj[nodeNum][1];
    string ctrue = getLabel(c,1);
    string cfalse = getLabel(c,2);
    string temp = ctrue + ":";
    attr3AC[nodeNum] = attr3AC[c];
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c2];
    temp = cfalse + ":";
    attr3AC[nodeNum].threeAC.push_back(temp);
    // for(int i=0;i<attr3AC[nodeNum].threeAC.size();i++){
    //     cout << "asdfasdf " << attr3AC[nodeNum].threeAC[i] << endl;
    // }
    return;
}

void execSwitchBlock(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:
        break;
        case 2:{
            int c2 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c2];
            pushLabelUp(nodeNum,c2);
            // for(int i=0;i<attr3AC[nodeNum].threeAC.size();i++){
            //     cout << "asdfadsfasdfasdfasf " << attr3AC[nodeNum].threeAC[i] << endl;
            // }
        }
        break;
        case 3:{
            int c2 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c2];
            pushLabelUp(nodeNum,c2);
            // for(int i=0;i<attr3AC[nodeNum].threeAC.size();i++){
            //     cout << "asdfadsfasdfasdfasf " << attr3AC[nodeNum].threeAC[i] << endl;
            // }
        }
        break;
        case 4:{
            int c2 = adj[nodeNum][1];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c2] + attr3AC[c3];
            // for(int i=0;i<attr3AC[nodeNum].threeAC.size();i++){
            //     cout << "asdfadsfasdfasdfasf " << attr3AC[nodeNum].threeAC[i] << endl;
            // }
        }
        break;
    }
    
    return;
}

void execSuper(int nodeNum){
    return;
}

void execStringLiteral(int nodeNum){
    int c = adj[nodeNum][0];
    attr3AC[nodeNum].addrName = nodeType[c]; 
    attr3AC[nodeNum].nodeno = c;
    typeOfNode[to_string(c)] = "string";
    return;
}

void execStaticInitializer(int nodeNum){
    int c2 = adj[nodeNum][1];
    attr3AC[nodeNum] = attr3AC[c2];
    pushLabelUp(nodeNum,c2);
    return;
}

void execStatementExpressionList(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c] + attr3AC[c3];
        }
        break;
    }
    return;
}

void execSingleTypeImportDeclaration(int nodeNum){
    return;
}

void execReturnStatement(int nodeNum){
    switch (prodNum[nodeNum]){
        case 1:{
            int c2 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c2];
            string temp = "RETURN " + attr3AC[c2].addrName;
            string t1 = attr3AC[c2].addrName;
            string arg1;
            if (t1[0] == 't'){
                int of1 = -8*stoi(t1.substr(1))-8;
                arg1 = to_string(of1);
                arg1+= "(%rbp)";
            }
            else if (t1[0]>='0' && t1[0]<='9'){
                arg1 = "$" + t1;
            }
            else{
                t1 = varToTemp[t1];
                int of1 = -8*stoi(t1.substr(1))-8;
                arg1 = to_string(of1);
                arg1+= "(%rbp)";

            }
            attr3AC[nodeNum].threeAC.push_back(temp);
            string tempac1 = "movq " + arg1 + ", %rax";
            string tempac2 = "ret";
            attr3AC[nodeNum].assemblyCode.push_back(tempac1);
            attr3AC[nodeNum].assemblyCode.push_back(tempac2);
            pushLabelUp(nodeNum,c2);
        }
        break;
        case 2:{
            string temp = "RETURN";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back("ret");
        }
        break;
    }
}

void execReferenceType(int nodeNum){
    //Types not required to write 3AC
    return;
}

void execQualifiedName(int nodeNum){
    int c = adj[nodeNum][0];
    int c2 = adj[nodeNum][2];
    attr3AC[nodeNum] = attr3AC[c];
    cout << "in qualified name " << attr3AC[c].threeAC.size() << endl;
    int offset_val = useOffset(adj[c2][0]);
    cout << "in qn " << offset_val << endl;
    // cout << "in qualified name " << getTypeNode(attr3AC[c2].nodeno) << endl;
    if(offset_val==-1)return;
    tempNum++;
    attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
    // typeOfNode[attr3AC[nodeNum].addrName] = typeOfNode[to_string(adj[c2][0])];
    
    attr3AC[nodeNum].threeAC.push_back(attr3AC[nodeNum].addrName +" = "+ to_string(offset_val));

    string temp2 = "*("+ attr3AC[c].addrName + " + " + attr3AC[nodeNum].addrName + ")";
    attr3AC[nodeNum].addrName = temp2;
    typeSize[attr3AC[nodeNum].addrName]=typeSize[attr3AC[c].addrName];

    //offset is present in %r8
    int stackOffset = getStackOffset(attr3AC[nodeNum].addrName);

    attr3AC[nodeNum].assemblyCode.push_back("movq "+to_string(stackOffset)+"(%rbp), %r8");

    // string temp = "*("+"whathere"+" + "++")";
    // attr3AC[nodeNum].addrName = temp;

    return;
}

void execPrimitiveType(int nodeNum){
    attr3AC[nodeNum] = attr3AC[adj[nodeNum][0]];
    // cout << "primtive " << attr3AC[nodeNum].type << endl;
    return;
}

void execPreIncrementExpression(int nodeNum){
    int c = adj[nodeNum][1];
    tempNum++;
    attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
    typeOfNode[attr3AC[nodeNum].addrName] = "int";
    string temp = attr3AC[nodeNum].addrName + " = " + attr3AC[c].addrName + " + 1";
    attr3AC[nodeNum].threeAC.push_back(temp);
    temp = attr3AC[c].addrName + " = " + attr3AC[nodeNum].addrName;
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum] = attr3AC[nodeNum]+ attr3AC[c];

    attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            
    // cout<<attr3AC[nodeNum].addrName<<"\n";

    // string arg2 = varToTemp[attr3AC[c].addrName];
    // if(varToTemp.find(attr3AC[c].addrName)== varToTemp.end()){ arg2 = attr3AC[c].addrName; }
    // if(arg2=="") { arg2 = attr3AC[c].addrName; }

    auto x = getPreandPostIncrementAssemblyCode(varToTemp[attr3AC[c].addrName]);

    for(auto el:x){
        // cout<<el<<"\n";
        attr3AC[nodeNum].assemblyCode.push_back(el);
    }

    // typeSize[attr3AC[nodeNum].addrName]=typeSize[attr3AC[c].addrName];
    pushLabelUp(nodeNum,c);
    return;
}

void execPreDecrementExpression(int nodeNum){
    int c = adj[nodeNum][1];
    tempNum++;
    attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
    typeOfNode[attr3AC[nodeNum].addrName]="int";
    string temp = attr3AC[nodeNum].addrName + " = " + attr3AC[c].addrName + " - 1";
    attr3AC[nodeNum].threeAC.push_back(temp);
    temp = attr3AC[c].addrName + " = " + attr3AC[nodeNum].addrName;
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c];
    // typeSize[attr3AC[nodeNum].addrName]=typeSize[attr3AC[c].addrName];

    attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            
    // cout<<attr3AC[nodeNum].addrName<<"\n";

    // string arg2 = varToTemp[attr3AC[c].addrName];
    // if(varToTemp.find(attr3AC[c].addrName)== varToTemp.end()){ arg2 = attr3AC[c].addrName; }
    // if(arg2=="") { arg2 = attr3AC[c].addrName; }

    auto x = getPreandPostDecrementAssemblyCode(varToTemp[attr3AC[c].addrName]);

    for(auto el:x){
        // cout<<el<<"\n";
        attr3AC[nodeNum].assemblyCode.push_back(el);
    }

    pushLabelUp(nodeNum,c);
    return;
}

void execPostIncrementExpression(int nodeNum){
    int c = adj[nodeNum][0];
    attr3AC[nodeNum] = attr3AC[c];
    tempNum++;
    attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
    typeOfNode[attr3AC[nodeNum].addrName] = "int";
    string temp = attr3AC[nodeNum].addrName + " = " + attr3AC[c].addrName + " + 1";
    attr3AC[nodeNum].threeAC.push_back(temp);
    temp = attr3AC[c].addrName + " = " + attr3AC[nodeNum].addrName;
    attr3AC[nodeNum].threeAC.push_back(temp);

    attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            
    // cout<<attr3AC[nodeNum].addrName<<"\n";

    // string arg2 = varToTemp[attr3AC[c].addrName];
    // if(varToTemp.find(attr3AC[c].addrName)== varToTemp.end()){ arg2 = attr3AC[c].addrName; }
    // if(arg2=="") { arg2 = attr3AC[c].addrName; }

    auto x = getPreandPostIncrementAssemblyCode(varToTemp[attr3AC[c].addrName]);

    for(auto el:x){
        // cout<<el<<"\n";
        attr3AC[nodeNum].assemblyCode.push_back(el);
    }

    pushLabelUp(nodeNum,c);
    return;
}

void execPostDecrementExpression(int nodeNum){
    int c = adj[nodeNum][0];
    attr3AC[nodeNum] = attr3AC[c];
    tempNum++;
    attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
    typeOfNode[attr3AC[nodeNum].addrName] = "int";
    string temp = attr3AC[nodeNum].addrName + " = " + attr3AC[c].addrName + " - 1";
    attr3AC[nodeNum].threeAC.push_back(temp);
    temp = attr3AC[c].addrName + " = " + attr3AC[nodeNum].addrName;
    attr3AC[nodeNum].threeAC.push_back(temp);

    attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            
    // cout<<attr3AC[nodeNum].addrName<<"\n";

    // string arg2 = varToTemp[attr3AC[c].addrName];
    // if(varToTemp.find(attr3AC[c].addrName)== varToTemp.end()){ arg2 = attr3AC[c].addrName; }
    // if(arg2=="") { arg2 = attr3AC[c].addrName; }

    auto x = getPreandPostDecrementAssemblyCode(varToTemp[attr3AC[c].addrName]);

    for(auto el:x){
        // cout<<el<<"\n";
        attr3AC[nodeNum].assemblyCode.push_back(el);
    }

    pushLabelUp(nodeNum,c);
    return;
}

void execPackageDeclaration(int nodeNum){
    return;
}

void execNumericType(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
    }
}

void execNullLiteral(int nodeNum){
    int c = adj[nodeNum][0];
    attr3AC[nodeNum].addrName = nodeType[c];
    return;
}

void execModifiers(int nodeNum){
    return;
}

void execModifier(int nodeNum){
    return;
}

void execMethodHeader_(int nodeNum){
    int c = adj[nodeNum][0];
    attr3AC[nodeNum] = attr3AC[c];
    for(int i=0;i<attr3AC[nodeNum].params.size();i++){
        // cout << "methodheader____ " << attr3AC[nodeNum].params[i] << " " << attr3AC[nodeNum].paramsNodeNo[i] << endl;
    }
    for(int i=attr3AC[nodeNum].params.size()-1;i>=0;i--){
        tempNum++;
        string temp = "t" + to_string(tempNum);
        funcParamTemp[attr3AC[nodeNum].params[i]]=temp;
        string temp2 = temp + " = popparam";
        attr3AC[nodeNum].threeAC.push_back(temp2);
        typeSize[temp] = 8;
        // cout << "in lop " << i << " " << temp2 << endl;

        //Add GAS x86_64 for popping the params
        int tempOffset = -8*(stoi(temp.substr(1)))-8;
        if(i==5){
            string gas = "movq %r9, " + to_string(tempOffset) + "(%rbp)";
            attr3AC[nodeNum].assemblyCode.push_back(gas);
        }else if(i==4){
            string gas = "movq %r8, " + to_string(tempOffset) + "(%rbp)";
            attr3AC[nodeNum].assemblyCode.push_back(gas);
        }else if(i==3){
            string gas = "movq %rcx, " + to_string(tempOffset) + "(%rbp)";
            attr3AC[nodeNum].assemblyCode.push_back(gas);
        }else if(i==2){
            string gas = "movq %rdx, " + to_string(tempOffset) + "(%rbp)";
            attr3AC[nodeNum].assemblyCode.push_back(gas);
        }else if(i==1){
            string gas = "movq %rsi, " + to_string(tempOffset) + "(%rbp)";
            attr3AC[nodeNum].assemblyCode.push_back(gas);            
        }else if(i==0){
            string gas = "movq %rdi, " + to_string(tempOffset) + "(%rbp)";
            attr3AC[nodeNum].assemblyCode.push_back(gas);
        }

    }
    attr3AC[nodeNum].params.clear();
    attr3AC[nodeNum].paramsNodeNo.clear();
    return;
}

void execMethodHeader(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c3];
        }
        break;
        case 2:{
            int c2 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c2];
        }
        break;
        case 3:{
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c3];
        }
        break;
        case 4:{
            int c2 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c2];
        }
        break;
        case 5:{
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c3];
        }
        break;
        case 6:{
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c3];
        }
        break;
        case 7:{
            int c2 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c2];
        }
        break;
        case 8:{
            int c2 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c2];
        }
        break;
    }
    return;
}

void execMethodDeclarator(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c];
            for(int i=0;i<attr3AC[c3].params.size();i++){
                attr3AC[nodeNum].params.push_back(attr3AC[c3].params[i]);
                attr3AC[nodeNum].paramsNodeNo.push_back(attr3AC[c3].paramsNodeNo[i]);
            }
            // for(int i=0;i<attr3AC[nodeNum].params.size();i++){
            //     cout << "methoddeclarator " << attr3AC[nodeNum].params[i] << " " << attr3AC[nodeNum].paramsNodeNo[i] << endl;
            // }
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
        }
        break;
    }
    return;
}

void execLocalVariableDeclarationStatement(int nodeNum){
    int c = adj[nodeNum][0];
    attr3AC[nodeNum] = attr3AC[c];
    pushLabelUp(nodeNum,c);
    return;
}

void execLocalVariableDeclaration(int nodeNum){
    int c = adj[nodeNum][0];
    int c2 = adj[nodeNum][1];
    attr3AC[nodeNum] = attr3AC[c2];
    attr3AC[nodeNum].type = attr3AC[c].type;
    return;
}

void execLiteral(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 3:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 4:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 5:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 6:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 7:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
    }
    return;
}

void execLabeledStatementNoShortIf(int nodeNum){
    int c3 = adj[nodeNum][2];
    int c = adj[nodeNum][0];
    string ctrue = getLabel(c,1);
    string temp = ctrue + ":";
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum].assemblyCode.push_back(temp);
    attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c3];
    pushLabelUp(nodeNum,c3);
    return;
}

void execLabeledStatement(int nodeNum){
    int c3 = adj[nodeNum][2];
    int c = adj[nodeNum][0];
    string ctrue = getLabel(c,1);
    string temp = ctrue + ":";
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum].assemblyCode.push_back(temp);
    attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c3];
    pushLabelUp(nodeNum,c3);
    return;
}

void execInterfaceTypeList(int nodeNum){
    return;
}

void execInterfaceType(int nodeNum){
    return;
}

void execInterfaces(int nodeNum){
    int c2 = adj[nodeNum][1];
    attr3AC[nodeNum] = attr3AC[c2];
    pushLabelUp(nodeNum,c2);
    return;
}

void execInterfaceMemberDeclarations(int nodeNum){
    return;
}

void execInterfaceMemberDeclaration(int nodeNum){
    return;
}

void execInterfaceDeclaration(int nodeNum){
    return;
}

void execInterfaceBody(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:
        break;
    }
    return;
}

void execIntegerLiteral(int nodeNum){
    int c = adj[nodeNum][0];
    // tempNum++;
    // string temp = "t" + to_string(tempNum);
    // string threAC = temp + " = " + nodeType[c];
    // attr3AC[nodeNum].threeAC.push_back(threAC);
    // attr3AC[nodeNum].addrName = temp;
    attr3AC[nodeNum].addrName = nodeType[c];
    attr3AC[nodeNum].nodeno = c;
    typeOfNode[to_string(c)]="int";
    return;
}

void execImportDeclaration(int nodeNum){
    return;
}

void execImportDeclarations(int nodeNum){
    return;
}

void execIfThenElseStatementNoShortIf(int nodeNum){
    int c3 = adj[nodeNum][2];
    int c5 = adj[nodeNum][4];
    int c7 = adj[nodeNum][6];
    // cout << "ifthen else " << attr3AC[c3].threeAC.size() << " " << attr3AC[c5].threeAC.size() << " " << attr3AC[c7].threeAC.size() << endl;
    string c3true = getLabel(c3,1);
    string c3false = getLabel(c3,2);
    attr3AC[nodeNum] = attr3AC[c3];
    // cout << " vasf " << attr3AC[c3].threeAC[0] << endl;
    string temp = c3true + ":";
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum].assemblyCode.push_back(temp);
    attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c5];
    string pnext = getLabel(nodeNum,3);
    temp = "goto " + pnext;
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum].assemblyCode.push_back("jmp "+pnext);
    temp = c3false + ":";
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum].assemblyCode.push_back(temp);
    attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c7];

    return;
}

void execForUpdate(int nodeNum){
    int c = adj[nodeNum][0];
    attr3AC[nodeNum] = attr3AC[c];
    pushLabelUp(nodeNum,c);
    return;
}

void execForStatementNoShortIf(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c3 = adj[nodeNum][2];
            int c5 = adj[nodeNum][4];
            int c7 = adj[nodeNum][6];
            int c9 = adj[nodeNum][8];
            string beg = getLabel(-1,0);
            nextLabel[c9]= getLabelNumber(beg);
            string c5true = getLabel(c5,1);
            attr3AC[nodeNum] = attr3AC[c3];
            string temp = beg + ":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c5];
            temp = c5true + ":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c9];
            attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].assemblyCode.push_back(forContinueLabel + ":");
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c7];
            temp = "goto " + beg;
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back("jmp "+beg);

            falseLabel[c5] = nextLabel[nodeNum];
        }
        break;
        case 2:{
            int c4 = adj[nodeNum][3];
            int c6 = adj[nodeNum][5];
            int c8 = adj[nodeNum][7];
            string beg = getLabel(-1,0);
            nextLabel[c8]= getLabelNumber(beg);
            string c4true = getLabel(c4,1);
            string temp = beg + ":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c4];
            temp = c4true + ":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c8];
            attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].assemblyCode.push_back(forContinueLabel + ":");
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c6];
            temp = "goto " + beg;
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back("jmp "+beg);

            falseLabel[c4] = nextLabel[nodeNum];
        }
        break;
        case 3:{
            int c3 = adj[nodeNum][2];
            int c6 = adj[nodeNum][6];
            int c8 = adj[nodeNum][7];
            string beg = getLabel(-1,0);
            nextLabel[c8]= getLabelNumber(beg);
            attr3AC[nodeNum] = attr3AC[c3];
            string temp = beg + ":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c8];
            attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].assemblyCode.push_back(forContinueLabel + ":");
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c6];
            temp = "goto " + beg;
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back("jmp "+beg);
        }
        break;
        case 4:{
            int c3 = adj[nodeNum][2];
            int c5 = adj[nodeNum][4];
            int c8 = adj[nodeNum][7];
            string beg = getLabel(-1,0);
            nextLabel[c8]= getLabelNumber(beg);
            string c5true = getLabel(c5,1);
            attr3AC[nodeNum] = attr3AC[c3];
            string temp = beg + ":";
            attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].assemblyCode.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c5];
            temp = c5true + ":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c8];
            temp = "goto " + beg;
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back("jmp "+beg);

            falseLabel[c5] = nextLabel[nodeNum];
        }
        break;
        case 5:{
            int c5 = adj[nodeNum][4];
            int c7 = adj[nodeNum][6];
            string beg = getLabel(-1,0);
            nextLabel[c7]= getLabelNumber(beg);
            string temp = beg + ":";
            // attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c7];
            attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].assemblyCode.push_back(forContinueLabel + ":");
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c5];
            temp = "goto " + beg;
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back("jmp "+beg);
        }
        break;
        case 6:{
            int c4 = adj[nodeNum][3];
            int c7 = adj[nodeNum][6];
            string beg = getLabel(-1,0);
            // attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            nextLabel[c7]= getLabelNumber(beg);
            string c4true = getLabel(c4,1);
            string temp = beg + ":";
            attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].assemblyCode.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c4];
            temp = c4true + ":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c7];
            temp = "goto " + beg;
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back("jmp "+beg);

            falseLabel[c4] = nextLabel[nodeNum];
        }
        break;
        case 7:{
            int c3 = adj[nodeNum][2];
            int c7 = adj[nodeNum][6];
            string beg = getLabel(-1,0);
            // attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            nextLabel[c7]= getLabelNumber(beg);
            attr3AC[nodeNum] = attr3AC[c3];
            string temp = beg + ":";
            attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].assemblyCode.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c7];
            temp = "goto " + beg;
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back("jmp "+beg);
        }
        break;
        case 8:{
            int c6 = adj[nodeNum][5];
            string beg = getLabel(-1,0);
            // attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            nextLabel[c6]= getLabelNumber(beg);
            string temp = beg + ":";
            attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].assemblyCode.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c6];
            temp = "goto " + beg;
            attr3AC[nodeNum].threeAC.push_back(temp); 
            attr3AC[nodeNum].assemblyCode.push_back("jmp "+beg);
            // pushLabelUp(nodeNum,c);
        }
        break;
    }
    return;
}

void execFormalParameter(int nodeNum){
    int c = adj[nodeNum][1];
    attr3AC[nodeNum].params.push_back(attr3AC[c].addrName);
    attr3AC[nodeNum].paramsNodeNo.push_back(attr3AC[c].nodeno);
    // cout << "formal parameter " << attr3AC[nodeNum].params[0] << " " << attr3AC[nodeNum].paramsNodeNo[0] << endl;
    return;
}

void execFormalParameterList(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum]=attr3AC[c];
            for(int i=0;i<attr3AC[nodeNum].params.size();i++){
                // cout << "formalparamterlist1 " << attr3AC[nodeNum].params[i] << " " << attr3AC[nodeNum].paramsNodeNo[i] << endl;
            }
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c];
            attr3AC[nodeNum].params.push_back(attr3AC[c3].params[0]);
            attr3AC[nodeNum].paramsNodeNo.push_back(attr3AC[c3].paramsNodeNo[0]);
            for(int i=0;i<attr3AC[nodeNum].params.size();i++){
                // cout << "formalparamterlist " << attr3AC[nodeNum].params[i] << " " << attr3AC[nodeNum].paramsNodeNo[i] << endl;
            }
        }
        break;
    }
    return;
}

void execForInit(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
    }
}

void execFloatingPointType(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum].type = nodeType[c]; 
            attr3AC[nodeNum].nodeno = c;
            typeOfNode[to_string(c)] = "float";
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum].type = nodeType[c]; 
            attr3AC[nodeNum].nodeno = c;
            typeOfNode[to_string(c)] = "double";
        }
        break;

    }
    return;
}

void execFinally(int nodeNum){
    int c = adj[nodeNum][1];
    attr3AC[nodeNum] = attr3AC[c];
    return;
}

void execFloatingPointLiteral(int nodeNum){
    int c = adj[nodeNum][0];
    attr3AC[nodeNum].addrName = nodeType[c]; 
    attr3AC[nodeNum].nodeno = c;
    typeOfNode[to_string(c)] = "float";
    return;
}

void execFieldDeclaration(int nodeNum){
    //declaration, not required for 3ac
    return;
}

void execFieldAccess(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c1 = adj[nodeNum][0];
            int c2 = adj[nodeNum][2];
            if(attr3AC[c1].isthis==1){
                attr3AC[nodeNum].isthis=1;
                // cout<<"this spotted\n";
                tempNum++;
                attr3AC[nodeNum].addrName = "t"+to_string(tempNum); 
                typeOfNode[attr3AC[nodeNum].addrName]= typeOfNode[to_string(adj[c2][0])];
                
                int offset_val = useOffset(adj[c2][0]);
                //have to get offset of classVarName and store in temporary

                attr3AC[nodeNum].threeAC.push_back(attr3AC[nodeNum].addrName + " = "+to_string(offset_val));

                int stackOffset = getStackOffset(attr3AC[nodeNum].addrName);

                attr3AC[nodeNum].assemblyCode.push_back("movq "+to_string(stackOffset)+"(%rbp), %r8");
                
                break;
            }
            else{
                //meant to be empty
            }
        }
        break;
        case 2:{
            
        }
        break;
    }
    return;
}

void execExtendsInterfaces(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            // pushLabelUp(nodeNum,c);
        }
        break;
    }
    return;
}

void execExplicitConstructorInvocation(int nodeNum){
    
    return;
}

void execDoublePointLiteral(int nodeNum){
    int c = adj[nodeNum][0];
    attr3AC[nodeNum].addrName = nodeType[c]; 
    attr3AC[nodeNum].nodeno = c;
    typeOfNode[to_string(c)] = "double";
    return;
}

void execDoStatement(int nodeNum){
    // string beg = getLabel(-1,0);
    int c2 = adj[nodeNum][1];
    int c5 = adj[nodeNum][4];
    string c5true = getLabel(c5,1);
    string c5false = getLabel(c5,2);
    // nextLabel[c2] = getLabelNumber(beg);
    string temp = c5true + ":";
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum].assemblyCode.push_back(temp);
    attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c2];
    attr3AC[nodeNum].threeAC.push_back(doContinueLabel + ":");
    attr3AC[nodeNum].assemblyCode.push_back(doContinueLabel + ":");
    attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c5];
    temp = c5false + ":";
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum].assemblyCode.push_back(temp);
    return;
}

// string beg = getLabel(-1,0);
//     int c3 = adj[nodeNum][2];
//     int c5 = adj[nodeNum][4];
//     // cout << "while " << attr3AC[c3].threeAC.size() << endl;
//     string c3true = getLabel(c3,1);
//     // cout << "B true: " << c3true << endl;
//     nextLabel[c5]= getLabelNumber(beg);
//     string temp = beg + ":";
//     attr3AC[nodeNum].threeAC.push_back(temp);
//     attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c3];
//     temp = c3true + ":";
//     attr3AC[nodeNum].threeAC.push_back(temp);
//     attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c5];
//     temp = "goto " + beg;
//     attr3AC[nodeNum].threeAC.push_back(temp);

//     falseLabel[c3] = nextLabel[nodeNum];

void execContinueStatement(int nodeNum){
    if(isFor){
        attr3AC[nodeNum].threeAC.push_back("goto " + forContinueLabel);
        attr3AC[nodeNum].assemblyCode.push_back("jmp " + forContinueLabel);
    }else if(isWhile){
        attr3AC[nodeNum].threeAC.push_back("goto " + whileContinueLabel);
        attr3AC[nodeNum].assemblyCode.push_back("jmp " + whileContinueLabel);
    }else if(isDo){
        attr3AC[nodeNum].threeAC.push_back("goto " + doContinueLabel);
        attr3AC[nodeNum].assemblyCode.push_back("jmp " + doContinueLabel);
    }else{
        cout << "ERROR: Continue Statement must occur within a loop." << endl;
    }
    return;
}

void execConstructorDeclarator(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            int c2 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c2];
            for(int i=attr3AC[nodeNum].params.size()-1;i>=0;i--){
                tempNum++;
                string temp = "t" + to_string(tempNum);
                funcParamTemp[attr3AC[nodeNum].params[i]]=temp;
                // cout << "over here " << attr3AC[nodeNum].params.size()<<attr3AC[nodeNum].params[i] << endl; 
                string temp2 = temp + " = popparam";
                attr3AC[nodeNum].threeAC.push_back(temp2);

                int stackOffset = -8*(stoi(temp.substr(1)))-8;
                if(i==5){
                    string gas = "movq %r9, "+to_string(stackOffset)+"(%rbp)";
                    attr3AC[nodeNum].assemblyCode.push_back(gas);
                }
                if(i==4){
                    string gas = "movq %r8, "+to_string(stackOffset)+"(%rbp)";
                    attr3AC[nodeNum].assemblyCode.push_back(gas);
                }
                if(i==3){
                    string gas = "movq %rcx, "+to_string(stackOffset)+"(%rbp)";
                    attr3AC[nodeNum].assemblyCode.push_back(gas);
                }
                if(i==2){
                    string gas = "movq %rdx, "+to_string(stackOffset)+"(%rbp)";
                    attr3AC[nodeNum].assemblyCode.push_back(gas);
                }
                if(i==1){
                    string gas = "movq %rsi, "+to_string(stackOffset)+"(%rbp)";
                    attr3AC[nodeNum].assemblyCode.push_back(gas);
                }
                if(i==0){
                    string gas = "movq %rdi, "+to_string(stackOffset)+"(%rbp)";
                    attr3AC[nodeNum].assemblyCode.push_back(gas);
                }
            }
            tempNum++;
            string temp = "t" + to_string(tempNum);
            funcParamTemp["this"]=temp;
            string temp2 = temp + " = popparam";
            attr3AC[nodeNum].threeAC.push_back(temp2);
            
            int stackOffset = -8*(stoi(temp.substr(1)))-8;

            string gas = "movq %r15, "+to_string(stackOffset)+"(%rbp)";
            attr3AC[nodeNum].assemblyCode.push_back(gas);

            // tempNum++;

            attr3AC[nodeNum].params.clear();
            attr3AC[nodeNum].paramsNodeNo.clear();
            break;
        }
        case 2:{
            tempNum++;
            string temp = "t" + to_string(tempNum);
            funcParamTemp["this"]=temp;

            string temp2 = temp + " = popparam";

            attr3AC[nodeNum].threeAC.push_back(temp2);
            
            int stackOffset = -8*(stoi(temp.substr(1)))-8;

            string gas = "movq %r15, "+to_string(stackOffset)+"(%rbp)";
            attr3AC[nodeNum].assemblyCode.push_back(gas);

            attr3AC[nodeNum].params.clear();
            attr3AC[nodeNum].paramsNodeNo.clear();
            break;
        }
    }
    return;
}

void execConstructorDeclaration(int nodeNum){
    string temp = insideClassName +"_ctor:";
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum].assemblyCode.push_back(temp);

    switch(prodNum[nodeNum]){
        case 1:{
            int c1=adj[nodeNum][1], c2=adj[nodeNum][3];
            // attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c1];
            attr3AC[nodeNum] =  attr3AC[nodeNum] + attr3AC[c2];
            break;
        }
        case 2:{
            int c1=adj[nodeNum][0], c2=adj[nodeNum][2];
            // attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c1];
            attr3AC[nodeNum] =  attr3AC[nodeNum] + attr3AC[c2];
            break;
        }
        case 3:{
            int c1=adj[nodeNum][0], c2=adj[nodeNum][1];
            // attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c1];
            attr3AC[nodeNum] =  attr3AC[nodeNum] + attr3AC[c2];
            break;
        }
        case 4:{
            int c1=adj[nodeNum][1], c2=adj[nodeNum][2];
            // attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c1];
            attr3AC[nodeNum] =  attr3AC[nodeNum] + attr3AC[c2];
            break;
        }
    }

    funcParamTemp.clear();
    return;
}

void execConstructorBody(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c2 = adj[nodeNum][1];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c2]+attr3AC[c3];
        }
        break;
        case 2:{
            int c2 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c2];
        }
        break;
        case 3:{
            int c2 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c2];
        }
        break;
        case 4:
        break;
    }
    return;
}

void execConstantExpression(int nodeNum){
    int c = adj[nodeNum][0];
    attr3AC[nodeNum] = attr3AC[c];
    pushLabelUp(nodeNum,c);
    return;
}

void execConstantDeclaration(int nodeNum){
    int c = adj[nodeNum][0];
    attr3AC[nodeNum] = attr3AC[c];
    pushLabelUp(nodeNum,c);
}

void execClassTypeList(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c] + attr3AC[c3];
        }
        break;
    }
    return;
}

void execClassOrInterfaceType(int nodeNum){
    int c = adj[nodeNum][0];
    attr3AC[nodeNum] = attr3AC[c];
    pushLabelUp(nodeNum,c);
    return;
}

void execClassInstanceCreationExpression(int nodeNum){
    switch(prodNum[nodeNum]){
        //push this and argumentlist
        case 1:{//nno arguments, only "this"
            int c = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c];
            
            string oldsp = "pushonstack oldstackpointer";
            attr3AC[nodeNum].threeAC.push_back(oldsp);
            oldsp = "stackpointer + 8";
            attr3AC[nodeNum].threeAC.push_back(oldsp);
            oldsp = "oldstackpointer = stackpointer";
            attr3AC[nodeNum].threeAC.push_back(oldsp);

            int size_class = getClassSize(insideClassName);

            // cout << "over here" << endl;
            tempNum++;
            string temp = "t"+to_string(tempNum)+" = "+to_string(size_class);
            attr3AC[nodeNum].threeAC.push_back(temp);

            temp = "pushparam t"+to_string(tempNum);
            attr3AC[nodeNum].threeAC.push_back(temp);

            //get this reference and push
            temp = "stackpointer + 4";            
            attr3AC[nodeNum].threeAC.push_back(temp);

            temp = "call allocmem 1";
            attr3AC[nodeNum].threeAC.push_back(temp);

            temp = "stackpointer - 4";
            attr3AC[nodeNum].threeAC.push_back(temp);

            tempNum++;
            temp = "t"+to_string(tempNum)+" = popparam";
            attr3AC[nodeNum].threeAC.push_back(temp);

            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);

            temp = "pushparam " + attr3AC[nodeNum].addrName;
            attr3AC[nodeNum].threeAC.push_back(temp);
            // cout << "safadsf " << endl;
            // vector<int> p;
            // checkFunctionParameterTypes(attr3AC[c].nodeno, p);
            // cout << "here" << endl;
            string spointer = "stackpointer + " + to_string(size_class);
            attr3AC[nodeNum].threeAC.push_back(spointer);
            temp = "call " + insideClassName + ".ctor , " + to_string(1);
            attr3AC[nodeNum].threeAC.push_back(temp);
            temp = attr3AC[nodeNum].addrName + " = popparam";
            attr3AC[nodeNum].threeAC.push_back(temp);
            oldsp = "stackpointer = oldstackpointer";
            attr3AC[nodeNum].threeAC.push_back(oldsp);
            oldsp = "stackpointer - 8";
            attr3AC[nodeNum].threeAC.push_back(oldsp);
            oldsp = "oldstackpointer = popfromstack";
            attr3AC[nodeNum].threeAC.push_back(oldsp);            
            // cout << "okay" << endl;
            pushLabelUp(nodeNum,c);
            break;
        }
        case 2:{
            int c2 = adj[nodeNum][1];
            int c4 = adj[nodeNum][3];
            attr3AC[nodeNum] = attr3AC[c2] + attr3AC[c4];   
            
            string oldsp = "pushonstack oldstackpointer";
            attr3AC[nodeNum].threeAC.push_back(oldsp);
            oldsp = "stackpointer + 8";
            attr3AC[nodeNum].threeAC.push_back(oldsp);
            oldsp = "oldstackpointer = stackpointer";
            attr3AC[nodeNum].threeAC.push_back(oldsp);

            int size_class = typeSize[insideClassName];


            tempNum++;
            string temp = "t"+to_string(tempNum)+" = "+to_string(size_class);
            attr3AC[nodeNum].threeAC.push_back(temp);

            temp = "pushparam t"+to_string(tempNum);
            attr3AC[nodeNum].threeAC.push_back(temp);

            //get this reference and push
            temp = "stackpointer + 4";            
            attr3AC[nodeNum].threeAC.push_back(temp);

            temp = "call allocmem 1";
            attr3AC[nodeNum].threeAC.push_back(temp);

            temp = "stackpointer - 4";
            attr3AC[nodeNum].threeAC.push_back(temp);

            tempNum++;
            temp = "t"+to_string(tempNum)+" = popparam";
            attr3AC[nodeNum].threeAC.push_back(temp);

            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);

            temp = "pushparam " + attr3AC[nodeNum].addrName;
            attr3AC[nodeNum].threeAC.push_back(temp);

            for(int fcall=0; fcall<(attr3AC[c4].params).size();fcall++){
                string temp = "pushparam " + (attr3AC[c4].params)[fcall];
                attr3AC[nodeNum].threeAC.push_back(temp);
            }
            // checkFunctionParameterTypes(attr3AC[c2].nodeno, attr3AC[c4].paramsNodeNo);

            string spointer = "stackpointer + " + to_string(size_class);
            attr3AC[nodeNum].threeAC.push_back(spointer);
            temp = "call " + insideClassName + ".ctor , " + to_string(attr3AC[c4].params.size()+1);
            attr3AC[nodeNum].threeAC.push_back(temp);
            temp = attr3AC[nodeNum].addrName + " = popparam";
            attr3AC[nodeNum].threeAC.push_back(temp);
            oldsp = "stackpointer = oldstackpointer";
            attr3AC[nodeNum].threeAC.push_back(oldsp);
            oldsp = "stackpointer - 8";
            attr3AC[nodeNum].threeAC.push_back(oldsp);
            oldsp = "oldstackpointer = popfromstack";
            attr3AC[nodeNum].threeAC.push_back(oldsp);   

            string temp3 = "movq $" + to_string(size_class) + ", %rdi";
            attr3AC[nodeNum].assemblyCode.push_back(temp3);
            //call the sbrk syscall
            temp3 = "movq $12, %rax";
            attr3AC[nodeNum].assemblyCode.push_back(temp3);
            temp3 = "syscall";
            attr3AC[nodeNum].assemblyCode.push_back(temp3);         

            temp3 = "movq %rax, %r15"; //this reference stored in %r15
            attr3AC[nodeNum].assemblyCode.push_back(temp3);         

            for(int fcall=0; fcall<(attr3AC[c4].params).size();fcall++){
                if(fcall==0){
                    string p = attr3AC[c4].params[fcall];
                    string ass = addFuncParamsToReg(p,"%rdi",c4,fcall);
                    attr3AC[nodeNum].assemblyCode.push_back(ass);
                }else if(fcall==1){
                    string p = attr3AC[c4].params[fcall];
                    string ass = addFuncParamsToReg(p,"%rsi",c4,fcall);
                    attr3AC[nodeNum].assemblyCode.push_back(ass);
                }else if(fcall==2){
                    string p = attr3AC[c4].params[fcall];
                    string ass = addFuncParamsToReg(p,"%rdx",c4,fcall);
                    attr3AC[nodeNum].assemblyCode.push_back(ass);
                }else if(fcall==3){
                    string p = attr3AC[c4].params[fcall];
                    string ass = addFuncParamsToReg(p,"%rcx",c4,fcall);
                    attr3AC[nodeNum].assemblyCode.push_back(ass);
                }else if(fcall==4){
                    string p = attr3AC[c4].params[fcall];
                    string ass = addFuncParamsToReg(p,"%r8",c4,fcall);
                    attr3AC[nodeNum].assemblyCode.push_back(ass);
                }else if(fcall==5){
                    string p = attr3AC[c4].params[fcall];
                    string ass = addFuncParamsToReg(p,"%r9",c4,fcall);
                    attr3AC[nodeNum].assemblyCode.push_back(ass);
                }else{
                    cout << "Too many func arguments" << endl;
                    exit(0);
                }
            }
            
            temp3 = "call " + insideClassName + "_ctor";
            attr3AC[nodeNum].assemblyCode.push_back(temp3);

            pushLabelUp(nodeNum,c2);
            break;
        }
    }
    return;
}

void execCharacterLiteral(int nodeNum){
    int c = adj[nodeNum][0];
            attr3AC[nodeNum].addrName = nodeType[c]; 
            attr3AC[nodeNum].nodeno = c;
            typeOfNode[to_string(c)] = "char";
    return;
}

void execCatches(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            int c3 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
        }
        break;
    }
    return;
}

void execCatchClause(int nodeNum){
    int c3 = adj[nodeNum][2];
    int c5 = adj[nodeNum][3];
    attr3AC[nodeNum] = attr3AC[c3];
    string c3true = getLabel(c3,1);
    string c3false = getLabel(c3,2);
    string temp = "if " + attr3AC[c3].addrName + " goto " + c3true;
    attr3AC[nodeNum].threeAC.push_back(temp);
    temp = "goto " + c3false;
    attr3AC[nodeNum].threeAC.push_back(temp);
    temp = c3true + ": ";
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c3];
    temp = c3false+":";
    return;
}

void execBreakStatement(int nodeNum){
    if(isFor){
        string temp = "goto " + forBreakLabel;
        attr3AC[nodeNum].threeAC.push_back(temp);
        attr3AC[nodeNum].assemblyCode.push_back("jmp " + forBreakLabel);
    }else if(isWhile){
        string temp = "goto " + whileBreakLabel;
        attr3AC[nodeNum].threeAC.push_back(temp);
        attr3AC[nodeNum].assemblyCode.push_back("jmp " + whileBreakLabel);
    }else if(isDo){
        string temp = "goto " + doBreakLabel;
        attr3AC[nodeNum].threeAC.push_back(temp);
        attr3AC[nodeNum].assemblyCode.push_back("jmp " + doBreakLabel);
    }
    return;
}

void execArrayType(int nodeNum){
    return;
}

void execBooleanLiteral(int nodeNum){
    int c = adj[nodeNum][0];
    attr3AC[nodeNum].type = nodeType[c]; 
    attr3AC[nodeNum].nodeno = c;
    typeOfNode[to_string(c)] = "boolean";
    // string ptrue = getLabel(nodeNum,1);
    // string pfalse = getLabel(nodeNum,2);
    // if(nodeType[nodeNum]=="true"){
    //     string temp = "goto " + ptrue;
    //     attr3AC[nodeNum].threeAC.push_back(temp);
    // }else{
    //     string temp = "goto " + pfalse;
    //     attr3AC[nodeNum].threeAC.push_back(temp);
    // }
    return;
}

void execAssignmentOperator(int nodeNum){
    return;
}

void execArrayInitializer(int nodeNum){
    //This kindof declaration 
    return;
}

void execAbstractMethodDeclaration(int nodeNum){
    int c = adj[nodeNum][2];
    attr3AC[nodeNum] = attr3AC[c];
    pushLabelUp(nodeNum,c);
    return;
}

void execCompilationUnit(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            int c2 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c] + attr3AC[c2];
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            int c2 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c] + attr3AC[c2];
        }
        break;
        case 3:{
            int c = adj[nodeNum][0];
            int c2 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c] + attr3AC[c2];
        }
        break;
        case 4:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 5:{
            int c = adj[nodeNum][0];
            int c2 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 6:{
            int c = adj[nodeNum][0];
            int c2 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 7:{
            int c = adj[nodeNum][0];
            int c2 = adj[nodeNum][1];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c] + attr3AC[c2];
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c3];
        }
        break;
    }
    return;
}

void execTypeDeclarations(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            int c2 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c] + attr3AC[c2];
        }
        break;
    }
    return;
}

void execTypeDeclaration(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            // attr3AC[nodeNum].nodeno = nodeNum;
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            // attr3AC[nodeNum].nodeno = nodeNum;
            pushLabelUp(nodeNum,c);
        }
        break;
        case 3:
        break;
    }
    return;
}

void execClassDeclaration(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][5];
            attr3AC[nodeNum] = attr3AC[c];
            // attr3AC[nodeNum].nodeno = nodeNum;
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:{
            int c = adj[nodeNum][4];
            attr3AC[nodeNum] = attr3AC[c];
            // attr3AC[nodeNum].nodeno = nodeNum;
            pushLabelUp(nodeNum,c);
        }
        break;
        case 3:{
            int c = adj[nodeNum][3];
            attr3AC[nodeNum] = attr3AC[c];
            // attr3AC[nodeNum].nodeno = nodeNum;
            pushLabelUp(nodeNum,c);
        }
        break;
        case 4:{
            int c = adj[nodeNum][3];
            attr3AC[nodeNum] = attr3AC[c];
            // attr3AC[nodeNum].nodeno = nodeNum;
            pushLabelUp(nodeNum,c);
        }
        break;
        case 5:{
            int c = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c];
            // attr3AC[nodeNum].nodeno = nodeNum;
            pushLabelUp(nodeNum,c);
        }
        break;
        case 6:{
            int c = adj[nodeNum][4];
            attr3AC[nodeNum] = attr3AC[c];
            // attr3AC[nodeNum].nodeno = nodeNum;
            pushLabelUp(nodeNum,c);
        }
        break;
        case 7:{
            int c = adj[nodeNum][4];
            attr3AC[nodeNum] = attr3AC[c];
            // attr3AC[nodeNum].nodeno = nodeNum;
            pushLabelUp(nodeNum,c);
        }
        break;
        case 8:{
            int c = adj[nodeNum][3];
            attr3AC[nodeNum] = attr3AC[c];
            // attr3AC[nodeNum].nodeno = nodeNum;
            pushLabelUp(nodeNum,c);
        }
        break;
    }
    return;
}

void execClassBody(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:
        break;
    }
    return;
}

void execClassBodyDeclarations(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            int c2 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c] + attr3AC[c2];
        }
        break;
    }
    return;
}

void execClassBodyDeclaration(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 3:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
    }
    return;
}

void execClassMemberDeclaration(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
    }
    return;
}

void execMethodDeclaration(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            int c2 = adj[nodeNum][1];
            // cout << "inside method declaration " << insideClassName << endl;
            string temp;
            if(nodeType[attr3AC[c].nodeno]=="main"){
                temp = nodeType[attr3AC[c].nodeno] +":";
            }else{
                temp = nodeType[attr3AC[c].nodeno] + insideClassName +":";
            }
            attr3AC[nodeNum].threeAC.push_back(temp);
            //Add function label for GAS x86_64
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c];
            attr3AC[nodeNum] =  attr3AC[nodeNum] + attr3AC[c2];
            attr3AC[nodeNum].nodeno =  attr3AC[c].nodeno;
            // cout<<nodeType[attr3AC[c].nodeno]<<endl;
            // cout<<scopeAndTable[attr3AC[c].nodeno].second<<endl;
        }
        break;
    }
    return;
}

void execMethodBody(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:
        break;
    }
    return;
}

void execBlock(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:
        break;
        case 2:{
            int c = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
    }
    return;
}

void execBlockStatements(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            int c2 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c] + attr3AC[c2];
        }
        break;
    }
    return;
}

void execBlockStatement(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
    }
    return;
}

void execStatement(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            // cout << "statement " << attr3AC[nodeNum].threeAC.size() << endl;
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 3:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            string cNext = getLabel(c,3);
            string temp = cNext +":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            // start Stack Allocation 3AC
            attr3AC[nodeNum].threeAC.push_back("stackpointer = oldstackpointer");
            attr3AC[nodeNum].threeAC.push_back("stackpointer -8");
            attr3AC[nodeNum].threeAC.push_back("oldstackpointer = popfromstack");
            // end Stack Allocation 3AC
            pushLabelUp(nodeNum,c);
        }
        break;
        case 4:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            string cNext = getLabel(c,3);
            string temp = cNext +":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            // start Stack Allocation 3AC
            attr3AC[nodeNum].threeAC.push_back("stackpointer = oldstackpointer");
            attr3AC[nodeNum].threeAC.push_back("stackpointer -8");
            attr3AC[nodeNum].threeAC.push_back("oldstackpointer = popfromstack");
            // end Stack Allocation 3AC
            pushLabelUp(nodeNum,c);
        }
        break;
        case 5:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            string cNext = getLabel(c,3);
            string temp = cNext +":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            // start Stack Allocation 3AC
            attr3AC[nodeNum].threeAC.push_back("stackpointer = oldstackpointer");
            attr3AC[nodeNum].threeAC.push_back("stackpointer -8");
            attr3AC[nodeNum].threeAC.push_back("oldstackpointer = popfromstack");
            // end Stack Allocation 3AC
            pushLabelUp(nodeNum,c);
        }
        break;
        case 6:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            string cNext = getLabel(c,3);
            string temp = cNext +":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            // start Stack Allocation 3AC
            
            if(prodNum[c]==1 || prodNum[c]==3 || prodNum[c]==4 || prodNum[c]== 7){
                for (int i=ForInitVars[ForInitVars.size()-1].size()-1;i>=0;i--){
                    attr3AC[nodeNum].threeAC.push_back(ForInitVars[ForInitVars.size()-1][i]);
                }

                ForInitVars.pop_back();
            }
            // cout << "statement" << endl;
            // end Stack Allocation 3AC
            pushLabelUp(nodeNum,c);
        }
        break;
    }
    return;
}

void execStatementNoShortIf(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            // cout << "statementNoshort if " << attr3AC[nodeNum].threeAC.size() << endl;;
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 3:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            string cNext = getLabel(c,3);
            string temp = cNext +":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            pushLabelUp(nodeNum,c);
        }
        break;
        case 4:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            string cNext = getLabel(c,3);
            string temp = cNext +":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            pushLabelUp(nodeNum,c);
        }
        break;
        case 5:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            string cNext = getLabel(c,3);
            string temp = cNext +":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            pushLabelUp(nodeNum,c);
        }
        break;
    }
    return;
}

void execIfThenElseStatement(int nodeNum){
    int c3 = adj[nodeNum][2];
    int c5 = adj[nodeNum][4];
    int c7 = adj[nodeNum][6];
    // cout << "ifthen else " << attr3AC[c3].threeAC.size() << " " << attr3AC[c5].threeAC.size() << " " << attr3AC[c7].threeAC.size() << endl;
    string c3true = getLabel(c3,1);
    string c3false = getLabel(c3,2);
    attr3AC[nodeNum] = attr3AC[c3];
    // cout << " vasf " << attr3AC[c3].threeAC[0] << endl;
    string temp = c3true + ":";
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum].assemblyCode.push_back(temp);
    // start Stack Allocation 3AC
            
    attr3AC[nodeNum].threeAC.push_back("pushonstack oldstackpointer");
    attr3AC[nodeNum].threeAC.push_back("stackpointer +8");
    attr3AC[nodeNum].threeAC.push_back("oldstackpointer = stackpointer");
            
    // end Stack Allocation 3AC
    attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c5];
    string pnext = getLabel(nodeNum,3);
    temp = "goto " + pnext;
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum].assemblyCode.push_back("jmp "+pnext);
    temp = c3false + ":";
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum].assemblyCode.push_back(temp);
        // start Stack Allocation 3AC
            
    attr3AC[nodeNum].threeAC.push_back("pushonstack oldstackpointer");
    attr3AC[nodeNum].threeAC.push_back("stackpointer +8");
    attr3AC[nodeNum].threeAC.push_back("oldstackpointer = stackpointer");
            
    // end Stack Allocation 3AC

    attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c7];

    return;
}

void execIfThenStatement(int nodeNum){
    int c3 = adj[nodeNum][2];
    int c5 = adj[nodeNum][4];
    string c3true = getLabel(c3,1);
    attr3AC[nodeNum] = attr3AC[nodeNum]+attr3AC[c3];
    string temp = c3true + ":";
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum].assemblyCode.push_back(temp);
    attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c5];

    // start Stack Allocation 3AC

            // need to pop vars declared in this scope
            // vector<localTableParams>* locTable=scopeAndTable[nodeNum].second;
            // pair<int,int> varsScope=scopeAndTable[nodeNum].first;

            // for (int i=(*locTable).size()-1;i>=0;i--){

            //     if ((*locTable)[i].arraySize.size()==0 && varsScope==(*locTable)[i].scope){
            //         // var other than array
            //         string temp3AC1="stackpointer -" + to_string(typeSize[(*locTable)[i].type]) ;
            //         string temp3AC2="popfromstack " +  (*locTable)[i].name;

            //     attr3AC[nodeNum].threeAC.push_back(temp3AC1);
            //     attr3AC[nodeNum].threeAC.push_back(temp3AC2);

            //     }
            // }
    // end Stack Allocation 3AC

    return;
}

void execForStatement(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c3 = adj[nodeNum][2];
            int c5 = adj[nodeNum][4];
            int c7 = adj[nodeNum][6];
            int c9 = adj[nodeNum][8];
            string beg = getLabel(-1,0);
            nextLabel[c9]= getLabelNumber(beg);
            string c5true = getLabel(c5,1);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c3];
            string temp = beg + ":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c5];
            temp = c5true + ":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            
            // start Stack Allocation 3AC
            
            attr3AC[nodeNum].threeAC.push_back("pushonstack oldstackpointer");
            attr3AC[nodeNum].threeAC.push_back("stackpointer +8");
            attr3AC[nodeNum].threeAC.push_back("oldstackpointer = stackpointer");
            
            // end Stack Allocation 3AC

            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c9];
            attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].assemblyCode.push_back(forContinueLabel + ":");
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c7];
             
            // start Stack Allocation 3AC

            // need to pop vars declared in this scope
            // vector<localTableParams>* locTable=scopeAndTable[nodeNum].second;
            // pair<int,int> varsScope=scopeAndTable[nodeNum].first;

            // for (int i=(*locTable).size()-1;i>=0;i--){

            //     if ((*locTable)[i].arraySize.size()==0 && varsScope==(*locTable)[i].scope){
            //         // var other than array
            //         string temp3AC1="stackpointer -" + to_string(typeSize[(*locTable)[i].type]) ;
            //         string temp3AC2="popfromstack " +  (*locTable)[i].name;

            //     attr3AC[nodeNum].threeAC.push_back(temp3AC1);
            //     attr3AC[nodeNum].threeAC.push_back(temp3AC2);

            //     }
            // }
            attr3AC[nodeNum].threeAC.push_back("stackpointer = oldstackpointer");
            attr3AC[nodeNum].threeAC.push_back("stackpointer -8");
            attr3AC[nodeNum].threeAC.push_back("oldstackpointer = popfromstack");
            // end Stack Allocation 3AC
            temp = "goto " + beg;
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back("jmp "+beg);

            falseLabel[c5] = nextLabel[nodeNum];
        }
        break;
        case 2:{
            int c4 = adj[nodeNum][3];
            int c6 = adj[nodeNum][5];
            int c8 = adj[nodeNum][7];
            string beg = getLabel(-1,0);
            nextLabel[c8]= getLabelNumber(beg);
            string c4true = getLabel(c4,1);
            string temp = beg + ":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c4];
            temp = c4true + ":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c8];
            attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].assemblyCode.push_back(forContinueLabel + ":");
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c6];
            temp = "goto " + beg;
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back("jmp "+beg);
            falseLabel[c4] = nextLabel[nodeNum];
        }
        break;
        case 3:{
            int c3 = adj[nodeNum][2];
            int c6 = adj[nodeNum][6];
            int c8 = adj[nodeNum][7];
            string beg = getLabel(-1,0);
            nextLabel[c8]= getLabelNumber(beg);
            attr3AC[nodeNum] = attr3AC[nodeNum]+attr3AC[c3];
            string temp = beg + ":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c8];
            attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].assemblyCode.push_back(forContinueLabel + ":");
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c6];
            temp = "goto " + beg;
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back("jmp "+beg);
        }
        break;
        case 4:{
            int c3 = adj[nodeNum][2];
            int c5 = adj[nodeNum][4];
            int c8 = adj[nodeNum][7];
            string beg = getLabel(-1,0);
            nextLabel[c8]= getLabelNumber(beg);
            string c5true = getLabel(c5,1);
            // attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            attr3AC[nodeNum] = attr3AC[nodeNum]+attr3AC[c3];
            string temp = beg + ":";
            attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].assemblyCode.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c5];
            temp = c5true + ":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c8];
            temp = "goto " + beg;
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back("jmp "+beg);

            falseLabel[c5] = nextLabel[nodeNum];
        }
        break;
        case 5:{
            int c5 = adj[nodeNum][4];
            int c7 = adj[nodeNum][6];
            string beg = getLabel(-1,0);
            nextLabel[c7]= getLabelNumber(beg);
            string temp = beg + ":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c7];
            attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].assemblyCode.push_back(forContinueLabel + ":");
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c5];
            temp = "goto " + beg;
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back("jmp "+beg);
        }
        break;
        case 6:{
            int c4 = adj[nodeNum][3];
            int c7 = adj[nodeNum][6];
            string beg = getLabel(-1,0);
            nextLabel[c7]= getLabelNumber(beg);
            // attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            string c4true = getLabel(c4,1);
            string temp = beg + ":";
            attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].assemblyCode.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c4];
            temp = c4true + ":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c7];
            temp = "goto " + beg;
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back("jmp "+beg);

            falseLabel[c4] = nextLabel[nodeNum];
        }
        break;
        case 7:{
            int c3 = adj[nodeNum][2];
            int c7 = adj[nodeNum][6];
            string beg = getLabel(-1,0);
            nextLabel[c7]= getLabelNumber(beg);
            // attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            attr3AC[nodeNum] = attr3AC[nodeNum]+attr3AC[c3];
            string temp = beg + ":";
            attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].assemblyCode.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c7];
            temp = "goto " + beg;
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back("jmp "+beg);
        }
        break;
        case 8:{
            int c6 = adj[nodeNum][5];
            string beg = getLabel(-1,0);
            nextLabel[c6]= getLabelNumber(beg);
            // attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            string temp = beg + ":";
            attr3AC[nodeNum].threeAC.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].assemblyCode.push_back(forContinueLabel + ":");
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c6];
            temp = "goto " + beg;
            attr3AC[nodeNum].threeAC.push_back(temp); 
            attr3AC[nodeNum].assemblyCode.push_back("jmp "+beg);
            // pushLabelUp(nodeNum,c);
        }
        break;
    }
    return;
}

void execWhileStatement(int nodeNum){
    string beg = getLabel(-1,0);
    int c3 = adj[nodeNum][2];
    int c5 = adj[nodeNum][4];
    // cout << "while " << attr3AC[c3].threeAC.size() << endl;
    string c3true = getLabel(c3,1);
    // cout << "B true: " << c3true << endl;
    nextLabel[c5]= getLabelNumber(beg);
    string temp = beg + ":";
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum].assemblyCode.push_back(temp);
    // start Stack Allocation 3AC
    
    attr3AC[nodeNum].threeAC.push_back("pushonstack oldstackpointer");
    attr3AC[nodeNum].threeAC.push_back("stackpointer +8");
    attr3AC[nodeNum].threeAC.push_back("oldstackpointer = stackpointer");
    
    // end Stack Allocation 3AC

    attr3AC[nodeNum].threeAC.push_back(whileContinueLabel + ":");
    attr3AC[nodeNum].assemblyCode.push_back(whileContinueLabel + ":");
    attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c3];
    temp = c3true + ":";
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum].assemblyCode.push_back(temp);
    attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c5];
     // start Stack Allocation 3AC

            // need to pop vars declared in this scope
            // vector<localTableParams>* locTable=scopeAndTable[nodeNum].second;
            // pair<int,int> varsScope=scopeAndTable[nodeNum].first;

            // for (int i=(*locTable).size()-1;i>=0;i--){

            //     if ((*locTable)[i].arraySize.size()==0 && varsScope==(*locTable)[i].scope){
            //         // var other than array
            //         string temp3AC1="stackpointer -" + to_string(typeSize[(*locTable)[i].type]) ;
            //         string temp3AC2="popfromstack " +  (*locTable)[i].name;

            //     attr3AC[nodeNum].threeAC.push_back(temp3AC1);
            //     attr3AC[nodeNum].threeAC.push_back(temp3AC2);

            //     }
            // }
            attr3AC[nodeNum].threeAC.push_back("stackpointer = oldstackpointer");
            attr3AC[nodeNum].threeAC.push_back("stackpointer -8");
            attr3AC[nodeNum].threeAC.push_back("oldstackpointer = popfromstack");
            // end Stack Allocation 3AC
    temp = "goto " + beg;
    attr3AC[nodeNum].threeAC.push_back(temp);
    attr3AC[nodeNum].assemblyCode.push_back("jmp "+beg);

    falseLabel[c3] = nextLabel[nodeNum];

    return;
}

void execStatementWithoutTrailingSubstatement(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            // cout << "swts " << attr3AC[nodeNum].threeAC.size() << endl;
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 3:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 4:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 5:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            string snext = getLabel(c,3);
            string temp = snext + ":";
            attr3AC[nodeNum].threeAC.push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            pushLabelUp(nodeNum,c);
        }
        break;
        case 6:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 7:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 8:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 9:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 10:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 11:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
    }
    return;
}

void execExpressionStatement(int nodeNum){
    int c = adj[nodeNum][0];
    attr3AC[nodeNum] = attr3AC[c];
    pushLabelUp(nodeNum,c);
    return;
}

void execStatementExpression(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            // cout << "statementexpression " << attr3AC[nodeNum].threeAC.size() << endl;;
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 3:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 4:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 5:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 6:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 7:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
    }
    return;
}

void execPrimary(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
    }
    return;
}

void execArrayCreationExpression(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c2 = adj[nodeNum][1];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum]=attr3AC[c2]+attr3AC[c3];
            int totsize = 8;
            for(int i=0;i<attr3AC[c3].arrDims.size();i++){
                totsize*=attr3AC[c3].arrDims[i];
            }
            // cout << "idhar " << totsize << endl;
            tempNum++;
            string temp = "t" + to_string(tempNum);
            string temp2 = temp + " = " + to_string(totsize);
            attr3AC[nodeNum].threeAC.push_back(temp2);
            temp2 = "pushparam " + temp;
            attr3AC[nodeNum].threeAC.push_back(temp2);
            temp2 = "stackpointer + 4";
            attr3AC[nodeNum].threeAC.push_back(temp2);
            temp2 = "call allocmem 1";
            attr3AC[nodeNum].threeAC.push_back(temp2);
            temp2 = "stackpointer - 4";
            attr3AC[nodeNum].threeAC.push_back(temp2);

            // tempNum++;
            // temp = "t" + to_string(tempNum);
            // temp2 = temp + " = " + "popparam";
            // attr3AC[nodeNum].threeAC.push_back(temp2);
            attr3AC[nodeNum].addrName = "popparam";

            //GAS x86_64 instructions
            int tempOffset = -8*(stoi(temp.substr(1)))-8;
            string temp3 = "movq $" + to_string(totsize) + ", %rdi";
            attr3AC[nodeNum].assemblyCode.push_back(temp3);
            //call the sbrk syscall
            temp3 = "movq $12, %rax";
            attr3AC[nodeNum].assemblyCode.push_back(temp3);
            temp3 = "syscall";
            attr3AC[nodeNum].assemblyCode.push_back(temp3);
            // temp3 = "callq malloc";
            // attr3AC[nodeNum].assemblyCode.push_back(temp3);
            // temp3 = "movq %rax, " + to_string(tempOffset) + "(%rbp)";
            // attr3AC[nodeNum].assemblyCode.push_back(temp3);
        }
        break;
        case 2:{
            // int c2 = adj[nodeNum][1];
            // int c3 = adj[nodeNum][2];
            // int c4 = adj[nodeNum][3];
            // attr3AC[nodeNum] = attr3AC[c3] + attr3AC[c4];
            // attr3AC[nodeNum].type = attr3AC[c2].type;
            // tempNum++;
            // attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            // string temp = attr3AC[nodeNum].addrName + " = NEW " + attr3AC[c2].type;
            // for(int i=0;i<attr3AC[nodeNum].arrDims.size();i++){
            //     temp = temp + to_string((attr3AC[nodeNum].arrDims)[i]) + ", ";
            // }
            // attr3AC[nodeNum].threeAC.push_back(temp);
        }
        break;
        case 3:
        break;
        case 4:
        break;
    }
    return;
}

void execArrayAccess(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            auto mdata= getArrayInfo(attr3AC[c].addrName,attr3AC[c].nodeno);
            // auto mdata = getArrayInfo(nodeType[adj[attr3AC[c].nodeno][0]],attr3AC[c].nodeno);// lowestnode
            // cout << "in array access wefasdfsf " << attr3AC[c].arrDims.size() << " " << attr3AC[c].dimsDone << endl;

            // cout << "over here " << nodeNum << " " << attr3AC[c].nodeno << " " << nodeType[adj[attr3AC[c].nodeno][0]] << endl;
            string t = mdata.first;
            vector<int> d = mdata.second;
            // cout << "idhar " << t << " " << d.size() << endl;
            int mult=8;
            for(int i=0;i<d.size();i++){
                // cout << "dims " << i << " " << d[i] << endl;
                if(i)mult*=d[i];
                attr3AC[nodeNum].arrDims.push_back(d[i]);
            }
            attr3AC[nodeNum] = attr3AC[c3];
            attr3AC[nodeNum].type = t;
            tempNum++;
            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            typeOfNode[attr3AC[nodeNum].addrName] = t;
            attr3AC[nodeNum].dimsDone++;//dimsdone=1
            for(int i=0;i<d.size();i++)attr3AC[nodeNum].arrDims.push_back(d[i]);
            attr3AC[nodeNum].nameAtNode = nodeType[attr3AC[c].nodeno];
            // cout << "yaha pe hu aray1 " << attr3AC[nodeNum].dimsDone << " " << attr3AC[c].dimsDone << endl;
            string temp = attr3AC[nodeNum].addrName + " = " + attr3AC[c3].addrName + " * " + to_string(mult);
            attr3AC[nodeNum].threeAC.push_back(temp);

            
            //Assembly code generation GAS x86_64
            // string temp2 = "movq " + attr3AC[c3].addrName + ", %rax";
            // cout << "oba here "<<attr3AC[nodeNum].addrName << " " << attr3AC[c3].addrName << " " << to_string(mult) << endl;
            vector<string> tempVec = getMulAssemblyCode(attr3AC[nodeNum].addrName,attr3AC[c3].addrName,to_string(mult));
            // cout << "after mul" << endl;
            for(int i=0;i<tempVec.size();i++){
                attr3AC[nodeNum].assemblyCode.push_back(tempVec[i]);
            }
            // cout << "done with acess" << endl;
        }
        break;
        case 2:{
            int c =adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            // cout << "first print " << attr3AC[c].dimsDone << endl;
            attr3AC[nodeNum] = attr3AC[c];
            attr3AC[nodeNum] = attr3AC[nodeNum]+attr3AC[c3];
            int mult = 8;
            // cout << "in array access " << attr3AC[c].arrDims.size() << " " << attr3AC[c].dimsDone << " " << mult << endl;
            for(int i=attr3AC[c].dimsDone+1;i<attr3AC[c].arrDims.size();i++){
                // cout << "multipling " << attr3AC[c].dimsDone << " " << i << " " << attr3AC[c].arrDims[i] <<" " << mult << endl;
                mult*=(attr3AC[c].arrDims)[i];
            }
            tempNum++;
            string temp = "t" + to_string(tempNum) + " = " + attr3AC[c3].addrName + " * " + to_string(mult);
            //assembly
            auto tempVec = getMulAssemblyCode("t"+to_string(tempNum),attr3AC[c3].addrName,to_string(mult));
            for(int i=0;i<tempVec.size();i++){
                attr3AC[nodeNum].assemblyCode.push_back(tempVec[i]);
            }
            typeOfNode["t"+to_string(tempNum)] = attr3AC[c].type;
            attr3AC[nodeNum].threeAC.push_back(temp);
            tempNum++;
            typeOfNode[attr3AC[nodeNum].addrName] = attr3AC[c].type;
            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            // cout << "dims 2 hojaye " << attr3AC[c].dimsDone << endl; 
            attr3AC[nodeNum].dimsDone = attr3AC[c].dimsDone +1;
            attr3AC[nodeNum].arrDims.clear();
            attr3AC[nodeNum].type = attr3AC[c].type;
            for(int i=0;i<attr3AC[c].arrDims.size();i++)attr3AC[nodeNum].arrDims.push_back(attr3AC[c].arrDims[i]);
            // cout << "dims 2 hojaye " << attr3AC[nodeNum].dimsDone << endl; 
            attr3AC[nodeNum].nameAtNode = attr3AC[c].nameAtNode;
            // cout << "yaha pe hu aray " << attr3AC[nodeNum].nameAtNode << " " << attr3AC[nodeNum].arrDims.size() << endl;
            temp = attr3AC[nodeNum].addrName + " = " + attr3AC[c].addrName + " + t" + to_string(tempNum-1) ;
            attr3AC[nodeNum].threeAC.push_back(temp);

            //Call function getmulassembly to generate assemblycode
            tempVec = getAddAssemblyCode(attr3AC[nodeNum].addrName,attr3AC[c].addrName,"t"+to_string(tempNum-1));
            for(int i=0;i<tempVec.size();i++){
                attr3AC[nodeNum].assemblyCode.push_back(tempVec[i]);
            }
        }   
        break;
    }
    return;
}

void execDimExprs(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            // cout << "dimexprs" << endl;
        }   
        break;
        case 2:{
            int c = adj[nodeNum][0];
            int c2 = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c] + attr3AC[c2];
        }
        break;
    }
    return;
}

void execDimExpr(int nodeNum){
    int c = adj[nodeNum][1];
    attr3AC[nodeNum]=attr3AC[c];
    // cout << "Dimexpr1 " << attr3AC[c].nameAtNode << endl;
    if(attr3AC[c].nameAtNode.size()!=0)attr3AC[nodeNum].arrDims.push_back(stoi(attr3AC[c].nameAtNode));
    else{
        attr3AC[nodeNum].arrDims.push_back(stoi(attr3AC[c].addrName));
        // cout << "dimexpr " << attr3AC[nodeNum].arrDims[0] << endl;
    }
    return;
}

void execDims(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            attr3AC[nodeNum].arrDims.push_back(-1);
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            attr3AC[nodeNum].arrDims.push_back(-1);
        }
        break;
    }
    return;
}

void execPrimaryNoNewArray(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:{
            attr3AC[nodeNum].isthis=1;
            attr3AC[nodeNum].nameAtNode = "this";
        }
        break;
        case 3:{
            int c = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 4:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 5:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
        case 6:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            // cout << "idahr dekhra hu " << attr3AC[nodeNum].dimsDone << " " << attr3AC[c].dimsDone << endl;
            // cout << "check dims " << attr3AC[nodeNum].dimsDone << " " << attr3AC[nodeNum].arrDims.size() << endl;
            attr3AC[nodeNum].dimsDone = attr3AC[c].dimsDone;
            if(attr3AC[nodeNum].dimsDone == attr3AC[nodeNum].arrDims.size()){
                tempNum++;
                attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
                typeOfNode[attr3AC[nodeNum].addrName] = attr3AC[c].type;
                string temp = attr3AC[nodeNum].addrName + " = " + attr3AC[c].nameAtNode + " [ " + attr3AC[c].addrName + " ] ";
                attr3AC[nodeNum].threeAC.push_back(temp);
                // cout << "idhar aagya " << endl;

                //Generate GAS code for array access
                string tempArr = varToTemp[attr3AC[c].nameAtNode];
                //Put value of index from attr3AC[c].addrName inside %rsi
                int tempOffset;string tempA;
                if(attr3AC[c].addrName[0]=='t'){
                    tempOffset = -8*(stoi(attr3AC[c].addrName.substr(1)))-8;
                    tempA = "movq " + to_string(tempOffset) + "(%rbp), %rsi";
                }else{
                    tempOffset = stoi(attr3AC[c].addrName);
                    tempA = "movq $" + attr3AC[c].addrName + ", %rsi"; 
                }
                attr3AC[nodeNum].assemblyCode.push_back(tempA);// ith index of array pushed
                // string arg1=getArgumentFromTemp(attr3AC[c].addrName);
                // cout << "over here now" << endl;

                //Get base of array in %rax
                tempOffset = -8*stoi(tempArr.substr(1))-8; 
                // string arg1 = to_string(tempOffset)+ "(%rbx,%rsi,8)";
                string movins = "movq " + to_string(tempOffset) + "(%rbp), %rax";
                attr3AC[nodeNum].assemblyCode.push_back(movins);
                // cout << "okay " << endl;
                //add %rsi and %rax
                attr3AC[nodeNum].assemblyCode.push_back("subq (%rax), %rsi");
                //move from %rax to tempvar
                tempOffset = -8*stoi(attr3AC[nodeNum].addrName.substr(1))-8;
                movins = "movq (%rsi), %r9";
                attr3AC[nodeNum].assemblyCode.push_back(movins);
                movins = "movq %r9, " + to_string(tempOffset) + "(%rbp)";
                // cout << "error here?" << endl;
                attr3AC[nodeNum].assemblyCode.push_back(movins);
                // cout << "done with this" << endl;
            }
            pushLabelUp(nodeNum,c);
        }
        break;
        case 7:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
    }
    return;
}

void execMethodInvocation(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{//simplename
            if(prodNum[adj[nodeNum][0]]==1){
                int c = adj[nodeNum][0];
                attr3AC[nodeNum] = attr3AC[c];
                tempNum++;
                attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
                attr3AC[nodeNum].type = getFuncRet(attr3AC[c].nodeno,attr3AC[c].addrName,insideClassName);
                // cout << "inside methodinvocation " << endl;
                vector<int> p;
                checkFunctionParameterTypes(attr3AC[c].nodeno, p);
                // pushonstack oldstackpointer
                //stackpointer =8
                //oldsp = sp
                string oldsp = "pushonstack oldstackpointer";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                oldsp = "stackpointer + 8";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                oldsp = "oldstackpointer = stackpointer";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                string temp = "call " + attr3AC[c].addrName +insideClassName + ", 0";
                attr3AC[nodeNum].threeAC.push_back(temp);
                temp = attr3AC[nodeNum].addrName + " = popparam";
                attr3AC[nodeNum].threeAC.push_back(temp);
                //stackpointer = oldsp
                //sp-8
                //oldsp=popfromstack
                oldsp = "stackpointer = oldstackpointer";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                oldsp = "stackpointer - 8";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                oldsp = "oldstackpointer = popfromstack";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                typeOfNode[attr3AC[nodeNum].addrName] = getFuncRet(nodeNum, attr3AC[c].addrName, insideClassName);

                //GAS code for function calls
                string temp2 = "callq " + attr3AC[c].addrName + insideClassName;
                attr3AC[nodeNum].assemblyCode.push_back(temp2);
                //store rax in temp after getting its offset
                int tempOffset = -8*(stoi(attr3AC[nodeNum].addrName.substr(1)))-8;
                string movins = "movq %rax, " + to_string(tempOffset) + "(%rbp)";
                attr3AC[nodeNum].assemblyCode.push_back(movins);

                pushLabelUp(nodeNum,c);
            }else{
                int c = adj[nodeNum][0];
                attr3AC[nodeNum] = attr3AC[c];
                attr3AC[nodeNum].type = getFuncRet(attr3AC[c].nodeno,attr3AC[c].addrName,insideClassName);
                tempNum++;
                string fname = attr3AC[adj[adj[c][0]][2]].addrName;
                attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
                // cout << "inside methodinvocation " << endl;
                // vector<int> p;
                // checkFunctionParameterTypes(attr3AC[c].nodeno, p);
                string oldsp = "pushonstack oldstackpointer";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                oldsp = "stackpointer + 8";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                oldsp = "oldstackpointer = stackpointer";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                string temp = "call " + fname +insideClassName + ", 0";
                attr3AC[nodeNum].threeAC.push_back(temp);
                temp = attr3AC[nodeNum].addrName + " = popparam";
                attr3AC[nodeNum].threeAC.push_back(temp);
                oldsp = "stackpointer = oldstackpointer";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                oldsp = "stackpointer - 8";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                oldsp = "oldstackpointer = popfromstack";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                typeOfNode[attr3AC[nodeNum].addrName] = getFuncRet(nodeNum, fname, insideClassName);
                pushLabelUp(nodeNum,c);

                //GAS x86_64 code for function call
                string temp2 = "callq " + fname + insideClassName;
                attr3AC[nodeNum].assemblyCode.push_back(temp2);
                //store rax in temp after getting its offset
                int tempOffset = -8*(stoi(attr3AC[nodeNum].addrName.substr(1)))-8;
                string movins = "movq %rax, " + to_string(tempOffset) + "(%rbp)";
                attr3AC[nodeNum].assemblyCode.push_back(movins);
            }
        }
            break;
        case 2:{
            if(prodNum[adj[nodeNum][0]]==1){
                int c = adj[nodeNum][0];
                int c3 = adj[nodeNum][2];
                attr3AC[nodeNum].type = getFuncRet(attr3AC[c].nodeno,attr3AC[c].addrName,insideClassName);
                attr3AC[nodeNum] = attr3AC[c] + attr3AC[c3];
                tempNum++;
                attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
                string oldsp = "pushonstack oldstackpointer";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                oldsp = "stackpointer + 8";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                oldsp = "oldstackpointer = stackpointer";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                for(int fcall=0; fcall<(attr3AC[c3].params).size();fcall++){
                    string temp = "pushparam " + (attr3AC[c3].params)[fcall];
                    attr3AC[nodeNum].threeAC.push_back(temp);
                }
                checkFunctionParameterTypes(attr3AC[c].nodeno, attr3AC[c3].paramsNodeNo);
                int sizeOfParams=0;
                for(int i=0;i<attr3AC[c3].paramsNodeNo.size();i++){
                    // sizeOfParams+=typeSize[typeOfNode[to_string(attr3AC[c3].paramsNodeNo[i])]];
                    sizeOfParams+=8;//since we're only required to implement long type
                }
                string spointer = "stackpointer + " + to_string(sizeOfParams);
                attr3AC[nodeNum].threeAC.push_back(spointer);
                string temp = "call " + attr3AC[c].addrName + insideClassName + ", " + to_string(attr3AC[c3].params.size());
                attr3AC[nodeNum].threeAC.push_back(temp);
                temp = attr3AC[nodeNum].addrName + " = popparam";
                attr3AC[nodeNum].threeAC.push_back(temp);
                oldsp = "stackpointer = oldstackpointer";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                oldsp = "stackpointer - 8";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                oldsp = "oldstackpointer = popfromstack";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                typeOfNode[attr3AC[nodeNum].addrName] = getFuncRet(nodeNum, attr3AC[c].addrName, insideClassName);
                pushLabelUp(nodeNum,c);

                //GAS x86_64 code for function call with parameters, pass the args in corresponding registers
                for(int fcall=0; fcall<(attr3AC[c3].params).size();fcall++){
                    if(fcall==0){
                        string p = attr3AC[c3].params[fcall];
                        string ass = addFuncParamsToReg(p,"%rdi",c3,fcall);
                        attr3AC[nodeNum].assemblyCode.push_back(ass);
                    }else if(fcall==1){
                        string p = attr3AC[c3].params[fcall];
                        string ass = addFuncParamsToReg(p,"%rsi",c3,fcall);
                        attr3AC[nodeNum].assemblyCode.push_back(ass);
                    }else if(fcall==2){
                        string p = attr3AC[c3].params[fcall];
                        string ass = addFuncParamsToReg(p,"%rdx",c3,fcall);
                        attr3AC[nodeNum].assemblyCode.push_back(ass);
                    }else if(fcall==3){
                        string p = attr3AC[c3].params[fcall];
                        string ass = addFuncParamsToReg(p,"%rcx",c3,fcall);
                        attr3AC[nodeNum].assemblyCode.push_back(ass);
                    }else if(fcall==4){
                        string p = attr3AC[c3].params[fcall];
                        string ass = addFuncParamsToReg(p,"%r8",c3,fcall);
                        attr3AC[nodeNum].assemblyCode.push_back(ass);
                    }else if(fcall==5){
                        string p = attr3AC[c3].params[fcall];
                        string ass = addFuncParamsToReg(p,"%r9",c3,fcall);
                        attr3AC[nodeNum].assemblyCode.push_back(ass);
                    }else{
                        cout << "Too many func arguments" << endl;
                        exit(0);
                    }
                }
                //call function in GAS
                string callFun = "callq " + attr3AC[c].addrName + insideClassName;
                attr3AC[nodeNum].assemblyCode.push_back(callFun);
                //store rax in temp after getting its offset
                int tempOffset = -8*(stoi(attr3AC[nodeNum].addrName.substr(1)))-8;
                string movins = "movq %rax, " + to_string(tempOffset) + "(%rbp)";
                attr3AC[nodeNum].assemblyCode.push_back(movins);

            }else{
                int c = adj[nodeNum][0];
                int c3 = adj[nodeNum][2];
                string fname = attr3AC[adj[adj[c][0]][2]].addrName;
                attr3AC[nodeNum].type = getFuncRet(attr3AC[c].nodeno,fname,insideClassName);
                // string fname = attr3AC[adj[adj[c][0]][2]].addrName;
                // cout << "in methodinvocation " << fname << endl;
                // cout << "inside method " << attr3AC[c].threeAC.size() << " " << attr3AC[c3].threeAC.size() << endl;
                // cout << attr3AC[c].threeAC[0] << endl;
                attr3AC[nodeNum] = attr3AC[c] + attr3AC[c3];
                tempNum++;
                attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
                string oldsp = "pushonstack oldstackpointer";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                oldsp = "stackpointer + 8";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                oldsp = "oldstackpointer = stackpointer";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                for(int fcall=0; fcall<(attr3AC[c3].params).size();fcall++){
                    string temp = "pushparam " + (attr3AC[c3].params)[fcall];
                    attr3AC[nodeNum].threeAC.push_back(temp);
                }
                // checkFunctionParameterTypes(attr3AC[c].nodeno, attr3AC[c3].paramsNodeNo);
                int sizeOfParams=0;
                for(int i=0;i<attr3AC[c3].paramsNodeNo.size();i++){
                    // sizeOfParams+=typeSize[typeOfNode[to_string(attr3AC[c3].paramsNodeNo[i])]];
                    sizeOfParams+=8;//Since we're only required to implement long
                }
                string spointer = "stackpointer + " + to_string(sizeOfParams);
                attr3AC[nodeNum].threeAC.push_back(spointer);
                string temp = "call " + fname + insideClassName + ", " + to_string(attr3AC[c3].params.size());
                attr3AC[nodeNum].threeAC.push_back(temp);
                temp = attr3AC[nodeNum].addrName + " = popparam";
                attr3AC[nodeNum].threeAC.push_back(temp);
                oldsp = "stackpointer = oldstackpointer";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                oldsp = "stackpointer - 8";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                oldsp = "oldstackpointer = popfromstack";
                attr3AC[nodeNum].threeAC.push_back(oldsp);
                typeOfNode[attr3AC[nodeNum].addrName] = getFuncRet(nodeNum, fname, insideClassName);
                pushLabelUp(nodeNum,c);

                //GAS x86_64 code for function call with parameters
                for(int fcall=0; fcall<(attr3AC[c3].params).size();fcall++){
                    if(fcall==0){
                        string p = attr3AC[c3].params[fcall];
                        string ass = addFuncParamsToReg(p,"%rdi",c3,fcall);
                        attr3AC[nodeNum].assemblyCode.push_back(ass);
                    }else if(fcall==1){
                        string p = attr3AC[c3].params[fcall];
                        string ass = addFuncParamsToReg(p,"%rsi",c3,fcall);
                        attr3AC[nodeNum].assemblyCode.push_back(ass);
                    }else if(fcall==2){
                        string p = attr3AC[c3].params[fcall];
                        string ass = addFuncParamsToReg(p,"%rdx",c3,fcall);
                        attr3AC[nodeNum].assemblyCode.push_back(ass);
                    }else if(fcall==3){
                        string p = attr3AC[c3].params[fcall];
                        string ass = addFuncParamsToReg(p,"%rcx",c3,fcall);
                        attr3AC[nodeNum].assemblyCode.push_back(ass);
                    }else if(fcall==4){
                        string p = attr3AC[c3].params[fcall];
                        string ass = addFuncParamsToReg(p,"%r8",c3,fcall);
                        attr3AC[nodeNum].assemblyCode.push_back(ass);
                    }else if(fcall==5){
                        string p = attr3AC[c3].params[fcall];
                        string ass = addFuncParamsToReg(p,"%r9",c3,fcall);
                        attr3AC[nodeNum].assemblyCode.push_back(ass);
                    }else{
                        cout << "Too many func arguments" << endl;
                        exit(0);
                    }
                }
                //call function in GAS
                string callFun = "callq " + fname + insideClassName;
                attr3AC[nodeNum].assemblyCode.push_back(callFun);
                //store rax in temp after getting its offset
                int tempOffset = -8*(stoi(attr3AC[nodeNum].addrName.substr(1)))-8;
                string movins = "movq %rax, " + to_string(tempOffset) + "(%rbp)";
                attr3AC[nodeNum].assemblyCode.push_back(movins);
            }
        }
            break;
    }

    return;
}

void execVariableInitializer(int nodeNum){
    int c;
    switch(prodNum[nodeNum]){
        case 1:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            // cout << "var init  " << attr3AC[nodeNum].threeAC.size() << endl;
            break;
        case 2:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
    }
}

void execArgumentList(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            attr3AC[nodeNum].params.push_back(attr3AC[c].addrName);
            attr3AC[nodeNum].paramsNodeNo.push_back(attr3AC[c].nodeno);
            // cout << "in argument list " << attr3AC[c].nodeno << endl;
            pushLabelUp(nodeNum,c);
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c];
            attr3AC[nodeNum].params.push_back(attr3AC[c3].addrName);
            attr3AC[nodeNum].paramsNodeNo.push_back(attr3AC[c3].nodeno);
            pushLabelUp(nodeNum,c);
        }

        break;
    }
    return;
}

void execExpression(int nodeNum){
    int c = adj[nodeNum][0];
    attr3AC[nodeNum] = attr3AC[c];
    someExpAddr = attr3AC[nodeNum].addrName;
    pushLabelUp(nodeNum,c);
    return;
}

void execAssignment(int nodeNum){
    int c = adj[nodeNum][0];
    int c2 = adj[nodeNum][1];
    int c3 = adj[nodeNum][2];
    switch(prodNum[c2]){
        case 1:{
            string arg1 ;
            string arg2 ;
            string t1=attr3AC[c].addrName ;
            string t2=attr3AC[c3].addrName ;
            // cout<<"t1 "<<t1<<endl;
            // cout<<t1.size()<<endl;
            // cout<<"t2 "<<t2<<endl;
            bool flag=false;
            string tempac1,tempac2,tempac3,tempac4,tempac,tempac3a;
            if (t1[0]=='*'|| t2[0]=='*'){
                flag=true;
                // cout<<"In wherersXdas\n";
                if (t1[0]=='*'){
                    // t1 is of the form *(t8 + t9) 
                    string arg2=getArg(t2);
                    string argti = getArg(t1.substr(2,t1.find("+")-2));
                    string argtj = getArg(t1.substr(t1.find("+")+2, t1.find(")")-t1.find("+")-2));
                    tempac1 = "movq " + argti + ", %r8";
                    tempac2 = "movq " + argtj + ", %r9";
                    tempac3 = "addq %r9, %r8";
                    tempac3a = "movq " + arg2 + ", %r10";
                    tempac3 += "\n";
                    tempac3 += tempac3a;
                    tempac4 = "movq %r10, (%r8)";

                }
                else{
                    string arg1=getArg(t1);
                    string argti = getArg(t2.substr(2,t2.find("+")-2));
                    string argtj = getArg(t2.substr(t2.find("+")+2, t2.find(")")-t2.find("+")-2));
                    tempac1 = "movq " + argti + ", %r8";
                    tempac2 = "movq " + argtj + ", %r9";
                    tempac3 = "addq %r9, %r8";
                    tempac4 = "movq 0(%r8), "+arg1;

                }
                // load t8 and t9 in registers
                

            }
            else{

                if (t1[0]=='t'){
                    
                    int of1 = -8*stoi(t1.substr(1))-8;
                    arg1 = to_string(of1);
                    arg1+= "(%rbp)";
                }
                // else if t1[0] is integer character
                else if (t1[0] >= '0' && t1[0] <= '9'){
                    arg1 = "$"+t1;
                }
                else{
                    t1=varToTemp[t1];
                    int of1 = -8*stoi(t1.substr(1))-8;
                    arg1 = to_string(of1);
                    arg1+= "(%rbp)";
                }
                
                if (t2[0]=='t'){
                    int of2 = -8*stoi(t2.substr(1))-8;
                    arg2 = to_string(of2);
                    arg2+= "(%rbp)";
                }
                // else if t2[0] is integer character
                else if (t2[0] >= '0' && t2[0] <= '9'){
                    arg2 = "$"+t2;
                }
                else{
                    t2=varToTemp[t2];
                    int of2 = -8*stoi(t2.substr(1))-8;
                    arg2 = to_string(of2);
                    arg2+= "(%rbp)";
                }
                // tempac = "movq " + arg2 + ", " + arg1;
                tempac = "movq " + arg2 + ", %r10" ;
                tempac += "\nmovq %r10, " + arg1;

                // cout<<tempac<<endl;

            }

            if(attr3AC[c].isthis==0){
                    attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
                    string temp = attr3AC[c].addrName + " = " + attr3AC[c3].addrName;
                    // cout << "assignment " << temp << endl;
                    typeOfNode[attr3AC[nodeNum].addrName]=getTypeNode(c3);
                    attr3AC[nodeNum].threeAC.push_back(temp);
                    if(flag==false)
                        attr3AC[nodeNum].assemblyCode.push_back(tempac);
                    else{
                        attr3AC[nodeNum].assemblyCode.push_back(tempac1);
                        attr3AC[nodeNum].assemblyCode.push_back(tempac2);
                        attr3AC[nodeNum].assemblyCode.push_back(tempac3);
                        attr3AC[nodeNum].assemblyCode.push_back(tempac4);

                    }
                    // cout << "assignment me " << attr3AC[c].addrName << " " << typeOfNode[attr3AC[nodeNum].addrName] << endl;
                    attr3AC[nodeNum].addrName = attr3AC[c].addrName;
            }
            else{
                    attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
                    string temp = attr3AC[c].addrName + " = " + attr3AC[c3].addrName;

                    typeOfNode[attr3AC[nodeNum].addrName]=getTypeNode(c3);
                    attr3AC[nodeNum].threeAC.push_back(temp);
                    if (flag==false)
                        attr3AC[nodeNum].assemblyCode.push_back(tempac);
                    else{

                        attr3AC[nodeNum].assemblyCode.push_back(tempac1);
                        attr3AC[nodeNum].assemblyCode.push_back(tempac2);
                        attr3AC[nodeNum].assemblyCode.push_back(tempac3);
                        attr3AC[nodeNum].assemblyCode.push_back(tempac4);

                    }
                    attr3AC[nodeNum].addrName = attr3AC[c].addrName;
            }
            
        }
        break;
        case 2:{
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            string temp = attr3AC[c].addrName + " *= " + attr3AC[c3].addrName;
            typeOfNode[attr3AC[nodeNum].addrName]=getTypeNode(c3);
            attr3AC[nodeNum].threeAC.push_back(temp);
            
            string arg1 = getArg(attr3AC[c].addrName);
            string arg2 = getArg(attr3AC[c3].addrName);
            string tempac1 =  "movq "+arg1+",%r8" ;
            string tempac2 =  "movq "+arg2+",%r9" ;
            string tempac3 =  "imulq %r9,%r8" ;
            string tempac4 =  "movq %r8,"+arg1 ;

            attr3AC[nodeNum].assemblyCode.push_back(tempac1);
            attr3AC[nodeNum].assemblyCode.push_back(tempac2);
            attr3AC[nodeNum].assemblyCode.push_back(tempac3);
            attr3AC[nodeNum].assemblyCode.push_back(tempac4);
            // cout << "assignment me " << attr3AC[c].addrName << " " << typeOfNode[attr3AC[nodeNum].addrName] << endl;
            attr3AC[nodeNum].addrName = attr3AC[c].addrName;
        }
        break;
        case 3:{
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            string temp = attr3AC[c].addrName + " /= " + attr3AC[c3].addrName;
            typeOfNode[attr3AC[nodeNum].addrName]=getTypeNode(c3);
            attr3AC[nodeNum].threeAC.push_back(temp);

            string arg1 = getArg(attr3AC[c].addrName);
            string arg2 = getArg(attr3AC[c3].addrName);
            string tempac1 =  "movq "+arg1+",%rax" ;
            string tempac2 =  "movq "+arg2+",%r9" ;
            string tempac3 =  "idivq %r9" ;
            string tempac4 =  "movq %rax,"+arg1 ;

            attr3AC[nodeNum].assemblyCode.push_back(tempac1);
            attr3AC[nodeNum].assemblyCode.push_back(tempac2);
            attr3AC[nodeNum].assemblyCode.push_back("cqto");
            attr3AC[nodeNum].assemblyCode.push_back(tempac3);
            attr3AC[nodeNum].assemblyCode.push_back(tempac4);
            // cout << "assignment me " << attr3AC[c].addrName << " " << typeOfNode[attr3AC[nodeNum].addrName] << endl;
            attr3AC[nodeNum].addrName = attr3AC[c].addrName;
        }
        break;
        case 4:{
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            string temp = attr3AC[c].addrName + " += " + attr3AC[c3].addrName;
            typeOfNode[attr3AC[nodeNum].addrName]=getTypeNode(c3);
            attr3AC[nodeNum].threeAC.push_back(temp);
                       
            string arg1 = getArg(attr3AC[c].addrName);
            string arg2 = getArg(attr3AC[c3].addrName);
            string tempac1 =  "movq "+arg1+",%r8" ;
            string tempac2 =  "movq "+arg2+",%r9" ;
            string tempac3 =  "addq %r9,%r8" ;
            string tempac4 =  "movq %r8,"+arg1 ;

            attr3AC[nodeNum].assemblyCode.push_back(tempac1);
            attr3AC[nodeNum].assemblyCode.push_back(tempac2);
            attr3AC[nodeNum].assemblyCode.push_back(tempac3);
            attr3AC[nodeNum].assemblyCode.push_back(tempac4);
  
            // cout << "assignment me " << attr3AC[c].addrName << " " << typeOfNode[attr3AC[nodeNum].addrName] << endl;
            attr3AC[nodeNum].addrName = attr3AC[c].addrName;
        }
        break;
        case 5:{
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            string temp = attr3AC[c].addrName + " -= " + attr3AC[c3].addrName;
            typeOfNode[attr3AC[nodeNum].addrName]=getTypeNode(c3);
            attr3AC[nodeNum].threeAC.push_back(temp);
                       
            string arg1 = getArg(attr3AC[c].addrName);
            string arg2 = getArg(attr3AC[c3].addrName);
            string tempac1 =  "movq "+arg1+",%r8" ;
            string tempac2 =  "movq "+arg2+",%r9" ;
            string tempac3 =  "subq %r9,%r8" ;
            string tempac4 =  "movq %r8,"+arg1 ;

            attr3AC[nodeNum].assemblyCode.push_back(tempac1);
            attr3AC[nodeNum].assemblyCode.push_back(tempac2);
            attr3AC[nodeNum].assemblyCode.push_back(tempac3);
            attr3AC[nodeNum].assemblyCode.push_back(tempac4);
  
            // cout << "assignment me " << attr3AC[c].addrName << " " << typeOfNode[attr3AC[nodeNum].addrName] << endl;
            attr3AC[nodeNum].addrName = attr3AC[c].addrName;
        }
        break;
        case 6:{
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            string temp = attr3AC[c].addrName + " <<= " + attr3AC[c3].addrName;
            typeOfNode[attr3AC[nodeNum].addrName]=getTypeNode(c3);
            attr3AC[nodeNum].threeAC.push_back(temp);
                       
            string arg1 = getArg(attr3AC[c].addrName);
            string arg2 = getArg(attr3AC[c3].addrName);
            string tempac1 =  "movq "+arg1+",%r8" ;
            string tempac2 =  "movq "+arg2+",%r9" ;
            string tempac3 =  "salq %r8,%r9" ;
            string tempac4 =  "movq %r8,"+arg1 ;

            attr3AC[nodeNum].assemblyCode.push_back(tempac1);
            attr3AC[nodeNum].assemblyCode.push_back(tempac2);
            attr3AC[nodeNum].assemblyCode.push_back(tempac3);
            attr3AC[nodeNum].assemblyCode.push_back(tempac4);
            
            // cout << "assignment me " << attr3AC[c].addrName << " " << typeOfNode[attr3AC[nodeNum].addrName] << endl;
            attr3AC[nodeNum].addrName = attr3AC[c].addrName;
        }
        break;
        case 7:{
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            string temp = attr3AC[c].addrName + " >>= " + attr3AC[c3].addrName;
            typeOfNode[attr3AC[nodeNum].addrName]=getTypeNode(c3);
            attr3AC[nodeNum].threeAC.push_back(temp);
                                   
            string arg1 = getArg(attr3AC[c].addrName);
            string arg2 = getArg(attr3AC[c3].addrName);
            string tempac1 =  "movq "+arg1+",%r8" ;
            string tempac2 =  "movq "+arg2+",%r9" ;
            string tempac3 =  "sarq %r8,%r9" ;
            string tempac4 =  "movq %r8,"+arg1 ;

            attr3AC[nodeNum].assemblyCode.push_back(tempac1);
            attr3AC[nodeNum].assemblyCode.push_back(tempac2);
            attr3AC[nodeNum].assemblyCode.push_back(tempac3);
            attr3AC[nodeNum].assemblyCode.push_back(tempac4);

            // cout << "assignment me " << attr3AC[c].addrName << " " << typeOfNode[attr3AC[nodeNum].addrName] << endl;
            attr3AC[nodeNum].addrName = attr3AC[c].addrName;
        }
        break;
        case 8:{
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            string temp = attr3AC[c].addrName + " >>>= " + attr3AC[c3].addrName;
            typeOfNode[attr3AC[nodeNum].addrName]=getTypeNode(c3);
            attr3AC[nodeNum].threeAC.push_back(temp);
                                   
            string arg1 = getArg(attr3AC[c].addrName);
            string arg2 = getArg(attr3AC[c3].addrName);
            string tempac1 =  "movq "+arg1+",%r8" ;
            string tempac2 =  "movq "+arg2+",%r9" ;
            string tempac3 =  "shrq %r8,%r9" ;
            string tempac4 =  "movq %r8,"+arg1 ;

            attr3AC[nodeNum].assemblyCode.push_back(tempac1);
            attr3AC[nodeNum].assemblyCode.push_back(tempac2);
            attr3AC[nodeNum].assemblyCode.push_back(tempac3);
            attr3AC[nodeNum].assemblyCode.push_back(tempac4);

            // cout << "assignment me " << attr3AC[c].addrName << " " << typeOfNode[attr3AC[nodeNum].addrName] << endl;
            attr3AC[nodeNum].addrName = attr3AC[c].addrName;
        }
        break;
        case 9:{
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            string temp = attr3AC[c].addrName + " &= " + attr3AC[c3].addrName;
            typeOfNode[attr3AC[nodeNum].addrName]=getTypeNode(c3);
            attr3AC[nodeNum].threeAC.push_back(temp);
            // cout << "assignment me " << attr3AC[c].addrName << " " << typeOfNode[attr3AC[nodeNum].addrName] << endl;
            attr3AC[nodeNum].addrName = attr3AC[c].addrName;
        }
        break;
        case 10:{
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            string temp = attr3AC[c].addrName + " ^= " + attr3AC[c3].addrName;
            typeOfNode[attr3AC[nodeNum].addrName]=getTypeNode(c3);
            attr3AC[nodeNum].threeAC.push_back(temp);
            // cout << "assignment me " << attr3AC[c].addrName << " " << typeOfNode[attr3AC[nodeNum].addrName] << endl;
            attr3AC[nodeNum].addrName = attr3AC[c].addrName;
        }
        break;
        case 11:{
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            string temp = attr3AC[c].addrName + " |= " + attr3AC[c3].addrName;
            typeOfNode[attr3AC[nodeNum].addrName]=getTypeNode(c3);
            attr3AC[nodeNum].threeAC.push_back(temp);
            // cout << "assignment me " << attr3AC[c].addrName << " " << typeOfNode[attr3AC[nodeNum].addrName] << endl;
            attr3AC[nodeNum].addrName = attr3AC[c].addrName;
        }
        break;
    }

    return;
}

void execAssignmentExpression(int nodeNum){
    int c;
    switch(prodNum[nodeNum]){
        case 1:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
        case 2:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            // typeOfNode[attr3AC[nodeNum].addrName] = 
            // cout << "in assignment expression " << attr3AC[nodeNum].addrName << " " << typeOfNode[attr3AC[nodeNum].addrName] << endl;
            pushLabelUp(nodeNum,c);
            break;
    }
}

void execConditionalExpression(int nodeNum){
    int c;
    switch(prodNum[nodeNum]){
        case 1:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
        case 2:{
            c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            int c5 = adj[nodeNum][4];
            string ltrue = getLabel(c,1);
            string lfalse = getLabel(c,2);
            string nextStmt = getLabel(nodeNum,3);

            attr3AC[nodeNum] = attr3AC[c];
            string temp = "if " + attr3AC[c].addrName + " goto " + ltrue;
            (attr3AC[nodeNum].threeAC).push_back(temp);
            temp = "goto " + lfalse;
            (attr3AC[nodeNum].threeAC).push_back(temp);
            temp = ltrue + ":";
            (attr3AC[nodeNum].threeAC).push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum]+attr3AC[c3];
            temp = "goto " + nextStmt;
            (attr3AC[nodeNum].threeAC).push_back(temp);
            temp = lfalse + ":";
            (attr3AC[nodeNum].threeAC).push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum]+attr3AC[c5];
            temp = nextStmt + ":";
            (attr3AC[nodeNum].threeAC).push_back(temp);
        }
            break;
    }
}

void execConditionalOrExpression(int nodeNum){
    int c;
    switch(prodNum[nodeNum]){
        case 1:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
        case 2:{
            c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c];
            string cFalse = getLabel(c,2);
            string temp = cFalse + ":";
            (attr3AC[nodeNum].threeAC).push_back(temp);
            attr3AC[nodeNum].assemblyCode.push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c3];

            trueLabel[c] = trueLabel[nodeNum];
            trueLabel[c3] = trueLabel[nodeNum];
            falseLabel[c3] = falseLabel[nodeNum];
        }
            break;
    }
}

void execConditionalAndExpression(int nodeNum){
    int c;
    switch(prodNum[nodeNum]){
        case 1:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
        case 2:{
            c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c];
            // cout << "condaitona and " << attr3AC[c].threeAC.size() << endl;
            // string cTrue = getLabel(c,1);
            // string temp = cTrue + ":";
            // (attr3AC[nodeNum].threeAC).push_back(temp);
            attr3AC[nodeNum] = attr3AC[nodeNum] + attr3AC[c3];
            string temp = attr3AC[c].addrName + " && " + attr3AC[c3].addrName;
            attr3AC[nodeNum].threeAC.push_back(temp);
            falseLabel[c] = falseLabel[nodeNum];
            trueLabel[c3]= trueLabel[nodeNum];
            falseLabel[c3] = trueLabel[nodeNum];
        }
            break;
        
    }
    return;
}

void execInclusiveOrExpression(int nodeNum){
    int c;
    switch(prodNum[nodeNum]){
        case 1:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
        case 2:{
            c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            tempNum++;
            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            typeOfNode[attr3AC[nodeNum].addrName] = "boolean";
    

            string temp = "t" + to_string(tempNum) + " = " + attr3AC[c].addrName + " | " + attr3AC[c3].addrName;
            (attr3AC[nodeNum].threeAC).push_back(temp);
        }
            break;
    }
    return;
}

void execExclusiveOrExpression(int nodeNum){
    int c;
    switch(prodNum[nodeNum]){
        case 1:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
        case 2:{
            c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            tempNum++;
            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            typeOfNode[attr3AC[nodeNum].addrName]="boolean";

            string temp = "t" + to_string(tempNum) + " = " + attr3AC[c].addrName + " ^ " + attr3AC[c3].addrName;
            (attr3AC[nodeNum].threeAC).push_back(temp);
        }
            break;
    }
}

void execAndExpression(int nodeNum){
    int c;
    switch(prodNum[nodeNum]){
        case 1:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
        case 2:{
            c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            tempNum++;
            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            typeOfNode[attr3AC[nodeNum].addrName] = "boolean";

            string temp = "t" + to_string(tempNum) + " = " + attr3AC[c].addrName + " & " + attr3AC[c3].addrName;
            (attr3AC[nodeNum].threeAC).push_back(temp);
        }
            break;
    }
}

void execEqualityExpression(int nodeNum){
    int c;
    switch(prodNum[nodeNum]){
        case 1:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
        case 2:{
            c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            string pTrue = getLabel(nodeNum,1);
            string pFalse = getLabel(nodeNum,2);
            string temp = "if " + attr3AC[c].addrName + " == " + attr3AC[c3].addrName + " goto " + pTrue;
            attr3AC[nodeNum].threeAC.push_back(temp);
            temp = "goto " + pFalse;
            attr3AC[nodeNum].threeAC.push_back(temp);

            string t1=attr3AC[c].addrName ;
            string t2=attr3AC[c3].addrName ;
            string arg1 ;
            string arg2 ;
            if (t1[0]=='t'){
                int of1 = -8*stoi(t1.substr(1))-8;
                arg1 = to_string(of1);
                arg1+= "(%rbp)";
            }
            // else if t1[0] is integer character
            else if (t1[0] >= '0' && t1[0] <= '9'){
                arg1 = "$"+t1;
            }
            else{
                t1=varToTemp[t1];
                int of1 = -8*stoi(t1.substr(1))-8;
                arg1 = to_string(of1);
                arg1+= "(%rbp)";
            }
            
            if (t2[0]=='t'){
                int of2 = -8*stoi(t2.substr(1))-8;
                arg2 = to_string(of2);
                arg2+= "(%rbp)";
            }
            // else if t2[0] is integer character
            else if (t2[0] >= '0' && t2[0] <= '9'){
                arg2 = "$"+t2;
            }
            else{
                t2=varToTemp[t2];
                int of2 = -8*stoi(t2.substr(1))-8;
                arg2 = to_string(of2);
                arg2+= "(%rbp)";
            }
                
            string tempac1 =  "movq "+arg1+",%r8" ;
            string tempac2 =  "movq "+arg2+",%r9" ;
            string tempac3 =  "cmp %r8,%r9";
            string tempac4 =  "je "+pTrue;

            attr3AC[nodeNum].assemblyCode.push_back(tempac1);
            attr3AC[nodeNum].assemblyCode.push_back(tempac2);
            attr3AC[nodeNum].assemblyCode.push_back(tempac3);
            attr3AC[nodeNum].assemblyCode.push_back(tempac4);
            tempac1 = "jmp "+pFalse;
            attr3AC[nodeNum].assemblyCode.push_back(tempac1);

        }
            break;
        case 3:{
            c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            string pTrue = getLabel(nodeNum,1);
            string pFalse = getLabel(nodeNum,2);
            string temp = "if " + attr3AC[c].addrName + " != " + attr3AC[c3].addrName + " goto " + pTrue;
            attr3AC[nodeNum].threeAC.push_back(temp);
            temp = "goto " + pFalse;
            attr3AC[nodeNum].threeAC.push_back(temp);
           
            string t1=attr3AC[c].addrName ;
            string t2=attr3AC[c3].addrName ;
            string arg1 ;
            string arg2 ;
            if (t1[0]=='t'){
                int of1 = -8*stoi(t1.substr(1))-8;
                arg1 = to_string(of1);
                arg1+= "(%rbp)";
            }
            // else if t1[0] is integer character
            else if (t1[0] >= '0' && t1[0] <= '9'){
                arg1 = "$"+t1;
            }
            else{
                t1=varToTemp[t1];
                int of1 = -8*stoi(t1.substr(1))-8;
                arg1 = to_string(of1);
                arg1+= "(%rbp)";
            }
            
            if (t2[0]=='t'){
                int of2 = -8*stoi(t2.substr(1))-8;
                arg2 = to_string(of2);
                arg2+= "(%rbp)";
            }
            // else if t2[0] is integer character
            else if (t2[0] >= '0' && t2[0] <= '9'){
                arg2 = "$"+t2;
            }
            else{
                t2=varToTemp[t2];
                int of2 = -8*stoi(t2.substr(1))-8;
                arg2 = to_string(of2);
                arg2+= "(%rbp)";
            }
                
            string tempac1 =  "movq "+arg1+",%r8" ;
            string tempac2 =  "movq "+arg2+",%r9" ;
            string tempac3 =  "cmp %r8,%r9";
            string tempac4 =  "jne "+pTrue;

            attr3AC[nodeNum].assemblyCode.push_back(tempac1);
            attr3AC[nodeNum].assemblyCode.push_back(tempac2);
            attr3AC[nodeNum].assemblyCode.push_back(tempac3);
            attr3AC[nodeNum].assemblyCode.push_back(tempac4);
            tempac1 = "jmp "+pFalse;
            attr3AC[nodeNum].assemblyCode.push_back(tempac1);

        }
            break;
    }
    return;
}

void execRelationalExpression(int nodeNum){
    int c;
    switch(prodNum[nodeNum]){
        case 1:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
        case 2:{
            c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            string pTrue = getLabel(nodeNum,1);
            string pFalse = getLabel(nodeNum,2);
            // cout << nodeNum << " " << nodeType[nodeNum] << " " << pTrue << " " << pFalse << endl;
            string temp = "if " + attr3AC[c].addrName+ " < " + attr3AC[c3].addrName + " goto " + pTrue;
            (attr3AC[nodeNum].threeAC).push_back(temp);
            temp = "goto " + pFalse;
            (attr3AC[nodeNum].threeAC).push_back(temp);

           
            string t1=attr3AC[c].addrName ;
            string t2=attr3AC[c3].addrName ;
            string arg1 ;
            string arg2 ;
            if (t1[0]=='t'){
                int of1 = -8*stoi(t1.substr(1))-8;
                arg1 = to_string(of1);
                arg1+= "(%rbp)";
            }
            // else if t1[0] is integer character
            else if (t1[0] >= '0' && t1[0] <= '9'){
                arg1 = "$"+t1;
            }
            else{
                t1=varToTemp[t1];
                int of1 = -8*stoi(t1.substr(1))-8;
                arg1 = to_string(of1);
                arg1+= "(%rbp)";
            }
            
            if (t2[0]=='t'){
                int of2 = -8*stoi(t2.substr(1))-8;
                arg2 = to_string(of2);
                arg2+= "(%rbp)";
            }
            // else if t2[0] is integer character
            else if (t2[0] >= '0' && t2[0] <= '9'){
                arg2 = "$"+t2;
            }
            else{
                t2=varToTemp[t2];
                int of2 = -8*stoi(t2.substr(1))-8;
                arg2 = to_string(of2);
                arg2+= "(%rbp)";
            }
                
            string tempac1 =  "movq "+arg1+",%r8" ;
            string tempac2 =  "movq "+arg2+",%r9" ;
            string tempac3 =  "cmp %r8,%r9";
            string tempac4 =  "jl "+pTrue;

            attr3AC[nodeNum].assemblyCode.push_back(tempac1);
            attr3AC[nodeNum].assemblyCode.push_back(tempac2);
            attr3AC[nodeNum].assemblyCode.push_back(tempac3);
            attr3AC[nodeNum].assemblyCode.push_back(tempac4);
            tempac1 = "jmp "+pFalse;
            attr3AC[nodeNum].assemblyCode.push_back(tempac1);

        }
            break;
        case 3:{
            c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            string pTrue = getLabel(nodeNum,1);
            string pFalse = getLabel(nodeNum,2);
            string temp = "if " + attr3AC[c].addrName+ " > " + attr3AC[c3].addrName + " goto " + pTrue;
            (attr3AC[nodeNum].threeAC).push_back(temp);
            temp = "goto " + pFalse;
            (attr3AC[nodeNum].threeAC).push_back(temp);
        
            
            string t1=attr3AC[c].addrName ;
            string t2=attr3AC[c3].addrName ;
            string arg1 ;
            string arg2 ;
            if (t1[0]=='t'){
                int of1 = -8*stoi(t1.substr(1))-8;
                arg1 = to_string(of1);
                arg1+= "(%rbp)";
            }
            // else if t1[0] is integer character
            else if (t1[0] >= '0' && t1[0] <= '9'){
                arg1 = "$"+t1;
            }
            else{
                t1=varToTemp[t1];
                int of1 = -8*stoi(t1.substr(1))-8;
                arg1 = to_string(of1);
                arg1+= "(%rbp)";
            }
            
            if (t2[0]=='t'){
                int of2 = -8*stoi(t2.substr(1))-8;
                arg2 = to_string(of2);
                arg2+= "(%rbp)";
            }
            // else if t2[0] is integer character
            else if (t2[0] >= '0' && t2[0] <= '9'){
                arg2 = "$"+t2;
            }
            else{
                t2=varToTemp[t2];
                int of2 = -8*stoi(t2.substr(1))-8;
                arg2 = to_string(of2);
                arg2+= "(%rbp)";
            }
                
            string tempac1 =  "movq "+arg1+",%r8" ;
            string tempac2 =  "movq "+arg2+",%r9" ;
            string tempac3 =  "cmp %r8,%r9";
            string tempac4 =  "jg "+pTrue;

            attr3AC[nodeNum].assemblyCode.push_back(tempac1);
            attr3AC[nodeNum].assemblyCode.push_back(tempac2);
            attr3AC[nodeNum].assemblyCode.push_back(tempac3);
            attr3AC[nodeNum].assemblyCode.push_back(tempac4);
            tempac1 = "jmp "+pFalse;
            attr3AC[nodeNum].assemblyCode.push_back(tempac1);

        }
            break;
        case 4:{
            {
            c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            string pTrue = getLabel(nodeNum,1);
            string pFalse = getLabel(nodeNum,2);
            string temp = "if " + attr3AC[c].addrName+ " <= " + attr3AC[c3].addrName + " goto " + pTrue;
            (attr3AC[nodeNum].threeAC).push_back(temp);
            temp = "goto " + pFalse;
            (attr3AC[nodeNum].threeAC).push_back(temp);

            
            string t1=attr3AC[c].addrName ;
            string t2=attr3AC[c3].addrName ;
            string arg1 ;
            string arg2 ;
            if (t1[0]=='t'){
                int of1 = -8*stoi(t1.substr(1))-8;
                arg1 = to_string(of1);
                arg1+= "(%rbp)";
            }
            // else if t1[0] is integer character
            else if (t1[0] >= '0' && t1[0] <= '9'){
                arg1 = "$"+t1;
            }
            else{
                t1=varToTemp[t1];
                int of1 = -8*stoi(t1.substr(1))-8;
                arg1 = to_string(of1);
                arg1+= "(%rbp)";
            }
            
            if (t2[0]=='t'){
                int of2 = -8*stoi(t2.substr(1))-8;
                arg2 = to_string(of2);
                arg2+= "(%rbp)";
            }
            // else if t2[0] is integer character
            else if (t2[0] >= '0' && t2[0] <= '9'){
                arg2 = "$"+t2;
            }
            else{
                t2=varToTemp[t2];
                int of2 = -8*stoi(t2.substr(1))-8;
                arg2 = to_string(of2);
                arg2+= "(%rbp)";
            }
                
            string tempac1 =  "movq "+arg1+",%r8" ;
            string tempac2 =  "movq "+arg2+",%r9" ;
            string tempac3 =  "cmp %r8,%r9";
            string tempac4 =  "jle "+pTrue;

            attr3AC[nodeNum].assemblyCode.push_back(tempac1);
            attr3AC[nodeNum].assemblyCode.push_back(tempac2);
            attr3AC[nodeNum].assemblyCode.push_back(tempac3);
            attr3AC[nodeNum].assemblyCode.push_back(tempac4);
            tempac1 = "jmp "+pFalse;
            attr3AC[nodeNum].assemblyCode.push_back(tempac1);

        }
        }
            break;
        case 5:{
            c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            string pTrue = getLabel(nodeNum,1);
            string pFalse = getLabel(nodeNum,2);
            string temp = "if " + attr3AC[c].addrName+ " >= " + attr3AC[c3].addrName + " goto " + pTrue;
            (attr3AC[nodeNum].threeAC).push_back(temp);
            temp = "goto " + pFalse;
            (attr3AC[nodeNum].threeAC).push_back(temp);
                        
            
            string t1=attr3AC[c].addrName ;
            string t2=attr3AC[c3].addrName ;
            string arg1 ;
            string arg2 ;
            if (t1[0]=='t'){
                int of1 = -8*stoi(t1.substr(1))-8;
                arg1 = to_string(of1);
                arg1+= "(%rbp)";
            }
            // else if t1[0] is integer character
            else if (t1[0] >= '0' && t1[0] <= '9'){
                arg1 = "$"+t1;
            }
            else{
                t1=varToTemp[t1];
                int of1 = -8*stoi(t1.substr(1))-8;
                arg1 = to_string(of1);
                arg1+= "(%rbp)";
            }
            
            if (t2[0]=='t'){
                int of2 = -8*stoi(t2.substr(1))-8;
                arg2 = to_string(of2);
                arg2+= "(%rbp)";
            }
            // else if t2[0] is integer character
            else if (t2[0] >= '0' && t2[0] <= '9'){
                arg2 = "$"+t2;
            }
            else{
                t2=varToTemp[t2];
                int of2 = -8*stoi(t2.substr(1))-8;
                arg2 = to_string(of2);
                arg2+= "(%rbp)";
            }
                
            string tempac1 =  "movq "+arg1+",%r8" ;
            string tempac2 =  "movq "+arg2+",%r9" ;
            string tempac3 =  "cmp %r8,%r9";
            string tempac4 =  "jge "+pTrue;

            attr3AC[nodeNum].assemblyCode.push_back(tempac1);
            attr3AC[nodeNum].assemblyCode.push_back(tempac2);
            attr3AC[nodeNum].assemblyCode.push_back(tempac3);
            attr3AC[nodeNum].assemblyCode.push_back(tempac4);
            tempac1 = "jmp "+pFalse;
            attr3AC[nodeNum].assemblyCode.push_back(tempac1);

        }
            break;
        case 6:{
            c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            string pTrue = getLabel(nodeNum,1);
            string pFalse = getLabel(nodeNum,2);
            string temp = "if " + attr3AC[c].addrName+ " < " + attr3AC[c3].addrName + " instanceof " + pTrue;
            (attr3AC[nodeNum].threeAC).push_back(temp);
            temp = "goto " + pFalse;
            (attr3AC[nodeNum].threeAC).push_back(temp);
        }
            break;
    }
    return;
}

void execShiftExpression(int nodeNum){
    int c;
    switch(prodNum[nodeNum]){
        case 1:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
        case 2:{
            c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            tempNum++;
            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            typeOfNode[attr3AC[nodeNum].addrName] = "int";
            
            string temp = "t" + to_string(tempNum) + " = " + attr3AC[c].addrName + " << " + attr3AC[c3].addrName;
            (attr3AC[nodeNum].threeAC).push_back(temp);

            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            
            // cout<<attr3AC[nodeNum].addrName<<"\n";

            // string arg1 = varToTemp[attr3AC[nodeNum].addrName];
            string arg2 = varToTemp[attr3AC[c].addrName];
            string arg3 = varToTemp[attr3AC[c3].addrName];

            // if(arg1.size()==0) arg1 = attr3AC[nodeNum].addrName;
            if(varToTemp.find(attr3AC[c].addrName)== varToTemp.end()){ arg2 = attr3AC[c].addrName; }
            if(varToTemp.find(attr3AC[c3].addrName)==varToTemp.end()){ arg3 = attr3AC[c3].addrName; }

            if(arg2=="") { arg2 = attr3AC[c].addrName; }
            if(arg3=="") { arg3 = attr3AC[c3].addrName; }

            auto x = getLeftShiftAssemblyCode(attr3AC[nodeNum].addrName, arg2, arg3);

            for(auto el:x){
                // cout<<el<<"\n";
                attr3AC[nodeNum].assemblyCode.push_back(el);
            }
        }
            break;
        case 3:{
            c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            tempNum++;
            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            typeOfNode[attr3AC[nodeNum].addrName] = "int";

            string temp = "t" + to_string(tempNum) + " = " + attr3AC[c].addrName + " >> " + attr3AC[c3].addrName;
            (attr3AC[nodeNum].threeAC).push_back(temp);

            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            
            // cout<<attr3AC[nodeNum].addrName<<"\n";

            // string arg1 = varToTemp[attr3AC[nodeNum].addrName];
            string arg2 = varToTemp[attr3AC[c].addrName];
            string arg3 = varToTemp[attr3AC[c3].addrName];

            // if(arg1.size()==0) arg1 = attr3AC[nodeNum].addrName;
            if(varToTemp.find(attr3AC[c].addrName)== varToTemp.end()){ arg2 = attr3AC[c].addrName; }
            if(varToTemp.find(attr3AC[c3].addrName)==varToTemp.end()){ arg3 = attr3AC[c3].addrName; }

            if(arg2=="") { arg2 = attr3AC[c].addrName; }
            if(arg3=="") { arg3 = attr3AC[c3].addrName; }

            auto x = getRightShiftAssemblyCode(attr3AC[nodeNum].addrName, arg2, arg3);

            for(auto el:x){
                // cout<<el<<"\n";
                attr3AC[nodeNum].assemblyCode.push_back(el);
            }
        }
            break;
        case 4:{
            c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            tempNum++;
            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);

            typeOfNode[attr3AC[nodeNum].addrName] = "int";

            string temp = "t" + to_string(tempNum) + " = " + attr3AC[c].addrName + " >>> " + attr3AC[c3].addrName;
            (attr3AC[nodeNum].threeAC).push_back(temp);

            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            
            // cout<<attr3AC[nodeNum].addrName<<"\n";

            // string arg1 = varToTemp[attr3AC[nodeNum].addrName];
            string arg2 = varToTemp[attr3AC[c].addrName];
            string arg3 = varToTemp[attr3AC[c3].addrName];

            // if(arg1.size()==0) arg1 = attr3AC[nodeNum].addrName;
            if(varToTemp.find(attr3AC[c].addrName)== varToTemp.end()){ arg2 = attr3AC[c].addrName; }
            if(varToTemp.find(attr3AC[c3].addrName)==varToTemp.end()){ arg3 = attr3AC[c3].addrName; }

            if(arg2=="") { arg2 = attr3AC[c].addrName; }
            if(arg3=="") { arg3 = attr3AC[c3].addrName; }

            auto x = getUnsignedRightShiftAssemblyCode(attr3AC[nodeNum].addrName, arg2, arg3);

            for(auto el:x){
                // cout<<el<<"\n";
                attr3AC[nodeNum].assemblyCode.push_back(el);
            }
        }
            break;
    }
}

void execAdditiveExpression(int nodeNum){
    int c;
    switch(prodNum[nodeNum]){
        case 1:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
        case 2:{
            c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            //widen
            widenNode(c,c3);
            string tp;
            tp = getTypeNode(c);
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            tempNum++;
            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);

            // cout<<"just after init"<<attr3AC[nodeNum].addrName<<"\n";

            string temp = "t" + to_string(tempNum) + " = " + attr3AC[c].addrName + " + " +tp + " " + attr3AC[c3].addrName;
            typeOfNode[attr3AC[nodeNum].addrName]=tp;
            (attr3AC[nodeNum].threeAC).push_back(temp);
            
            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            
            // cout<<attr3AC[nodeNum].addrName<<"\n";

            // string arg1 = varToTemp[attr3AC[nodeNum].addrName];
            string arg2 = varToTemp[attr3AC[c].addrName];
            string arg3 = varToTemp[attr3AC[c3].addrName];

            // if(arg1.size()==0) arg1 = attr3AC[nodeNum].addrName;
            if(varToTemp.find(attr3AC[c].addrName)== varToTemp.end()){ arg2 = attr3AC[c].addrName; }
            if(varToTemp.find(attr3AC[c3].addrName)==varToTemp.end()){ arg3 = attr3AC[c3].addrName; }

            if(arg2=="") { arg2 = attr3AC[c].addrName; }
            if(arg3=="") { arg3 = attr3AC[c3].addrName; }

            // cout << "obafasfsadfsafsd hre " << attr3AC[nodeNum].addrName << " " << arg2 << " " << arg3 << endl;
            auto x = getAddAssemblyCode(attr3AC[nodeNum].addrName, arg2, arg3);

            for(auto el:x){
                // cout<<el<<"\n";
                attr3AC[nodeNum].assemblyCode.push_back(el);
            }
        }
            break;
        case 3:{
            c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            widenNode(c,c3);
            tempNum++;
            string tp = getTypeNode(c);
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            string temp = "t" + to_string(tempNum) + " = " + attr3AC[c].addrName + " -"+tp+" " + attr3AC[c3].addrName;
            typeOfNode[attr3AC[nodeNum].addrName] = tp;
            (attr3AC[nodeNum].threeAC).push_back(temp);

            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            
            // cout<<attr3AC[nodeNum].addrName<<"\n";

            string arg2 = varToTemp[attr3AC[c].addrName];
            string arg3 = varToTemp[attr3AC[c3].addrName];

            // if(arg1.size()==0) arg1 = attr3AC[nodeNum].addrName;
            if(varToTemp.find(attr3AC[c].addrName)== varToTemp.end()){ arg2 = attr3AC[c].addrName; }
            if(varToTemp.find(attr3AC[c3].addrName)==varToTemp.end()){ arg3 = attr3AC[c3].addrName; }

            if(arg2=="") { arg2 = attr3AC[c].addrName; }
            if(arg3=="") { arg3 = attr3AC[c3].addrName; }

            auto x = getSubAssemblyCode(attr3AC[nodeNum].addrName, arg2, arg3);

            for(auto el:x){
                // cout<<el<<"\n";
                attr3AC[nodeNum].assemblyCode.push_back(el);
            }
        }
            break;
    }
    return;
}

void execMultiplicativeExpression(int nodeNum){
    int c;
    switch(prodNum[nodeNum]){
        case 1:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
        case 2:{
            c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            widenNode(c,c3);
            tempNum++;
            string tp = getTypeNode(c);
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            string temp = "t" + to_string(tempNum) + " = " + attr3AC[c].addrName + " *" + tp + " " + attr3AC[c3].addrName;
            typeOfNode[attr3AC[nodeNum].addrName]=tp;
            (attr3AC[nodeNum].threeAC).push_back(temp);

            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            
            // cout<<attr3AC[nodeNum].addrName<<"\n";
            string arg2 = varToTemp[attr3AC[c].addrName];
            string arg3 = varToTemp[attr3AC[c3].addrName];

            // if(arg1.size()==0) arg1 = attr3AC[nodeNum].addrName;
            if(varToTemp.find(attr3AC[c].addrName)== varToTemp.end()){ arg2 = attr3AC[c].addrName; }
            if(varToTemp.find(attr3AC[c3].addrName)==varToTemp.end()){ arg3 = attr3AC[c3].addrName; }

            if(arg2=="") { arg2 = attr3AC[c].addrName; }
            if(arg3=="") { arg3 = attr3AC[c3].addrName; }

            auto x = getMulAssemblyCode(attr3AC[nodeNum].addrName, arg2, arg3);

            for(auto el:x){
                // cout<<el<<"\n";
                attr3AC[nodeNum].assemblyCode.push_back(el);
            }
        }
            break;
        case 3:{
            c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            widenNode(c,c3);
            tempNum++;
            string tp = getTypeNode(c);
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            string temp = "t" + to_string(tempNum) + " = " + attr3AC[c].addrName + " /"+ tp + " " + attr3AC[c3].addrName;
            typeOfNode[attr3AC[nodeNum].addrName]=tp;
            (attr3AC[nodeNum].threeAC).push_back(temp);

            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            
            string arg2 = varToTemp[attr3AC[c].addrName];
            string arg3 = varToTemp[attr3AC[c3].addrName];

            // if(arg1.size()==0) arg1 = attr3AC[nodeNum].addrName;
            if(varToTemp.find(attr3AC[c].addrName)== varToTemp.end()){ arg2 = attr3AC[c].addrName; }
            if(varToTemp.find(attr3AC[c3].addrName)==varToTemp.end()){ arg3 = attr3AC[c3].addrName; }
            // cout << attr3AC[c].addrName << "idhaf" << endl;
            if(attr3AC[c].addrName[0]>='0' && attr3AC[c].addrName[0]<='9') { arg2 = attr3AC[c].addrName; }
            if(attr3AC[c3].addrName[0]>='0' && attr3AC[c3].addrName[0]<='9') { arg3 = attr3AC[c3].addrName; }

            auto x = getDivAssemblyCode(attr3AC[nodeNum].addrName, arg2, arg3);

            for(auto el:x){
                // cout<<el<<"\n";
                attr3AC[nodeNum].assemblyCode.push_back(el);
            }
        }
            break;
        case 4:{
            c = adj[nodeNum][0];
            int c3 = adj[nodeNum][2];
            widenNode(c,c3);
            tempNum++;
            string tp = getTypeNode(c);
            attr3AC[nodeNum] = attr3AC[c]+attr3AC[c3];
            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            string temp = "t" + to_string(tempNum) + " = " + attr3AC[c].addrName + " % " + attr3AC[c3].addrName;
            typeOfNode[attr3AC[nodeNum].addrName] = tp;
            (attr3AC[nodeNum].threeAC).push_back(temp);

            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            
            string arg2 = varToTemp[attr3AC[c].addrName];
            string arg3 = varToTemp[attr3AC[c3].addrName];

            // if(arg1.size()==0) arg1 = attr3AC[nodeNum].addrName;
            if(varToTemp.find(attr3AC[c].addrName)== varToTemp.end()){ arg2 = attr3AC[c].addrName; }
            if(varToTemp.find(attr3AC[c3].addrName)==varToTemp.end()){ arg3 = attr3AC[c3].addrName; }
            // cout << attr3AC[c].addrName << "idhaf" << endl;
            if(attr3AC[c].addrName[0]>='0' && attr3AC[c].addrName[0]<='9') { arg2 = attr3AC[c].addrName; }
            if(attr3AC[c3].addrName[0]>='0' && attr3AC[c3].addrName[0]<='9') { arg3 = attr3AC[c3].addrName; }

            auto x = getPerAssemblyCode(attr3AC[nodeNum].addrName, arg2, arg3);

            for(auto el:x){
                // cout<<el<<"\n";
                attr3AC[nodeNum].assemblyCode.push_back(el);
            }
        }
            break;
    }
    return;
}

void execUnaryExpression(int nodeNum){
    int c;
    switch(prodNum[nodeNum]){
        case 1:{
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
            break;
        case 2:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
        case 3:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
        case 4:{
            c = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c];
            tempNum++;
            string tp;
            if(typeOfNode.find(to_string(attr3AC[c].nodeno))!=typeOfNode.end()){
                tp = typeOfNode[to_string(attr3AC[c].nodeno)];
            }
            if(attr3AC[c].addrName.size()>0 && typeOfNode.find(attr3AC[c].addrName)!=typeOfNode.end()){
                tp = typeOfNode[attr3AC[c].addrName];
            }
            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            string temp = "t" + to_string(tempNum) + " =- " + attr3AC[c].addrName;
            typeOfNode[attr3AC[nodeNum].addrName] = tp;
            (attr3AC[nodeNum].threeAC).push_back(temp);

            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            
            // cout<<attr3AC[nodeNum].addrName<<"\n";

            // string arg2 = varToTemp[attr3AC[c].addrName];
            // if(varToTemp.find(attr3AC[c].addrName)== varToTemp.end()){ arg2 = attr3AC[c].addrName; }
            // if(arg2=="") { arg2 = attr3AC[c].addrName; }

            auto x = getMinusUnaryExpressionAssemblyCode(attr3AC[nodeNum].addrName);

            for(auto el:x){
                // cout<<el<<"\n";
                attr3AC[nodeNum].assemblyCode.push_back(el);
            }
        }
            break;
        case 5:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
    }
    return;
}

void execUnaryExpressionNotPlusMinus(int nodeNum){
    int c;
    switch(prodNum[nodeNum]){
        case 1:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
        case 2:{
            c = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c];
            tempNum++;
            string tp;
            if(typeOfNode.find(to_string(attr3AC[c].nodeno))!=typeOfNode.end()){
                tp = typeOfNode[to_string(attr3AC[c].nodeno)];
            }
            if(attr3AC[c].addrName.size()>0 && typeOfNode.find(attr3AC[c].addrName)!=typeOfNode.end()){
                tp = typeOfNode[attr3AC[c].addrName];
            }
            attr3AC[nodeNum].addrName = "t" + to_string(tempNum);
            string temp = "t" + to_string(tempNum) + " =~ " + attr3AC[c].addrName;
            typeOfNode[attr3AC[nodeNum].addrName] = tp;
            (attr3AC[nodeNum].threeAC).push_back(temp);

            

            pushLabelUp(nodeNum,c);
        }
            break;
        case 3:{
            c = adj[nodeNum][1];
            attr3AC[nodeNum] = attr3AC[c];
            trueLabel[c] = falseLabel[nodeNum];
            falseLabel[c] = trueLabel[nodeNum];
            pushLabelUp(nodeNum,c);
        }
            break;
        case 4:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
    }

    return;
}

void execPostfixExpression(int nodeNum){
    int c;
    switch(prodNum[nodeNum]){
        case 1:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
        case 2:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
        case 3:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
        case 4:
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
    }
}

void execLeftHandSide(int nodeNum){
    switch(prodNum[nodeNum]){
        case 1:{
            int c = adj[nodeNum][0];//Get child node number
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            // cout << "LeftHandSide " << attr3AC[nodeNum].addrName << " " << attr3AC[c].addrName << endl;
            break;
        }
        break;
        case 2:{
            int c = adj[nodeNum][0];//Get child node number
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            if(attr3AC[nodeNum].isthis==1){
                string temp = "*("+funcParamTemp["this"]+" + "+ attr3AC[c].addrName +")";
                // string temp = funcParamTemp["this"] +")";
                attr3AC[nodeNum].addrName = temp;
            }
            break;
        }
        break;
        case 3:{
            int c = adj[nodeNum][0];//Get child node number
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
        }
        break;
    }
    return;
}

void execName(int nodeNum){
    int c;
    switch(prodNum[nodeNum]){
        case 1:{
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            pushLabelUp(nodeNum,c);
            break;
        }
        case 2:{
            c = adj[nodeNum][0];
            attr3AC[nodeNum] = attr3AC[c];
            // cout << "in name " << attr3AC[c].threeAC[0] << endl;
            pushLabelUp(nodeNum,c);
            break;
        }
    }
    return;
}

void execSimpleName(int nodeNum){
    int c = adj[nodeNum][0];//Get child node number
    attr3AC[nodeNum] = attr3AC[c];
    pushLabelUp(nodeNum,c);
    // cout << "Simplename " << attr3AC[nodeNum].addrName << " " << attr3AC[c].addrName << endl;
    return;
}

void execIdentifier(int nodeNum){
    //initialize varToTemp
    int c = adj[nodeNum][0];//Get child node number
    if(varToTemp.find(nodeType[c])==varToTemp.end()){
        tempNum++;
        string temp = "t" + to_string(tempNum);
        varToTemp[nodeType[c]] = temp;
    }
    attr3AC[nodeNum].addrName = nodeType[c];//Add addrname of identifier 
    attr3AC[nodeNum].nodeno = c;
    if(inStatement && !(inMethodInvocation && inMN))checkIfDeclared(c,nodeType[c]);
    if(funcParamTemp.find(nodeType[c])!=funcParamTemp.end())attr3AC[nodeNum].addrName = funcParamTemp[nodeType[c]];
    return;
}

void labelStatement(int nodeNum){
    getLabel(adj[nodeNum][0],3);
    return;
}
void labelStatementNoShortIf(int nodeNum){
    getLabel(adj[nodeNum][0],3);
    return;
}

void labelStatementWithoutTrailingSubstatement(int nodeNum){
    if(prodNum[nodeNum]==5){
        getLabel(adj[nodeNum][4],3);
    }
}

void labelConditionalAndExpression(int nodeNum){
    if(prodNum[nodeNum]==2){
        getLabel(nodeNum,1);
        getLabel(nodeNum,2);
        falseLabel[adj[nodeNum][0]] = falseLabel[nodeNum];
        trueLabel[adj[nodeNum][2]] = trueLabel[nodeNum];
        falseLabel[adj[nodeNum][2]] = falseLabel[nodeNum];
    }
    return;
}

void labelConditionalOrExpression(int nodeNum){
    if(prodNum[nodeNum]==2){
        getLabel(nodeNum,1);
        getLabel(nodeNum,2);
        trueLabel[adj[nodeNum][0]] = trueLabel[nodeNum];
        trueLabel[adj[nodeNum][2]] = trueLabel[nodeNum];
        falseLabel[adj[nodeNum][2]]= falseLabel[nodeNum];
    }
    return;
}

void labelIfElse(int nodeNum){
    getLabel(adj[nodeNum][2],1);
    getLabel(adj[nodeNum][2],2);
    nextLabel[adj[nodeNum][4]] = nextLabel[nodeNum];
    nextLabel[adj[nodeNum][6]] = nextLabel[nodeNum];
    return;
}

void labelIf(int nodeNum){
    getLabel(adj[nodeNum][2],1);
    falseLabel[adj[nodeNum][2]]=nextLabel[nodeNum];
    nextLabel[adj[nodeNum][4]] = nextLabel[nodeNum];
    return;
}

void labelWhile(int nodeNum){
    getLabel(adj[nodeNum][2],1);
    whileContinueLabel = getLabel(-1,0);//get label for continue
    falseLabel[adj[nodeNum][2]]=nextLabel[nodeNum];
    // cout << "idhar while ke label banana he " << nextLabel[nodeNum];
    
    whileBreakLabel = "L" + to_string(nextLabel[nodeNum]);
    return;
}

void labelFor(int nodeNum){
    forContinueLabel = getLabel(-1,0);
    forBreakLabel = "L" + to_string(nextLabel[nodeNum]);
    switch(prodNum[nodeNum]){
        case 1:{
            getLabel(adj[nodeNum][4],1);
            falseLabel[adj[nodeNum][4]]=nextLabel[nodeNum];
        }
        break;
        case 2:{
            getLabel(adj[nodeNum][3],1);
            falseLabel[adj[nodeNum][3]]=nextLabel[nodeNum];
        }
        break;
        case 3:
        break;
        case 4:{
            getLabel(adj[nodeNum][4],1);
            falseLabel[adj[nodeNum][4]]=nextLabel[nodeNum];
        }
        break;
        case 5:
        break;
        case 6:{
            getLabel(adj[nodeNum][3],1);
            falseLabel[adj[nodeNum][3]]=nextLabel[nodeNum];
        }
        break;
        case 7:
        break;
    }
}

void labelDo(int nodeNum){
    getLabel(adj[nodeNum][4],1);
    doContinueLabel = getLabel(-1,0);
    falseLabel[adj[nodeNum][4]] = nextLabel[nodeNum];
    doBreakLabel = "L" + to_string(nextLabel[nodeNum]);
    return;
}

void labelNot(int nodeNum){
    trueLabel[adj[nodeNum][1]] = falseLabel[nodeNum];
    falseLabel[adj[nodeNum][1]] = trueLabel[nodeNum];
    return;
}

void generateLabels(int nodeNum){
    string s = nodeType[nodeNum];
    // cout<<"going to "<<s<<endl;
    if("Statement"==s && (prodNum[nodeNum]==3 || prodNum[nodeNum]==4 || prodNum[nodeNum]==5 || prodNum[nodeNum]==6)){
        labelStatement(nodeNum);
    }else if("StatementNoShortIf"==s && (prodNum[nodeNum]==3 || prodNum[nodeNum]==4 || prodNum[nodeNum]==5)){
        labelStatementNoShortIf(nodeNum);
    }else if("ConditionalAndExpression"==s){
        labelConditionalAndExpression(nodeNum);
    }else if("ConditionalOrExpression"==s){
        labelConditionalOrExpression(nodeNum);
    }else if("IfThenElseStatement"==s || "IfThenElseStatementNoShortIf"==s){
        labelIfElse(nodeNum);
    }else if("IfThenStatement" == s){
        labelIf(nodeNum);
    }else if("WhileStatement"==s || "WhileStatementNoShortIf"==s){
        labelWhile(nodeNum);
    }else if("ForStatement"==s || "ForStatementNoShortIf"==s){
        labelFor(nodeNum);
    }else if("DoStatement"==s){
        labelDo(nodeNum);
    }else if("StatementWithoutTrailingSubstatement"==s){
        labelStatementWithoutTrailingSubstatement(nodeNum);
    }else if("UnaryExpressionNotPlusMinus"==s && prodNum[nodeNum]==3){
        labelNot(nodeNum);
    }
    // if(nodeNum==127)cout << "ihfadf " << nodeType[nodeNum] << endl;
    if(adj[nodeNum].size()==1){
        // if(nodeNum==127)cout << "here " << trueLabel[nodeNum] << endl;
        // if(adj[nodeNum][0]==118)cout<< " asdfsadfasdf " << adj[nodeNum][0] << " " << trueLabel[nodeNum] << endl;
        if(trueLabel.find(nodeNum)!=trueLabel.end())trueLabel[adj[nodeNum][0]] = trueLabel[nodeNum];
        if(falseLabel.find(nodeNum)!=falseLabel.end())falseLabel[adj[nodeNum][0]] = falseLabel[nodeNum];
        if(nextLabel.find(nodeNum)!=nextLabel.end())nextLabel[adj[nodeNum][0]] = nextLabel[nodeNum];
    }
    for(int i=0;i<adj[nodeNum].size();i++){
        generateLabels(adj[nodeNum][i]);
    }
    return;
}

void postOrderTraversal3AC(int nodeNum){
    // cout<<nodeType[nodeNum]<<endl;
    if(nodeType[nodeNum]=="ClassBody"){
        insideClassName = classNameMap[nodeNum];
        // cout << "initialized insideclassname " << insideClassName << " " << nodeType[nodeNum] << " " << nodeNum << endl;
    }

    if(nodeType[nodeNum]=="Statement"){
        inStatement=1;
    }
    if (nodeType[nodeNum]=="FormalParameterList"){
        inFormalParameterList=true;
    }
    if (nodeType[nodeNum]=="ForInit"){
        inForInit=true;
        vector<string> temp;
        ForInitVars.push_back(temp);
    }
    if(nodeType[nodeNum]=="MethodInvocation"){
        inMethodInvocation=1;
    }
    if(nodeType[nodeNum]=="Name"){
        inMN=1;
    }

    if(nodeType[nodeNum]=="WhileStatement"){
        isWhile=1;
    }
    if(nodeType[nodeNum]=="DoStatement"){
        isDo=1;
    }
    if(nodeType[nodeNum]=="ForStatement" || nodeType[nodeNum]=="ForStatementNoShortIf"){
        isFor = prodNum[nodeNum];
    }

    if ( nodeType[nodeNum]=="IfThenStatement"){
        // new scope starting
        // cout<<"Hello\n";
        attr3AC[nodeNum].threeAC.push_back("pushonstack oldstackpointer");
        attr3AC[nodeNum].threeAC.push_back("stackpointer +8");
        attr3AC[nodeNum].threeAC.push_back("oldstackpointer = stackpointer");

    }
    // cout << "NODENUM " << nodeNum << " " << nodeType[nodeNum] << endl;
    for(int i=0;i<adj[nodeNum].size();i++){
        // cout<<"I am "<<nodeType[nodeNum]<<" calling child:"<<nodeType[adj[nodeNum][i]]<<endl;
        postOrderTraversal3AC(adj[nodeNum][i]);
        // if("SwitchStatement"==nodeType[nodeNum] && i==2){
        //     switchExpAddr = someExpAddr;
            // cout << "inside possafsdfas df " << switchExpAddr << endl;
        // }
    }
    if(nodeType[nodeNum]=="WhileStatement"){
        isWhile=0;
    }
    if(nodeType[nodeNum]=="DoStatement"){
        isDo=0;
    }
    if(nodeType[nodeNum]=="ForStatement" || nodeType[nodeNum]=="ForStatementNoShortIf"){
        isFor = 0;
    }
    string s = nodeType[nodeNum];
    if(adj[nodeNum].size()==0)return;
    if("CompilationUnit" == s){
        execCompilationUnit(nodeNum);
        // cout<<"finiss\n";
    }else if("TypeDeclarations"==s){
        execTypeDeclarations(nodeNum);
    }else if("TypeDeclaration"==s){
        execTypeDeclaration(nodeNum);
    }else if("ClassDeclaration" == s){
        execClassDeclaration(nodeNum);
    }else if("ClassBody" == s){
        execClassBody(nodeNum);
        insideClassName.clear();
    }else if("ClassBodyDeclarations" == s){
        execClassBodyDeclarations(nodeNum);
    }else if("ClassBodyDeclaration" == s){
        execClassBodyDeclaration(nodeNum);
    }else if("ClassMemberDeclaration" == s){
        execClassMemberDeclaration(nodeNum);
    }else if("MethodDeclaration" == s){
        execMethodDeclaration(nodeNum);
        funcParamTemp.clear();
        // vector<localTableParams>* fTabPtr = scopeAndTable[attr3AC[nodeNum].nodeno].second;

        // for (int i=(*fTabPtr).size()-1;i>=0;i--){
        //         auto fRow = (*fTabPtr)[i];
        //         if (fRow.scope.first==3 && fRow.arraySize.size()==0){
        //                 string temp3AC="stackpointer -"+to_string(typeSize[fRow.type])+"\npopfromstack "+fRow.name;
        //                 attr3AC[nodeNum].threeAC.push_back(temp3AC);
        //         }
        // }

    }else if("MethodBody" == s){
        execMethodBody(nodeNum);
    }else if("Block" == s){
        execBlock(nodeNum);
    }else if("BlockStatements" == s){
        execBlockStatements(nodeNum);
    }else if("BlockStatement" == s){
        execBlockStatement(nodeNum);
    }else if("Statement" == s){
        execStatement(nodeNum);
        inStatement=0;
    }else if("IfThenElseStatement" == s){
        execIfThenElseStatement(nodeNum);
    }else if("IfThenStatement" == s){
        execIfThenStatement(nodeNum);
    }else if("StatementWithoutTrailingSubstatement" == s){
        execStatementWithoutTrailingSubstatement(nodeNum);
    }else if("ExpressionStatement" == s){
        execExpressionStatement(nodeNum);
    }else if("StatementExpression" == s){
        execStatementExpression(nodeNum);
    }else if("Primary" == s){
        execPrimary(nodeNum);
    }else if("ArrayAccess" == s){
        execArrayAccess(nodeNum);
    }else if("DimExprs" == s){
        execDimExprs(nodeNum);
    }else if("DimExpr" == s){
        execDimExpr(nodeNum);
    }else if("Dims" == s){
        execDims(nodeNum);
    }else if("PrimaryNoNewArray" == s){
        execPrimaryNoNewArray(nodeNum);
    }else if("MethodInvocation" == s){
        execMethodInvocation(nodeNum);
        // cout<<"outta MethodInvocation\n";
        inMethodInvocation=0;
    }else if("VariableInitializer" == s){
        execVariableInitializer(nodeNum);
    }else if("ArgumentList" == s){
        execArgumentList(nodeNum);
    }else if("Expression" == s){
        execExpression(nodeNum);
    }else if("Assignment" == s){
        execAssignment(nodeNum);
    }else if("AssignmentExpression" == s){
        execAssignmentExpression(nodeNum);
    }else if("ConditionalExpression" == s){
        execConditionalExpression(nodeNum);
    }else if("ConditionalOrExpression" == s){
        execConditionalOrExpression(nodeNum);
    }else if("ConditionalAndExpression" == s){
        execConditionalAndExpression(nodeNum);
    }else if("InclusiveOrExpression" == s){
        execInclusiveOrExpression(nodeNum);
    }else if("ExclusiveOrExpression" == s){
        execExclusiveOrExpression(nodeNum);
    }else if("AndExpression" == s){
        execAndExpression(nodeNum);
    }else if("EqualityExpression" == s){
        execEqualityExpression(nodeNum);
    }else if("RelationalExpression" == s){
        execRelationalExpression(nodeNum);
    }else if("ShiftExpression" == s){
        execShiftExpression(nodeNum);
    }else if("AdditiveExpression" == s){
        execAdditiveExpression(nodeNum);
    }else if("MultiplicativeExpression" == s){
        execMultiplicativeExpression(nodeNum);
    }else if("UnaryExpression" == s){
        execUnaryExpression(nodeNum);
    }else if("PostfixExpression" == s){
        execPostfixExpression(nodeNum);
    }else if("Name" == s){
        execName(nodeNum);
        inMN = 0;
    }else if("SimpleName" == s){
        execSimpleName(nodeNum);
    }else if("Identifier" == s){
        execIdentifier(nodeNum);
    }else if("LeftHandSide" == s){
        execLeftHandSide(nodeNum);
    }else if("UnaryExpressionNotPlusMinus" == s){
        execUnaryExpressionNotPlusMinus(nodeNum);
    }else if("StatementNoShortIf" == s){
        execStatementNoShortIf(nodeNum);
    }else if("WhileStatement" == s){
        execWhileStatement(nodeNum);
    }else if("ForStatement" == s){
        execForStatement(nodeNum);
    }else if("AbstractMethodDeclaration" == s){
        execAbstractMethodDeclaration(nodeNum);
    }else if("ArrayCreationExpression" == s){
        execArrayCreationExpression(nodeNum);
    }else if("ArrayInitializer" == s){
        execArrayInitializer(nodeNum);
    }else if("ArrayType" == s){
        execArrayType(nodeNum);
    }else if("AssignmentOperator" == s){
        execAssignmentOperator(nodeNum);
    }else if("BooleanLiteral" == s){
        execBooleanLiteral(nodeNum);
    }else if("BreakStatement" == s){
        execBreakStatement(nodeNum);
    }else if("CatchClause" == s){
        execCatchClause(nodeNum);
    }else if("Catches"== s){
        execCatches(nodeNum);
    }else if("CharacterLiteral" == s){
        execCharacterLiteral(nodeNum);
    }else if("ClassInstanceCreationExpression" == s){
        execClassInstanceCreationExpression(nodeNum);
    }else if("ClassOrInterfaceType" == s){
        execClassOrInterfaceType(nodeNum);
    }else if("ClassTypeList"==s){
        execClassTypeList(nodeNum);
    }else if("ConstantDeclaration" == s){
        execConstantDeclaration(nodeNum);
    }else if("ConstantExpression" == s){
        execConstantExpression(nodeNum);
    }else if("ConstructorBody" == s){
        execConstructorBody(nodeNum);
    }else if("ConstructorDeclaration" == s){
        execConstructorDeclaration(nodeNum);
    }else if("ConstructorDeclarator" == s){
        execConstructorDeclarator(nodeNum);
    }else if("ContinueStatement" == s){
        execContinueStatement(nodeNum);
    }else if("DoStatement" == s){
        execDoStatement(nodeNum);
    }else if("DoublePointLiteral" == s){
        execDoublePointLiteral(nodeNum);
    }else if("EmptyStatement" == s){

    }else if("ExplicitConstructorInvocation"==s){
        execExplicitConstructorInvocation(nodeNum);
    }else if("ExtendsInterfaces" == s){
        execExtendsInterfaces(nodeNum);
    }else if("FieldAccess"==s){
        execFieldAccess(nodeNum);
    }else if("FieldDeclaration"==s){
        execFieldDeclaration(nodeNum);
    }else if("Finally"==s){
        execFinally(nodeNum);
    }else if("FloatingPointLiteral" == s){
        execFloatingPointLiteral(nodeNum);
    }else if("FloatingPointType" == s){
        execFloatingPointType(nodeNum);
    }else if("ForInit"==s){
        execForInit(nodeNum);
        inForInit=false;
    }else if("FormalParameterList"== s){
        execFormalParameterList(nodeNum);
        inFormalParameterList=false;
    }else if("FormalParameter" == s){
        execFormalParameter(nodeNum);
    }else if("ForStatementNoShortIf"==s){
        execForStatementNoShortIf(nodeNum);
    }else if("ForUpdate"==s){
        execForUpdate(nodeNum);
    }else if("IfThenElseStatementNoShortIf" == s){
        execIfThenElseStatementNoShortIf(nodeNum);
    }else if("ImportDeclarations"==s){
        execImportDeclarations(nodeNum);
    }else if("ImportDeclaration"==s){
        execImportDeclaration(nodeNum);
    }else if("IntegerLiteral" == s){
        execIntegerLiteral(nodeNum);
    }else if("InterfaceBody"==s){
        execInterfaceBody(nodeNum);
    }else if("InterfaceDeclaration" == s){
        execInterfaceDeclaration(nodeNum);
    }else if("InterfaceMemberDeclaration"==s){
        execInterfaceMemberDeclaration(nodeNum);
    }else if("InterfaceMemberDeclarations"==s){
        execInterfaceMemberDeclarations(nodeNum);
    }else if("Interfaces"==s){
        execInterfaces(nodeNum);
    }else if("InterfaceType"==s){
        execInterfaceType(nodeNum);
    }else if("InterfaceTypeList"==s){
        execInterfaceTypeList(nodeNum);
    }else if("LabeledStatement" == s){
        execLabeledStatement(nodeNum);
    }else if("LabeledStatementNoShortIf" == s){
        execLabeledStatementNoShortIf(nodeNum);
    }else if("Literal"==s){
        execLiteral(nodeNum);
    }else if("LocalVariableDeclaration"==s){
        execLocalVariableDeclaration(nodeNum);
    }else if("LocalVariableDeclarationStatement"==s){
        execLocalVariableDeclarationStatement(nodeNum);
    }else if("MethodDeclarator"==s){
        execMethodDeclarator(nodeNum);
    }else if("MethodHeader" == s){
        execMethodHeader(nodeNum);
    }else if("Modifier" == s){
        execModifier(nodeNum);
    }else if("Modifiers" == s){
        execModifiers(nodeNum);
    }else if("NullLiteral" == s){
        execNullLiteral(nodeNum);
    }else if("NumericType" == s){
        execNumericType(nodeNum);
    }else if("PackageDeclaration" == s){
        execPackageDeclaration(nodeNum);
    }else if("PostDecrementExpression"==s){
        execPostDecrementExpression(nodeNum);
    }else if("PostIncrementExpression"==s){
        execPostIncrementExpression(nodeNum);
    }else if("PreDecrementExpression"==s){
        execPreDecrementExpression(nodeNum);
    }else if("PreIncrementExpression"==s){
        execPreIncrementExpression(nodeNum);
    }else if("PrimitiveType"==s){
        execPrimitiveType(nodeNum);
    }else if("QualifiedName"==s){
        execQualifiedName(nodeNum);
    }else if("ReferenceType"==s){
        execReferenceType(nodeNum);
    }else if("ReturnStatement"==s){
        execReturnStatement(nodeNum);
    }else if("SingleTypeImportDeclaration" == s){
        execSingleTypeImportDeclaration(nodeNum);
    }else if("StatementExpressionList" == s){
        execStatementExpressionList(nodeNum);
    }else if("StaticInitializer" == s){
        execStaticInitializer(nodeNum);
    }else if("StringLiteral" == s){
        execStringLiteral(nodeNum);
    }else if("Super"==s){
        execSuper(nodeNum);
    }else if("SwitchBlock"==s){
        execSwitchBlock(nodeNum);
    }else if("SwitchBlockStatementGroup"==s){
        execSwitchBlockStatementGroup(nodeNum);
    }else if("SwitchBlockStatementGroups"==s){
        execSwitchBlockStatementGroups(nodeNum);
    }else if("SwitchLabel"==s){
        execSwitchLabel(nodeNum);
    }else if("SwitchLabels"==s){
        execSwitchLabels(nodeNum);
    }else if("SwitchStatement"==s){
        execSwitchStatement(nodeNum);
    }else if("SynchronizedStatement"==s){
        execSynchronizedStatement(nodeNum);
    }else if("Throws"==s){
        execThrows(nodeNum);
    }else if("ThrowStatement"==s){
        execThrowStatement(nodeNum);
    }else if("TryStatement"==s){
        execTryStatement(nodeNum);
    }else if("Type"==s){
        execType(nodeNum);
    }else if("TypeImportOnDemandDeclaration"==s){
        execTypeImportOnDemandDeclaration(nodeNum);
    }else if("VariableDeclarator"==s){
        execVariableDeclarator(nodeNum);
    }else if("VariableDeclaratorId"==s){
        execVariableDeclaratorId(nodeNum);
    }else if("VariableDeclarators"==s){
        execVariableDeclarators(nodeNum);
    }else if("VariableInitializers"==s){
        execVariableInitializers(nodeNum);
    }else if("WhileStatementNoShortIf"==s){
        execWhileStatementNoShortIf(nodeNum);
    }else if("MethodHeader_"==s){
        execMethodHeader_(nodeNum);
    }else if("IntegralType"==s){
        execIntegralType(nodeNum);
    }else if("CastExpression"==s){
        execCastExpression(nodeNum);
    }
    else{
        // cout << "function not written " << s << endl;
    }

    // cout<<"exiting "<<s<<endl;
    return;
}

void print3AC(int nodeNum){
    FILE* fp = freopen("threeAC.txt","w",stdout);
    set<string> checkLabel;
    for(int i=0;i<attr3AC[nodeNum].threeAC.size();i++){
        int siz = attr3AC[nodeNum].threeAC[i].size();
        if(attr3AC[nodeNum].threeAC[i][0]=='L' && attr3AC[nodeNum].threeAC[i][siz-1]==':'){
            string lab=attr3AC[nodeNum].threeAC[i];
            if(checkLabel.find(lab)==checkLabel.end()){
                cout << (attr3AC[nodeNum].threeAC)[i] << endl;
                checkLabel.insert(lab);
            }
        }else cout << (attr3AC[nodeNum].threeAC)[i] << endl;
    }
    // fclose(fp);
}

void printAssemblyCode (int nodeNum){
    // cout<<"printing assembly code"<<endl;
    FILE* fp = freopen("assemblyCode.s","w",stdout);

    cout << ".data\nmessage:\n"
        << ".ascii \"The answer is 42\\n\\0\"\n"
        << ".align 8\nmessage_len = . - message\n"
        << "zeroErr:\n"
        << ".ascii \"Divide by Zero Error. [EXIT 1]\\n\\0\"\n"
        << ".align 8\n\n";
    int inMani = 0;
    // cout << attr3AC[nodeNum].assemblyCode.size() << endl;
    cout << ".text" << endl;
    cout << ".globl main" << endl;
    set<string> checkLabel;
    for(int i=0;i<attr3AC[nodeNum].assemblyCode.size();i++){
        if(attr3AC[nodeNum].assemblyCode[i].substr(0,3)=="ret" && inMani){
            int numtemps = 8*tempNum;
            string makespace = "addq $" + to_string(numtemps) + ", %rsp";
            cout << makespace << endl;
            //push value 0 in register rax
            //idk if this works
            // for(int frH=0;frH<freeHeap.size();frH++){
            //     cout << "movq $12, %rax" << endl;
            //     cout << freeHeap[frH] << endl;
            //     cout << "movq $0, %rsi" << endl;
            //     cout << "syscall" << endl;
            // }
            cout << "popq %rbp" << endl;
            cout << "movq $0, %rax" << endl;
            cout << "ret" << endl;
            inMani=0;
            continue;
        }
        int siz = attr3AC[nodeNum].assemblyCode[i].size();
        if(attr3AC[nodeNum].assemblyCode[i][0]=='L' && attr3AC[nodeNum].assemblyCode[i][siz-1]==':'){
            string lab=attr3AC[nodeNum].assemblyCode[i];
            if(checkLabel.find(lab)==checkLabel.end()){
                cout << (attr3AC[nodeNum].assemblyCode)[i] << endl;
                checkLabel.insert(lab);
            }
        }
        else cout << (attr3AC[nodeNum].assemblyCode)[i] << endl;
        if(attr3AC[nodeNum].assemblyCode[i].substr(0,4)=="main"){
            inMani = 1;
            cout << "pushq %rbp" << endl;
            cout << "movq %rsp, %rbp" << endl;
            int numtemps = 8*tempNum;
            string makespace = "subq $" + to_string(numtemps) + ", %rsp";
            cout << makespace << endl;
            // for(int temppush=0;temppush<tempNum;temppush++){
            //     //make more space on stack by subtracting %rsp by 8
            //     string makespace = "subq $8, %rsp";
            //     cout << makespace << endl;
            // }    
        }
    }

    cout << "\ndivide_by_zero:\n";
    cout << "movq $1, %rax   # system call number for write()\n"
    << "movq $1, %rdi   # file descriptor for stdout\n"
    << "leaq zeroErr(%rip), %rsi   # address of message\n"
    << "movq $32, %rdx   # length of message\n"
    << "syscall\n"
    << "movq $60, %rax\n"
    << "movq $1, %rdx\n"
    << "syscall\n";

    fclose(fp);
}

