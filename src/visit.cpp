#include "visit.h"

#include <iostream>
#include <sstream>
#include <unordered_map>

static std::unordered_map<std::string, int> func_param_counts;

static int current_func_param_count;
static int max_calling_param_count;
static int if_call_other_functions;
static int local_var_count;
static int ra_space;

// stack sturcture
// high: ra (if call other functions)
//       local variables
// low:  parameters (if calling other functions needs more than 8 parameters, the rest will be passed in stack)
static int stack_size;

static int cur_local_var_index;
static std::unordered_map<std::string, std::string> local_var_indices;

std::string get_local_var_index(std::string var_name) {
    //std::cout << "looking for local variable " << var_name << "\n";
    if (local_var_indices.find(var_name) == local_var_indices.end()) {
        local_var_indices[var_name] = std::to_string(cur_local_var_index) + "(sp)";
        cur_local_var_index += 4;;
    }
    return local_var_indices[var_name];
}

std::string visit_program(std::unique_ptr<Program> program) {
    // init works
    for (const auto &func: program->funcs)  {
        // record the number of parameters for each function
        func_param_counts[func->get_func_name()] = func->get_param_count();
    }

    // begin to visit
    std::ostringstream oss;

    oss << "  .globl main\n";
    for (const auto &func : program->funcs) {
        oss << visit_function(std::move(func)) << "\n";
    }

    return oss.str();
}

std::string visit_function(const std::unique_ptr<Function> &func) {
    std::ostringstream oss;

    // entry of the function
    oss <<  func->get_func_name() << ":\n";
    current_func_param_count = func->get_param_count();

    // calculate the stack size that needs to be allocated
    // 1. parameters space
    current_func_param_count = func_param_counts[func->get_func_name()];
    // 2. local variables space
    local_var_count = func->local_var_count;
    // 3. space for calling other functions when its parameter count is more than 8
    //std::cout << "visiting function, calculating the extra space needed for calling other functions.\n";
    for (const auto &bb: func->bbs) {
        for (const auto &inst : bb->insts) {
            if (inst->v_tag == IRValueTag::CALL) {
                if_call_other_functions = 1;
                auto *call_value = dynamic_cast<CallValue*>(inst.get());
                if (call_value) {
                    max_calling_param_count = std::max(max_calling_param_count, static_cast<int>(call_value->args.size()));
                }
            }
        }
    }
    // 4. ra if this function calls other functions
    ra_space = (if_call_other_functions == 1) ? 4 : 0;
    int extra_param_count_for_calling = std::max(0, max_calling_param_count - 8);
    int temp = 4 * (local_var_count + ra_space + extra_param_count_for_calling);
    // align to 16
    stack_size = (temp + 15) & ~15;

    // set the static values for visiting this function
    cur_local_var_index = extra_param_count_for_calling * 4;
    local_var_indices.clear();

    // prologue
    oss << "  addi sp, sp, -" << stack_size << "\n";
    if (if_call_other_functions != 0) {
        // should done by caller, but we do ahead.
        oss << "  sw ra, " << (stack_size - 4) << "(sp)\n"; // save return address
    }

    // set indices for arguments
    //std::cout << "visiting function, setting indices for parameters.\n";
    for (const auto& param: func->params) {
        auto *param_ref = dynamic_cast<FuncArgRefValue*>(param.get());
        if (param_ref) {
            if (param_ref->index < 8) {
                local_var_indices[param_ref->name] = "a" + std::to_string(param_ref->index);
            } else {
                // extra parameters are stored in the stack of function who calls this function
                local_var_indices[param_ref->name] =
                    std::to_string(stack_size + 4 * (param_ref->index - 8)) + "(sp)";
            }
        } else {
            throw std::runtime_error("Expecting FuncArgRefValue in function parameters");
        }
    }

    // visit basic blocks
    // epilogue is done in return instruction
    for (const auto &bb : func->bbs) {
        oss << visit_basic_block(std::move(bb)) << "\n";
    }

    return oss.str();
}

