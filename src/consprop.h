#ifndef CONSTANT_PROPAGATION_H
#define CONSTANT_PROPAGATION_H

#include "IR.h"
#include <unordered_map>
#include <string>

// 常量传播 + 死代码删除优化器
class ConstantPropagationOptimizer
{
public:
    ConstantPropagationOptimizer() = default;

    void optimize(Program *program);

private:
    // 用于记录变量 / alloc 地址的常量值
    std::unordered_map<std::string, int> const_table;

    // 优化单个函数
    void optimize_function(Function *func);

    // 优化单个基本块
    void optimize_basic_block(BasicBlock *bb);

    // 尝试进行常量折叠
    bool fold_binary(BinaryValue *bin, int &result);

    // 检查 IRValue 是否是一个可用的常量
    bool get_constant(IRValue *val, int &out_const);
};

#endif
