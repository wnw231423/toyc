#ifndef INLINE_H
#define INLINE_H

#include "IR.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>

// 函数内联优化器
class InlineOptimizer {
private:
    // 存储函数定义，用于内联时查找函数体
    std::unordered_map<std::string, Function*> function_map;
    
    // 记录已内联的函数，避免递归内联
    std::unordered_set<std::string> inlined_functions;
    
    // 内联深度限制，防止过度内联
    int inline_depth_limit;
    int current_depth;
    
    // 内联大小限制，防止内联过大的函数
    int inline_size_limit;
    
public:
    InlineOptimizer(int depth_limit = 3, int size_limit = 50);
    
    // 对程序进行函数内联优化
    void optimize(Program* program);
    
    // 对单个函数进行内联优化
    void optimize_function(Function* func);
    
    // 对基本块进行内联优化
    void optimize_basic_block(BasicBlock* bb);
    
    // 检查函数是否可以内联
    bool can_inline(const Function* func) const;
    
    // 检查函数调用是否可以内联
    bool can_inline_call(const CallValue* call, const Function* callee) const;
    
    // 执行函数内联，返回内联后的指令列表
    std::vector<std::unique_ptr<IRValue>> inline_function_call(CallValue* call, Function* callee);
    
    // 计算函数的大小（指令数量）
    int calculate_function_size(const Function* func) const;
    
    // 计算基本块的大小（指令数量）
    int calculate_basic_block_size(const BasicBlock* bb) const;
    
    // 替换参数引用
    std::string replace_param_reference(const std::string& param_name, 
                                       const std::vector<std::string>& args);
    
    // 生成新的临时变量名
    std::string generate_temp_name();
    
    // 克隆指令（用于内联）
    std::unique_ptr<IRValue> clone_instruction(const IRValue* inst, 
                                              std::unordered_map<std::string, std::string>& var_mapping,
                                              const std::vector<std::string>& arg_names);
    
private:
    static int temp_counter;
};

#endif // INLINE_H 