#include "inline.h"
#include <iostream>
#include <sstream>
#include <algorithm>

int InlineOptimizer::temp_counter = 0;

InlineOptimizer::InlineOptimizer(int depth_limit, int size_limit) 
    : inline_depth_limit(depth_limit), current_depth(0), inline_size_limit(size_limit) {
}

void InlineOptimizer::optimize(Program* program) {
    // 首先建立函数映射表
    for (auto& func : program->funcs) {
        function_map[func->name] = func.get();
    }
    
    // 对每个函数进行内联优化
    for (auto& func : program->funcs) {
        optimize_function(func.get());
    }
}

void InlineOptimizer::optimize_function(Function* func) {
    // 对每个基本块进行内联优化
    for (auto& bb : func->bbs) {
        optimize_basic_block(bb.get());
    }
}

void InlineOptimizer::optimize_basic_block(BasicBlock* bb) {
    std::vector<std::unique_ptr<IRValue>> new_instructions;
    
    for (size_t i = 0; i < bb->insts.size(); ++i) {
        auto& inst = bb->insts[i];
        
        if (inst->v_tag == IRValueTag::CALL) {
            auto* call = dynamic_cast<CallValue*>(inst.get());
            if (call) {
                std::string callee_name = call->callee;
                auto it = function_map.find(callee_name);
                
                if (it != function_map.end() && can_inline_call(call, it->second)) {
                    // 执行内联，将内联后的指令添加到新指令列表
                    std::vector<std::unique_ptr<IRValue>> inlined_instructions = 
                        inline_function_call(call, it->second);
                    
                    // 将内联的指令添加到新指令列表
                    for (auto& inlined_inst : inlined_instructions) {
                        new_instructions.push_back(std::move(inlined_inst));
                    }
                    
                    // 跳过原来的调用指令
                    continue;
                }
            }
        }
        
        // 保留原指令
        new_instructions.push_back(std::move(inst));
    }
    
    // 替换指令列表
    bb->insts = std::move(new_instructions);
}

bool InlineOptimizer::can_inline(const Function* func) const {
    // 检查函数是否已经内联过（避免递归内联）
    if (inlined_functions.find(func->name) != inlined_functions.end()) {
        return false;
    }
    
    // 检查内联深度
    if (current_depth >= inline_depth_limit) {
        return false;
    }
    
    // 检查函数大小
    int func_size = calculate_function_size(func);
    if (func_size > inline_size_limit) {
        return false;
    }
    
    // 检查函数是否有递归调用
    for (const auto& bb : func->bbs) {
        for (const auto& inst : bb->insts) {
            if (inst->v_tag == IRValueTag::CALL) {
                const auto* call = dynamic_cast<const CallValue*>(inst.get());
                if (call && call->callee == func->name) {
                    return false; // 递归函数不内联
                }
            }
        }
    }
    
    return true;
}

bool InlineOptimizer::can_inline_call(const CallValue* call, const Function* callee) const {
    // 检查被调用函数是否可以内联
    if (!can_inline(callee)) {
        return false;
    }
    
    // 检查参数数量是否匹配
    if (call->args.size() != callee->params.size()) {
        return false;
    }
    
    // 检查是否有复杂的控制流（多个基本块）
    if (callee->bbs.size() > 3) {
        return false;
    }
    
    return true;
}

std::vector<std::unique_ptr<IRValue>> InlineOptimizer::inline_function_call(CallValue* call, Function* callee) {
    current_depth++;
    inlined_functions.insert(callee->name);
    
    std::vector<std::unique_ptr<IRValue>> inlined_instructions;
    
    // 创建参数映射
    std::vector<std::string> arg_names;
    for (const auto& arg : call->args) {
        arg_names.push_back(arg->name);
    }
    
    // 为内联的函数体创建新的临时变量名
    std::unordered_map<std::string, std::string> var_mapping;
    
    // 复制函数体的指令
    for (const auto& callee_bb : callee->bbs) {
        for (const auto& inst : callee_bb->insts) {
            auto new_inst = clone_instruction(inst.get(), var_mapping, arg_names);
            if (new_inst) {
                inlined_instructions.push_back(std::move(new_inst));
            }
        }
    }
    
    // 如果函数有返回值，需要处理返回值
    if (!call->name.empty() && callee->f_type->return_type->isInt32()) {
        if (var_mapping.find("%ret_val") != var_mapping.end()) {
            std::string return_var = call->name;
            auto load_inst = std::make_unique<LoadValue>(
                return_var, 
                std::make_unique<VarRefValue>(var_mapping["%ret_val"]),
                0
            );
            inlined_instructions.push_back(std::move(load_inst));
        }
    }
    
    current_depth--;
    inlined_functions.erase(callee->name);
    
    return inlined_instructions;
}

