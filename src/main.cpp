#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include "ast.h"
#include "consprop.h"
#include "visit.h"
#include "inline.h"

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
    int ast_mode = 0;
    int ir_mode = 0;
    int opt_ir_mode = 0;
    int opt_mode = 0;
    char* input;

    assert(argc == 2 || argc == 3);
    if (argc == 3) {
        if (string(argv[1]) == "-a") {
            ast_mode = 1;
        } else if (string(argv[1]) == "-ir") {
            ir_mode = 1;
        } else if (string(argv[1]) == "-opt-ir") {
            opt_ir_mode = 1;
        } else if (string(argv[1]) == "opt") {
            opt_mode = 1;
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
    } else if (opt_ir_mode) {
        auto program = comp_unit->to_IR();
        
        // 执行函数内联优化
        InlineOptimizer optimizer(3, 50); // 深度限制3，大小限制50
        optimizer.optimize(program.get());

        // 执行常量传播，控制流简化
        ConstantPropagationOptimizer optimizer2;
        optimizer2.optimize(program.get());
        
        cout << "// 优化后的IR代码:" << endl;
        cout << program->toString() << endl;
    } else if (opt_mode) {
        auto program = comp_unit->to_IR();
        
        // 执行函数内联优化
        InlineOptimizer optimizer(3, 50); // 深度限制3，大小限制50
        optimizer.optimize(program.get());

        // 执行常量传播，控制流简化
        ConstantPropagationOptimizer optimizer2;
        optimizer2.optimize(program.get());
        
        cout << "// 优化后的汇编代码:" << endl;
        cout << visit_program(std::move(program)) << endl;
    } else {
        auto ir = comp_unit->to_IR();
        cout << visit_program(std::move(ir)) << endl;
    }

    return 0;
}
