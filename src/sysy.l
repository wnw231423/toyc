%option noyywrap
%option nounput
%option noinput

%{
#include <cstdlib>
#include <string>
#include "sysy.tab.hpp"

using namespace std;
%}

WhiteSpace [ \t\n\r]+

LineComment "//".*

Identifier [a-zA-Z_][a-zA-Z0-9_]*

Decimal 0|[1-9][0-9]*

%%

{WhiteSpace} {}
{LineComment} {}

"return" { return RETURN; }

"int" {
    yylval.str_val = new string(yytext);
    return TYPE;
}

"+" { return PLUS; }

"-" { return MINUS; }

"!" { return NOT; }

"*" { return TIMES; }

"/" { return DIV; }

"%" { return MOD; }

"<" { return LT; }

"<=" { return LE; }

">" { return GT; }

">=" { return GE; }

"==" { return EQ; }

"!=" { return NE; }

"&&" { return AND; }

"||" { return OR; }

{Identifier} {
    yylval.str_val = new string(yytext);
    return IDENT;
}

{Decimal} {
    yylval.int_val = strtol(yytext, nullptr, 0);
    return INT_CONST;
}

. {return yytext[0];}

%%