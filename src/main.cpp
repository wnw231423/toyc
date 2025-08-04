#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include "ast.h"
#include "visit.h"

using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

/** Usage:
 * ./compiler <input_file>
 * The input file should contain the source code to be parsed.
 * The output will be the AST representation of the source code.
 */
int main(int argc, char *argv[]) {
    int ast_mode = 0;
    int ir_mode = 0;
    char* input;

    assert(argc == 2 || argc == 3);
    if (argc == 3) {
        if (string(argv[1]) == "-a") {
            ast_mode = 1;
        } else if (string(argv[1]) == "-ir") {
            ir_mode = 1;
        }
        input = argv[2];
    } else {
        input = argv[1];
    }

    yyin = fopen(input, "r");
    assert(yyin);

    unique_ptr<BaseAST> ast;
    auto ret = yyparse(ast);
    assert(!ret);

    auto comp_unit = dynamic_cast<CompUnitAST *>(ast.get());
    if (ast_mode) {
        comp_unit->Dump(0);
    } else if (ir_mode) {
        cout << comp_unit->to_IR()->toString() << endl;
    } else {
        cout << visit_program(comp_unit->to_IR()) << endl;
    }

    return 0;
}
