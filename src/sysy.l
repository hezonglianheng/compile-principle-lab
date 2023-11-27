%option noyywrap
%option noinput
%option nounput

%{
#include <cstdlib>
#include <string>
#include "sysy.tab.hpp"

using namespace std;

%}

WhiteSpace [ \t\n\r]*
LineComment "//".*
MultilineComment "/*"(.|\n)*"*/"

Identifier [a-zA-Z_][a-zA-Z0-9_]*

LOrOp "||"
LAndOp "&&"
EqOp "=="|"!="
RelOp ">"|"<"|">="|"<="

Decimal [1-9][0-9]*
Octal 0[0-7]*
Hexadecimal 0[xX][0-9a-fA-F]*

%%

{WhiteSpace} {}
{LineComment} {}
{MultilineComment} {}

"int" {return INT;}
"return" {return RETURN;}
{Identifier} {yylval.str_val = new string(yytext); return IDENT;}
{LOrOp} {yylval.str_val = new string(yytext); return LOR_CONST;}
{LAndOp} {yylval.str_val = new string(yytext); return LAND_CONST;}
{EqOp} {yylval.str_val = new string(yytext); return EQ_CONST;}
{RelOp} {yylval.str_val = new string(yytext); return REL_CONST;}
{Decimal} {yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Octal} {yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Hexadecimal} {yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
. {return yytext[0];}
%%