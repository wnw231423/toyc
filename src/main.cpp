#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include "ast.h"
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
    int inline_mode = 0;
    int inline_asm_mode = 0;
    int conservative_inline_mode = 0;
    char* input;

    assert(argc == 2 || argc == 3);
    if (argc == 3) {
        if (string(argv[1]) == "-a") {
            ast_mode = 1;
        } else if (string(argv[1]) == "-ir") {
            ir_mode = 1;
        } else if (string(argv[1]) == "-inline") {
            inline_mode = 1;
        } else if (string(argv[1]) == "-inline-asm") {
            inline_asm_mode = 1;
        } else if (string(argv[1]) == "-conservative-inline") {
            conservative_inline_mode = 1;
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
    } else if (inline_mode) {
        auto program = comp_unit->to_IR();
        
        // 执行函数内联优化 - 允许内联更大的函数
        InlineOptimizer optimizer(1, 10); // 深度限制1，大小限制10
        optimizer.optimize(program.get());
        
        cout << "// 优化后的IR代码:" << endl;
        cout << program->toString() << endl;
    } else if (inline_asm_mode) {
        auto program = comp_unit->to_IR();
        
        // 执行函数内联优化 - 允许内联更大的函数
        InlineOptimizer optimizer(1, 10); // 深度限制1，大小限制10
        optimizer.optimize(program.get());
        
        cout << "// 内联优化后的汇编代码:" << endl;
        cout << visit_program(std::move(program)) << endl;
    } else if (conservative_inline_mode) {
        auto program = comp_unit->to_IR();
        
        // 执行保守的函数内联优化
        InlineOptimizer optimizer(1, 6); // 深度限制1，大小限制6
        optimizer.optimize(program.get());
        
        cout << "// 保守内联优化后的汇编代码:" << endl;
        cout << visit_program(std::move(program)) << endl;
    } else {
        cout << visit_program(comp_unit->to_IR()) << endl;
    }

    return 0;
}
