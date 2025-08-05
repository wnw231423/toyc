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
    yyin = stdin;
    assert(yyin);

    unique_ptr<BaseAST> ast;
    auto ret = yyparse(ast);
    assert(!ret);

    auto comp_unit = dynamic_cast<CompUnitAST *>(ast.get());
    cout << visit_program(comp_unit->to_IR()) << endl;


    return 0;
}
