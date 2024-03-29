%{
    #include <stdio.h>
    #include <string.h>
    #include "parser.tab.h"
    #include "attr.h"
%}

%option yylineno

EndOfLineComment "//".*
Comment "/*"[^*]*"*"+([^*/][^*]*[*]+)*"/"|{EndOfLineComment}


LineTerminator \n|\r|\r\n
WhiteSpace [ \t\f]+|{LineTerminator}


JavaLetter [a-zA-Z$_]
JavaLetterOrDigit [a-zA-Z$_0-9]
Identifier {JavaLetter}{JavaLetterOrDigit}*
ReservedKeyword abstract|continue|for|new|switch|assert|default|if|package|synchronized|boolean|do|goto|private|this|break|double|implements|protected|throw|byte|else|import|public|throws|case|enum|instanceof|return|transient|catch|extends|int|short|try|char|final|interface|static|void|class|finally|long|strictfp|volatile|const|float|native|super|while|_ 
ContextualKeyword exports|opens|requires|uses|module|permits|sealed|var|non-sealed|provides|to|with|open|record|transitive|yield
Keyword {ReservedKeyword}|{ContextualKeyword}

Underscores _+
NonZeroDigit [1-9]
DigitOrUnderscore {Digit}|"_"
DigitsAndUnderscores {DigitOrUnderscore}{DigitOrUnderscore}*
Digit "0"|{NonZeroDigit}
Digits {Digit}|({Digit}{DigitsAndUnderscores}?{Digit})
DecimalNumeral "0"|({NonZeroDigit}{Digits}?)|({NonZeroDigit}{Underscores}{Digits})
IntegerTypeSuffix [lL]
DecimalIntegerLiteral {DecimalNumeral}{IntegerTypeSuffix}?


HexDigit [0-9a-fA-F]
HexDigitsAndUnderscore {HexDigit}|"_"
HexDigitsAndUnderscores {HexDigitsAndUnderscore}{HexDigitsAndUnderscore}*
HexDigits {HexDigit}|({HexDigit}{HexDigitsAndUnderscores}?{HexDigit})
HexNumeral ("0x"{HexDigits})|("0X"{HexDigits})
HexIntegerLiteral {HexNumeral}{IntegerTypeSuffix}?



OctalDigit [0-7]
OctalDigitOrUnderscore {OctalDigit}|"_"
OctalDigitsAndUnderscores {OctalDigitOrUnderscore}{OctalDigitOrUnderscore}*
OctalDigits {OctalDigit}|({OctalDigit}{OctalDigitsAndUnderscores}?{OctalDigit})
OctalNumeral ("0"{OctalDigits})|("0"{Underscores}{OctalDigits})
OctalIntegerLiteral {OctalNumeral}{IntegerTypeSuffix}?


BinaryDigit [0-1]
BinaryDigitOrUnderscore {BinaryDigit}|"_"
BinaryDigitsAndUnderscores {BinaryDigitOrUnderscore}{BinaryDigitOrUnderscore}*
BinaryDigits {BinaryDigit}|({BinaryDigit}{BinaryDigitsAndUnderscores}?{BinaryDigit})
BinaryNumeral ("0b"{BinaryDigits})|("0B"{BinaryDigits})
BinaryIntegerLiteral {BinaryNumeral}{IntegerTypeSuffix}?

IntegerLiteral {DecimalIntegerLiteral}|{HexIntegerLiteral}|{OctalIntegerLiteral}|{BinaryIntegerLiteral}

FloatTypeSuffix "f"|"F"
DoubleTypeSuffix "D"|"d"
Sign [+-]
SignedInteger {Sign}?{Digits}
ExponentIndicator "e"|"E"
ExponentPart {ExponentIndicator}{SignedInteger}
DecimalFloatingPointLiteral ({Digits}\.{Digits}?{ExponentPart}?{FloatTypeSuffix}?)|(\.{Digits}{ExponentPart}?{FloatTypeSuffix}?)|({Digits}{ExponentPart}{FloatTypeSuffix}?)|({Digits}{ExponentPart}?{FloatTypeSuffix})
DecimalDoublePointLiteral ({Digits}\.{Digits}?{ExponentPart}?{DoubleTypeSuffix}?)|(\.{Digits}{ExponentPart}?{DoubleTypeSuffix}?)|({Digits}{ExponentPart}{DoubleTypeSuffix}?)|({Digits}{ExponentPart}?{DoubleTypeSuffix})


BinaryExponentIndicator "p"|"P"
BinaryExponent {BinaryExponentIndicator}{SignedInteger}
HexSignificand ({HexNumeral}\.?)|("0x"{HexDigits}?\.{HexDigits})
HexadecimalFloatingPointLiteral {HexSignificand}{BinaryExponent}{FloatTypeSuffix}?
HexadecimalDoublePointLiteral {HexSignificand}{BinaryExponent}{DoubleTypeSuffix}?

