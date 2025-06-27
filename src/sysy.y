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
%token PLUS MINUS NOT
%token <str_val> TYPE IDENT
%token <int_val> INT_CONST

%type <ast_val> FuncDef Block Stmt Exp UnaryExp PrimaryExp Number
%type <str_val> UnaryOp
%%

CompUnit
    : FuncDef {
        auto comp_unit = make_unique<CompUnitAST>();
        comp_unit->func_def = unique_ptr<BaseAST>($1);
        ast = move(comp_unit);
    }
    ;

FuncDef
    : TYPE IDENT '(' ')' Block {
        auto ast = new FuncDefAST();
        ast->func_type = *unique_ptr<string>($1);
        ast->ident = *unique_ptr<string>($2);
        ast->block = unique_ptr<BaseAST>($5);
        $$ = ast;
    }
    ;

Block
    : '{' Stmt '}' {
        auto ast = new BlockAST();
        ast->stmt = unique_ptr<BaseAST>($2);
        $$ = ast;
    }
    ;

Stmt
    : RETURN Exp ';' {
        auto ast = new ReturnStmtAST();
        ast->exp = unique_ptr<BaseAST>($2);
        $$ = ast;
    }
    ;

Exp
    : UnaryExp {
        auto ast = new ExpAST();
        ast->unaryExp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }

UnaryExp
    : PrimaryExp {
        auto ast = new UnaryExpAST();
        ast->type = 1;
        ast->primaryExp_unaryExp = unique_ptr<BaseAST>($1);
    }
    | UnaryOp UnaryExp {
        auto ast = new UnaryExpAST();
        ast->type = 2;
        ast->unary_op = *unique_ptr<string>($1);
        ast->primaryExp_unaryExp = unique_ptr<BaseAST>($2);
        $$ = ast;
    }
    ;

UnaryOp
    : PLUS {
        string *op = new string("+");
        $$ = op;
     }
    | MINUS {
        string *op = new string("-");
        $$ = op;
     }
    | NOT {
        string *op = new string("!");
        $$ = op;
    }
    ;

PrimaryExp
    : '(' Exp ')' {
        auto ast = new PrimaryExpAST();
        ast->type = 1;
        ast->exp_number = unique_ptr<BaseAST>($2);
        $$ = ast;
    }
    | Number {
        auto ast = new PrimaryExpAST();
        ast->type = 2;
        ast->exp_number = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    ;

Number
    : INT_CONST {
        auto ast = new NumberAST();
        ast->value = $1;
        $$ = ast;
    }
    ;

%%

void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
    cerr << "error: " << s << endl;
}