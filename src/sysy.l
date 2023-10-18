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

Identifier [a-zA-Z_][a-zA-Z0-9_]*

Decimal [1-9][0-9]*
Octal 0[0-7]*
Hexadecimal 0[xX][0-9a-fA-F]*

%%

{WhiteSpace} {}
{LineComment} {}

"int" {return INT;}
"return" {return RETURN;}
{Identifier} {yylval.str_val = new string(yytext); return IDENT;}