std::string visit_basic_block(const std::unique_ptr<BasicBlock> &bb) {
    std::ostringstream oss;

    // visit each instruction in the basic block
    if (bb->get_name() != "entry") {
        oss << bb->get_name()  << ":\n";
    }

    for (const auto &inst : bb->insts) {
        oss << visit_value(std::move(inst)) << "\n";
    }

    return oss.str();
}

std::string visit_value(const std::unique_ptr<IRValue> &value) {
    std::ostringstream oss;

    switch (value->v_tag) {
        case IRValueTag::ALLOC: {
            auto *v = dynamic_cast<AllocValue*>(value.get());
            visit_alloc_value(v);
            //oss << visit_alloc_value(v) << "\n";
            break;
        }
        case IRValueTag::BINARY: {
            auto *v = dynamic_cast<BinaryValue*>(value.get());
            oss << visit_binary_value(v) << "\n";
            break;
        }
        case IRValueTag::LOAD: {
            auto *v = dynamic_cast<LoadValue*>(value.get());
            oss << visit_load_value(v) << "\n";
            break;
        }
        case IRValueTag::STORE: {
            auto *v  = dynamic_cast<StoreValue*>(value.get());
            oss << visit_store_value(v) << "\n";
            break;
        }
        case IRValueTag::CALL: {
            auto *v  = dynamic_cast<CallValue*>(value.get());
            oss << visit_call_value(v) << "\n";
            break;
        }
        case IRValueTag::RETURN: {
            auto *v = dynamic_cast<ReturnValue*>(value.get());
            oss << visit_return_value(v) << "\n";
            break;
        }
        case IRValueTag::BRANCH: {
            auto *v = dynamic_cast<BranchValue*>(value.get());
            oss << visit_branch_value(v) << "\n";
            break;
        }
        case IRValueTag::JUMP: {
            auto *v = dynamic_cast<JumpValue*>(value.get());
            oss << visit_jump_value(v) << "\n";
            break;
        }
        default: {
            std::cout << value->toString() << "\n";
            throw std::runtime_error("Unhandled cases. (A possible case, a number or a variable itself as a statement.)");
        };
    }

    return oss.str();
}

void visit_alloc_value(const AllocValue* value) {
    local_var_indices[value->name] = std::to_string(cur_local_var_index) + "(sp)";
    cur_local_var_index += 4;
}

std::string visit_load_value(const LoadValue* value) {
    std::ostringstream oss;
    if (value->type == 0) {
        std::string src_index = get_local_var_index(value->src->name);
        std::string result_index = get_local_var_index(value->name);

        oss << "  lw t0, " << src_index << "\n";
        oss << "  sw t0, " << result_index;

    } else if (value->type == 1) {
        // load immediate value
        std::string result_index = get_local_var_index(value->name);
        auto int_value = dynamic_cast<IntergerValue*>(value->src.get());
        oss << "  li t0, " << int_value->value << "\n";
        oss << "  sw t0, " << result_index;
    } else {
        throw std::runtime_error("Unknown load type");
    }

    return oss.str();
}