std::unique_ptr<IRValue> InlineOptimizer::clone_instruction(
    const IRValue* inst, 
    std::unordered_map<std::string, std::string>& var_mapping,
    const std::vector<std::string>& arg_names) {
    
    switch (inst->v_tag) {
        case IRValueTag::ALLOC: {
            const auto* alloc = dynamic_cast<const AllocValue*>(inst);
            std::string new_name = generate_temp_name();
            var_mapping[alloc->name] = new_name;
            return std::make_unique<AllocValue>(new_name);
        }
        
        case IRValueTag::LOAD: {
            const auto* load = dynamic_cast<const LoadValue*>(inst);
            std::string new_name = generate_temp_name();
            var_mapping[load->name] = new_name;
            
            std::string new_src = replace_param_reference(load->src->name, arg_names);
            if (var_mapping.find(new_src) != var_mapping.end()) {
                new_src = var_mapping[new_src];
            }
            
            return std::make_unique<LoadValue>(
                new_name,
                std::make_unique<VarRefValue>(new_src),
                load->type
            );
        }
        
        case IRValueTag::STORE: {
            const auto* store = dynamic_cast<const StoreValue*>(inst);
            
            std::string new_value = replace_param_reference(store->value->name, arg_names);
            if (var_mapping.find(new_value) != var_mapping.end()) {
                new_value = var_mapping[new_value];
            }
            
            std::string new_dest = replace_param_reference(store->dest->name, arg_names);
            if (var_mapping.find(new_dest) != var_mapping.end()) {
                new_dest = var_mapping[new_dest];
            }
            
            return std::make_unique<StoreValue>(
                std::make_unique<VarRefValue>(new_value),
                std::make_unique<VarRefValue>(new_dest)
            );
        }
        
        case IRValueTag::BINARY: {
            const auto* binary = dynamic_cast<const BinaryValue*>(inst);
            std::string new_name = generate_temp_name();
            var_mapping[binary->name] = new_name;
            
            std::string new_lhs = replace_param_reference(binary->lhs->name, arg_names);
            if (var_mapping.find(new_lhs) != var_mapping.end()) {
                new_lhs = var_mapping[new_lhs];
            }
            
            std::string new_rhs = replace_param_reference(binary->rhs->name, arg_names);
            if (var_mapping.find(new_rhs) != var_mapping.end()) {
                new_rhs = var_mapping[new_rhs];
            }
            
            return std::make_unique<BinaryValue>(
                new_name,
                binary->op,
                std::make_unique<VarRefValue>(new_lhs),
                std::make_unique<VarRefValue>(new_rhs)
            );
        }
        
        case IRValueTag::RETURN: {
            const auto* ret = dynamic_cast<const ReturnValue*>(inst);
            if (ret->value) {
                std::string new_value = replace_param_reference(ret->value->name, arg_names);
                if (var_mapping.find(new_value) != var_mapping.end()) {
                    new_value = var_mapping[new_value];
                }
                
                // 将返回值存储到临时变量中
                std::string ret_var = generate_temp_name();
                var_mapping["%ret_val"] = ret_var;
                
                auto store_ret = std::make_unique<StoreValue>(
                    std::make_unique<VarRefValue>(new_value),
                    std::make_unique<VarRefValue>(ret_var)
                );
                return store_ret;
            }
            return nullptr; // void return
        }
        
        default:
            // 其他指令类型暂时不处理
            return nullptr;
    }
}

std::string InlineOptimizer::replace_param_reference(
    const std::string& param_name, 
    const std::vector<std::string>& args) {
    
    // 检查是否是参数引用（函数参数通常以@开头）
    if (param_name.find("@") == 0) {
        // 尝试从参数名中提取索引
        // 参数名格式可能是 "@SYM_TABLE_X_param_name" 或 "@param_name"
        std::string param_base_name = param_name;
        
        // 移除作用域前缀
        size_t last_underscore = param_base_name.find_last_of('_');
        if (last_underscore != std::string::npos) {
            param_base_name = param_base_name.substr(last_underscore + 1);
        }
        
        // 查找对应的参数
        for (size_t i = 0; i < args.size(); ++i) {
            // 这里简化处理，假设参数按顺序对应
            if (i < args.size()) {
                return args[i];
            }
        }
    }
    
    return param_name;
}

std::string InlineOptimizer::generate_temp_name() {
    std::ostringstream oss;
    oss << "%inline_" << temp_counter++;
    return oss.str();
}

int InlineOptimizer::calculate_function_size(const Function* func) const {
    int size = 0;
    for (const auto& bb : func->bbs) {
        size += calculate_basic_block_size(bb.get());
    }
    return size;
}

int InlineOptimizer::calculate_basic_block_size(const BasicBlock* bb) const {
    return static_cast<int>(bb->insts.size());
} 