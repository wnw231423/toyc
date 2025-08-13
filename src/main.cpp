#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#include "ast.h"
#include "visit.h"
#include "inline.h"
#include "consprop.h"

using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);


int main(int argc, char *argv[]) {
    int opt = 0;

    yyin = stdin;
    assert(yyin);

    unique_ptr<BaseAST> ast;
    auto ret = yyparse(ast);
    assert(!ret);

    auto comp_unit = dynamic_cast<CompUnitAST *>(ast.get());

    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-opt") == 0) {
            opt = 1;
            // opt = 0;
        }
    }

    //if (opt) {
    if ( true ) {
        auto program = comp_unit->to_IR();
        InlineOptimizer optimizer(1, 10);
        //ConstantPropagationOptimizer optimizer;
        optimizer.optimize(program.get());
        cout << visit_program(std::move(program)) << endl;
    }
    else
    {
        cout << visit_program(comp_unit->to_IR()) << endl;
    }
    return 0;
}