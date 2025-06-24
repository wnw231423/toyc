/* 以下内容会添加到Bison生成的头文件中 */
%code requires {
    #include <memory>
    #include <string>
    #include "ast.h"
}

/* 以下内容会添加到Bison生成的源文件中 */
%{
#include <iostream>
#include <memory>
#include <string>
#include "ast.h"

int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;
%}

// 定义 parser 函数
%parse-param { std::unique_ptr<BaseAST> &ast }

// 定义yylval, 是一个union
%union {
    std::string *str_val;
    int int_val;
    BaseAST *ast_val;
}

%token RETURN
%token <str_val> TYPE IDENT
%token <int_val> INT_CONST

%type <ast_val> FuncDef Block Stmt
%type <int_val> Number
%%

CompUnit
    : FuncDef {
        auto comp_unit = make_unique<CompUnitAST>();
        comp_unit->func_def = unique_ptr<BaseAST>($1);
        comp_unit->Dump();
        ast = move(comp_unit);
    }
    ;

FuncDef
    : TYPE IDENT '(' ')' Block {
        auto ast = new FuncDefAST();
        ast->func_type = *unique_ptr<string>($1);
        ast->ident = *unique_ptr<string>($2);
        ast->block = unique_ptr<BaseAST>($5);
        ast->Dump();
        $$ = ast;
    }
    ;

Block
    : '{' Stmt '}' {
        auto ast = new BlockAST();
        ast->stmt = unique_ptr<BaseAST>($2);
        ast->Dump();
        $$ = ast;
    }
    ;

Stmt
    : RETURN Number ';' {
        auto ast = new ReturnAST();
        ast->ret_value = $2;
        ast->Dump();
        $$ = ast;
    }
    ;

Number
    : INT_CONST {
        $$ = $1;
    }
    ;

%%

void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
    cerr << "error: " << s << endl;
}