FloatingPointLiteral {DecimalFloatingPointLiteral}|{HexadecimalFloatingPointLiteral}
DoublePointLiteral {DecimalDoublePointLiteral}|{HexadecimalDoublePointLiteral}


BooleanLiteral "true"|"false"


OctalEscape \{OctalDigit}|\{OctalDigit}{OctalDigit}|\[0-3]{OctalDigit}{OctalDigit}
EscapeSequence \b|\s|\t|\n|\f|\r|{LineTerminator}|\'|\\
/* OctalEscape is not included in EscapeSequence for now */
/* \" is not included in EscapeSequence for now */
InputCharacter [^\n\r]
SingleCharacter [^\n\r\'\\]
CharacterLiteral ("'"{SingleCharacter}"'")|\'\\n\'|(\'\\s\')|(\'\\t\')|(\'\\b\')|(\'\\f\')|(\'\\r\')|(\'\\'\')|(\'\\\\\')|(\'\\r\\n\')


StringCharacter [^\"\\]|{EscapeSequence}
StringLiteral \"{StringCharacter}*\"


TextBlockCharacter [^\"\n\r\\]|{EscapeSequence}|{LineTerminator}
TextBlockWhiteSpace [ \t\f]+
TextBlock1 \"\"\"([^\"]+\"{0,2}[^\"]+)*\"\"\"
TextBlock (\"\"\"{TextBlockWhiteSpace}*{LineTerminator}{TextBlockCharacter}*\"\"\")|{TextBlock1}


NullLiteral "null"

Literal {IntegerLiteral}|{FloatingPointLiteral}|{BooleanLiteral}|{CharacterLiteral}|{StringLiteral}|{TextBlock}|{NullLiteral}

Separator "::"|"..."|[(){};,@]|"["|"]"


%%

{Comment} {continue;}
{LineTerminator} {continue;}
"."            {yylval.name=strdup(yytext); return (DOT);}
{IntegerLiteral} {yylval.name=strdup(yytext); return (INTEGERLITERAL);}
{FloatingPointLiteral} {yylval.name=strdup(yytext); return (FLOATINGPOINTLITERAL);}
{DoublePointLiteral} {yylval.name=strdup(yytext); return (DOUBLEPOINTLITERAL);}
{BooleanLiteral} {yylval.name=strdup(yytext); return (BOOLEANLITERAL);}
{CharacterLiteral} {yylval.name=strdup(yytext); return (CHARACTERLITERAL);}
{StringLiteral} {yylval.name=strdup(yytext); return (STRINGLITERAL);}
{NullLiteral} {yylval.name=strdup(yytext); return (NULLLITERAL);}
"byte" {yylval.name=strdup(yytext); return (BYTE);}
"import" {yylval.name=strdup(yytext); return (IMPORT);}

"public"      {yylval.name=strdup(yytext); return (PUBLIC);}
"protected"      {yylval.name=strdup(yytext); return (PROTECTED);}
"private"      {yylval.name=strdup(yytext); return (PRIVATE);}
"static"      {yylval.name=strdup(yytext); return (STATIC);}
"abstract"      {yylval.name=strdup(yytext); return (ABSTRACT);}
"final"      {yylval.name=strdup(yytext); return (FINAL);}
"native"      {yylval.name=strdup(yytext); return (NATIVE);}
"synchronized"      {yylval.name=strdup(yytext); return (SYNCHRONIZED);}
"transient"      {yylval.name=strdup(yytext); return (TRANSIENT);}
"volatile"      {yylval.name=strdup(yytext); return (VOLATILE);}

"package"      {yylval.name=strdup(yytext); return (PACKAGE);}
","            {yylval.name=strdup(yytext); return (COMMA);}
"="            {yylval.name=strdup(yytext); return (EQUALTO);}
"super"        {yylval.name=strdup(yytext); return (SUPER);}
"if"           {yylval.name=strdup(yytext); return (IF);}
"else"         {yylval.name=strdup(yytext); return (ELSE);}
"switch"       {yylval.name=strdup(yytext); return (SWITCH);}
"case"         {yylval.name=strdup(yytext); return (CASE);}
"default"      {yylval.name=strdup(yytext); return (DEFAULT);}
"while"        {yylval.name=strdup(yytext); return (WHILE);}
"do"           {yylval.name=strdup(yytext); return (DO);}
"for"          {yylval.name=strdup(yytext); return (FOR);}
"break"        {yylval.name=strdup(yytext); return (BREAK);}
"throw"        {yylval.name=strdup(yytext); return (THROW);}
"try"          {yylval.name=strdup(yytext); return (TRY);}
"finally"      {yylval.name=strdup(yytext); return (FINALLY);}
"catch"        {yylval.name=strdup(yytext); return (CATCH);}
"void"         {yylval.name=strdup(yytext); return (VOID);}
"throws"       {yylval.name=strdup(yytext); return (THROWS);}
"extends"      {yylval.name=strdup(yytext); return (EXTENDS);}
"implements"   {yylval.name=strdup(yytext); return (IMPLEMENTS);}
"class"        {yylval.name=strdup(yytext); return (CLASS);}
"interface"    {yylval.name=strdup(yytext); return (INTERFACE);}
"boolean"      {yylval.name=strdup(yytext); return (BOOLEAN);}
"short"        {yylval.name=strdup(yytext); return (SHORT);}
"int"          {yylval.name=strdup(yytext); return (INT);}
"long"         {yylval.name=strdup(yytext); return (LONG);}
"char"         {yylval.name=strdup(yytext); return (CHAR);}
"float"        {yylval.name=strdup(yytext); return (FLOAT);}
"double"       {yylval.name=strdup(yytext); return (DOUBLE);}
"instanceof"   {yylval.name=strdup(yytext); return (INSTANCEOF);}
"this"         {yylval.name=strdup(yytext); return (THIS);}
"new"          {yylval.name=strdup(yytext); return (NEW);}
"continue"     {yylval.name=strdup(yytext); return (CONTINUE);}
"return"       {yylval.name=strdup(yytext); return (RETURN);}
"{"     {yylval.name=strdup(yytext); return (OPCURLY);}
"}"     {yylval.name=strdup(yytext); return (CLCURLY);}
"("     {yylval.name=strdup(yytext); return (OPROUND);}
")"     {yylval.name=strdup(yytext); return (CLROUND);}
"["     {yylval.name=strdup(yytext); return (OPSQR);}
"]"     {yylval.name=strdup(yytext); return (CLSQR);}
"+"     {yylval.name=strdup(yytext); return (PLUS);}
"++"    {yylval.name=strdup(yytext); return (DPLUS);}
"--"    {yylval.name=strdup(yytext); return (DMINUS);}
"-"     {yylval.name=strdup(yytext); return (MINUS);}
":"     {yylval.name=strdup(yytext); return (COLON);}
";"     {yylval.name=strdup(yytext); return (SEMICOLON);}
"~"     {yylval.name=strdup(yytext); return (TILDA);}
"!"     {yylval.name=strdup(yytext); return (EX);}
"?"     {yylval.name=strdup(yytext); return (QUES);}
"*"     {yylval.name=strdup(yytext); return (ASTERIX);}
"/"     {yylval.name=strdup(yytext); return (FSLASH);}
"%"     {yylval.name=strdup(yytext); return (MOD);}
"<<"    {yylval.name=strdup(yytext); return (LSHIFT);}
">>"    {yylval.name=strdup(yytext); return (RSHIFT);}
">>>"   {yylval.name=strdup(yytext); return (URSHIFT);}
"<"     {yylval.name=strdup(yytext); return (LT);}
">"     {yylval.name=strdup(yytext); return (GT);}
"<="    {yylval.name=strdup(yytext); return (LTE);}
">="    {yylval.name=strdup(yytext); return (GTE);}
"=="    {yylval.name=strdup(yytext); return (DOUBLEEQ);}
"!="    {yylval.name=strdup(yytext); return (NOTEQ);}
"&"     {yylval.name=strdup(yytext); return (AND);}
"^"     {yylval.name=strdup(yytext); return (XOR);}
"|"     {yylval.name=strdup(yytext); return (OR);}
"&&"    {yylval.name=strdup(yytext); return (DAND);}
"||"    {yylval.name=strdup(yytext); return (DOR);}
"*="     {yylval.name=strdup(yytext); return (STAREQUALTO);}
"/="     {yylval.name=strdup(yytext); return (BYEQUALTO);}
"+="     {yylval.name=strdup(yytext); return (PLUSEQUALTO);}
"-="     {yylval.name=strdup(yytext); return (MINUSEQUALTO);}
"<<="     {yylval.name=strdup(yytext); return (TWOLEFTSHIFTEQUALTO);}
">>="     {yylval.name=strdup(yytext); return (TWORIGHTSHIFTEQUALTO);}
">>>="     {yylval.name=strdup(yytext); return (THREERIGHTSHIFTEQUALTO);}
"&="     {yylval.name=strdup(yytext); return (ANDEQUALTO);}
"^="     {yylval.name=strdup(yytext); return (XOREQUALTO);}
"|="     {yylval.name=strdup(yytext); return (BAREQUALTO);}
{Literal} {yylval.name=strdup(yytext);  return (LITERAL); }
{Identifier} {yylval.name=strdup(yytext); return (IDENTIFIER);}
{WhiteSpace} { continue;}
.       {
            printf("Error: Unrecognized character on line %d\n",yylineno);
            exit(0);
        }

%%

void showError(){
    printf("Other input");
}
int yywrap(){
    return 1;
}