std::string visit_binary_value(const  BinaryValue* value) {
    std::ostringstream oss;

    std::string lhs_index = get_local_var_index(value->lhs->name);
    std::string rhs_index = get_local_var_index(value->rhs->name);
    std::string result_index = get_local_var_index(value->name);

    oss << "  lw t0, " << lhs_index << "\n";
    oss << "  lw t1, " << rhs_index << "\n";
    if (value->op == BinaryOp::ADD) {
        oss << "  add t2, t0, t1\n";
    } else if (value->op == BinaryOp::SUB) {
        oss << "  sub t2, t0, t1\n";
    } else if (value->op == BinaryOp::MUL) {
        oss << "  mul t2, t0, t1\n";
    } else if (value->op == BinaryOp::DIV) {
        oss << "  div t2, t0, t1\n";
    } else if (value->op == BinaryOp::MOD) {
        oss << "  rem t2, t0, t1\n";
    } else if (value->op == BinaryOp::EQ) {
        oss << "  sub t2, t0, t1\n";
        oss << "  seqz t2, t2\n";
    } else if (value->op == BinaryOp::NE) {
        oss << "  sub t2, t0, t1\n";
        oss << "  snez t2, t2\n";
    } else if (value->op == BinaryOp::LT) {
        oss << "  slt t2, t0, t1\n";
    } else if (value->op == BinaryOp::LE) {
        oss << "  sgt t2, t0, t1\n";
        oss << "  seqz t2, t2\n";
    } else if (value->op == BinaryOp::GT) {
        oss << "  sgt t2, t0, t1\n";
    } else if (value->op == BinaryOp::GE) {
        oss << "  slt t2, t0, t1\n";
        oss << "  seqz t2, t2\n";
    } else if (value->op == BinaryOp::AND) {
        oss << "  and t2, t0, t1\n";
    } else if (value->op == BinaryOp::OR) {
        oss << "  or t2, t0, t1\n";
    } else if (value->op == BinaryOp::XOR) {
        oss << "  xor t2, t0, t1\n";
    } else if (value->op == BinaryOp::SHL) {
        oss << "  sll t2, t0, t1\n";
    } else if (value->op == BinaryOp::SHR) {
        oss << "  srl t2, t0, t1\n";
    } else if (value->op == BinaryOp::SAR) {
        oss << "  sra t2, t0, t1\n";
    } else {
        throw std::runtime_error("Unknown binary operation");
    }

    oss << "  sw t2, " << result_index;
    return oss.str();
}

std::string visit_store_value(const StoreValue* value) {
    std::ostringstream oss;

    std::string src_index = get_local_var_index(value->value->name);
    std::string dest_index = get_local_var_index(value->dest->name);

    // TODO: hot fix. NOT a good way to handle this.
    if (src_index.at(0) == 'a') {
        oss << "  mv t0, " << src_index << "\n";
        oss << "  sw t0, " << dest_index << "\n";
    } else {
        oss << "  lw t0, " << src_index << "\n";
        oss << "  sw t0, " << dest_index;
    }

    return oss.str();
}

std::string visit_call_value(const CallValue* value) {
    std::ostringstream oss;

    // prepare arguments
    int arg_count = value->args.size();
    for (int i = 0; i < arg_count; ++i) {
        std::string arg_index = get_local_var_index(value->args[i]->name);
        if (i < 8) {
            oss << "  lw a" << i << ", " << arg_index << "\n"; // a0-a7
        } else {
            oss << "  lw t0, " << arg_index << "\n";
            oss << "  sw t0, " <<  4 * (i - 8) << "(sp)\n"; // store to stack
        }
    }

    // call the function
    oss << "  call " << value->get_callee() << "\n";

    // restore ra, done by caller
    oss << "  lw ra, " << (stack_size - 4) << "(sp)\n";

    // save return value
    if (!value->name.empty()) {
        std::string result_index = get_local_var_index(value->name);
        oss << "  sw a0, " << result_index << "\n"; // return value in a0
    }

    return oss.str();
}

std::string visit_return_value(const ReturnValue* value) {
    std::ostringstream oss;
    if (value->value != nullptr) {
        oss << "  lw a0, " << get_local_var_index(value->value->name) << "\n"; // return value in a0
    }
    // do epilogue
    oss << "  addi sp, sp, " << stack_size << "\n"; // restore stack pointer
    oss << "  ret\n"; // return from function

    return oss.str();
}

std::string visit_branch_value(const BranchValue* value) {
    std::ostringstream oss;

    oss << "  lw t0, " << get_local_var_index(value->cond->name) << "\n"; // load condition
    oss << "  beqz t0, " << value->false_block.substr(1) << "\n"; // if condition is zero, branch to false block
    oss << "  j " << value->true_block.substr(1) << "\n"; // otherwise, jump to true block

    return oss.str();
}

std::string visit_jump_value(const JumpValue* value) {
    std::ostringstream oss;

    oss << "  j " << value->target_block.substr(1) << "\n"; // jump to target block

    return oss.str();
}


