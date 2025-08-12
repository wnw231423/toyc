#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#include "ast.h"
#include "visit.h"
#include "inline.h"
#include "register_allocation.h"

using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

/** Usage:
 * ./compiler <input_file>
 * ./compiler -a <input_file>     # AST mode
 * ./compiler -ir <input_file>    # IR mode
 * ./compiler -inline <input_file> # IR with inline optimization
 * ./compiler -inline-asm <input_file> # Assembly with inline optimization
 * The input file should contain the source code to be parsed.
 * The output will be the AST representation of the source code.
 */
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
            //opt = 1;
            opt = 0;
        }
    }

    if (opt) {
        auto program = comp_unit->to_IR();
        InlineOptimizer optimizer(3, 50);
        optimizer.optimize(program.get());
        cout << visit_program(std::move(program)) << endl;
    } else {
        auto ir = comp_unit->to_IR();
        cout << visit_program(std::move(ir)) << endl;
    }
    return 0;
}
