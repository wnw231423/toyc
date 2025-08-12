#include "visit.h"
#include "rv_defs.h"
#include "register_allocation.h"

#include <cassert>
#include <cstring>
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
static std::unordered_map<std::string, Position> local_var_indices;

Position get_local_var_index(std::string var_name) {
    //std::cout << "looking for local variable " << var_name << "\n";
    if (strncmp(var_name.c_str(), "$imm_", 5) == 0) {
        int imm_value = std::stoi(var_name.substr(5));
        return Position(2, imm_value); // use t0 to hold immediate values
    }
    if (local_var_indices.find(var_name) == local_var_indices.end()) {
        Position pos = Position(cur_local_var_index);
        local_var_indices[var_name] = pos;
        cur_local_var_index += 4;
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
    for (const auto &func : program->funcs) {
        oss << "  .text\n";
        oss << "  .globl " << func->get_func_name() << "\n";
        oss << visit_function(std::move(func)) << "\n";
    }

    return oss.str();
}

std::string visit_function(const std::unique_ptr<Function> &func) {
    local_var_indices.clear();

    RegisterAllocator allocator;
    auto liveness = allocator.performLivenessAnalysis(func.get());
    auto allocation = allocator.performLinearScanAllocation(liveness);
    for (const auto pair : allocation.var_to_reg) {
        // std::cout << "Variable " << pair.first << " is assigned to register " << pair.second << "\n";
        Position pos = Position(pair.second);
        local_var_indices[pair.first] = pos; // update local_var_indices with register positions
    }
    for (const auto pair : allocation.var_to_spill_location) {
        Position pos = Position(pair.second);
        local_var_indices[pair.first] = pos; // update local_var_indices with stack positions
    }
    int max_spill_slots = allocation.max_spill_slots;

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
    //int temp = 4 * (local_var_count + extra_param_count_for_calling) + ra_space;
    int temp = 4 * (max_spill_slots + extra_param_count_for_calling) + ra_space + 12 * 4; // 12 is the save register s0-s11, which is 12 * 4 bytes
    // align to 16
    stack_size = (temp + 15) & ~15;

    // set the static values for visiting this function
    cur_local_var_index = extra_param_count_for_calling * 4;
    //local_var_indices.clear();

    // prologue
    // 1. add stack pointer
    // 2. save ra if this function calls other functions
    // 3. save s0-s11
    if (stack_size < 2048 && stack_size >= -2048) {
        oss << "  addi sp, sp, -" << stack_size << "\n";
    } else {
        oss << "  li t6, " << -stack_size << "\n" // load immediate value
            << "  add sp, sp, t6\n"; // adjust stack pointer
    }

    if (if_call_other_functions != 0) {
        // should done by caller, but we do ahead.
        //oss << "  sw ra, " << (stack_size - 4) << "(sp)\n"; // save return address
        // std::string mem = std::to_string(stack_size - 4) + "(sp)";
        // oss << save_to_mem("ra", mem); // save return address

        Position ra_mem(stack_size - 4);
        Position ra("ra");
        oss << move(ra, ra_mem); // save return address
    }

    // save s0-s11
    for (int i = 0; i < 12; ++i) {
        Position s_i("s" + std::to_string(i));
        Position s_i_mem(stack_size - 4 * (i + 2)); // s0-s11 are saved in the stack
        oss << move(s_i, s_i_mem) << "\n"; // save s0-s11
    }

    // set indices for arguments
    //std::cout << "visiting function, setting indices for parameters.\n";
    for (const auto& param: func->params) {
        auto *param_ref = dynamic_cast<FuncArgRefValue*>(param.get());
        if (param_ref) {
            if (param_ref->index < 8) {
                local_var_indices[param_ref->name] = Position("a" + std::to_string(param_ref->index));
            } else {
                // extra parameters are stored in the stack of function who calls this function
                //local_var_indices[param_ref->name] = std::to_string(stack_size + 4 * (param_ref->index - 8)) + "(sp)";
                local_var_indices[param_ref->name] = Position(stack_size + 4 * (param_ref->index - 8));
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
            //visit_alloc_value(v);
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
    local_var_indices[value->name] = Position(cur_local_var_index);
    cur_local_var_index += 4;
}

std::string visit_load_value(const LoadValue* value) {
    std::ostringstream oss;
    if (value->type == 0) {
        Position src_index = get_local_var_index(value->src->name);
        Position result_index = get_local_var_index(value->name);

        oss << move(src_index, result_index) << "\n";

    } else if (value->type == 1) {
        // load immediate value
        Position result_index = get_local_var_index(value->name);
        auto int_value = dynamic_cast<IntergerValue*>(value->src.get());
        oss << "  li t0, " << int_value->value << "\n";
        Position t0 = Position("t0");
        oss << move(t0, result_index) << "\n";
    } else {
        throw std::runtime_error("Unknown load type");
    }

    return oss.str();
}

std::string visit_binary_value(const  BinaryValue* value) {
    std::ostringstream oss;

    Position lhs_index = get_local_var_index(value->lhs->name);
    Position rhs_index = get_local_var_index(value->rhs->name);
    Position result_index = get_local_var_index(value->name);

    Position t0 = Position("t0");
    Position t1 = Position("t1");
    oss << move(lhs_index, t0) << "\n"; // load lhs into t0
    oss << move(rhs_index, t1) << "\n"; // load rhs into t1
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

    //oss << "  sw t2, " << result_index;
    Position t2 = Position("t2");
    oss << move(t2, result_index) << "\n"; // store result in result_index
    return oss.str();
}

std::string visit_store_value(const StoreValue* value) {
    std::ostringstream oss;

    Position src_index = get_local_var_index(value->value->name);
    Position dest_index = get_local_var_index(value->dest->name);

    oss << move(src_index, dest_index) << "\n"; // store value in destination

    return oss.str();
}

std::string visit_call_value(const CallValue* value) {
    std::ostringstream oss;

    // prepare arguments
    int arg_count = value->args.size();
    for (int i = 0; i < arg_count; ++i) {
        Position arg_index = get_local_var_index(value->args[i]->name);
        if (i < 8) {
            //oss << "  lw a" << i << ", " << arg_index << "\n"; // a0-a7
            Position a_i("a" + std::to_string(i));
            oss << move(arg_index, a_i) << "\n"; // move argument to a0-a7
        } else {
            // oss << "  lw t0, " << arg_index << "\n";
            // oss << "  sw t0, " <<  4 * (i - 8) << "(sp)\n"; // store to stack

            Position mem_index(4 * (i - 8));
            oss << move(arg_index, mem_index) << "\n"; // store to stack
        }
    }

    // call the function
    oss << "  call " << value->get_callee() << "\n";

    // restore ra, done by caller
    Position ra("ra");
    Position ra_mem(stack_size - 4);
    oss << move(ra_mem, ra) << "\n"; // restore return address

    // save return value
    if (!value->name.empty()) {
        Position result_index = get_local_var_index(value->name);
        Position a0("a0");
        //oss << "  sw a0, " << result_index << "\n"; // return value in a0
        oss << move(a0, result_index) << "\n"; // move return value to result_index
    }

    return oss.str();
}

std::string visit_return_value(const ReturnValue* value) {
    std::ostringstream oss;

    // do epilogue
    // 1. add stack pointer
    // 2. restore s0-s11
    // 3. move return value to a0
    if (stack_size < 2048 && stack_size >= -2048) {
        oss << "  addi sp, sp, " << stack_size << "\n";
    } else {
        oss << "  li t6, " << stack_size << "\n" // load immediate value
            << "  add sp, sp, t6\n"; // adjust stack pointer
    }

    for (int i = 0; i < 12; ++i) {
        Position s_i("s" + std::to_string(i));
        Position s_i_mem(stack_size - 4 * (i + 2)); // s0-s11 are saved in the stack
        oss << move(s_i_mem, s_i) << "\n"; // restore s0-s11
    }

    if (value->value != nullptr) {
        //oss << "  lw a0, " << get_local_var_index(value->value->name) << "\n"; // return value in a0
        Position a0("a0");
        Position return_value_index = get_local_var_index(value->value->name);
        oss << move(return_value_index, a0) << "\n"; // move return value to a0
    }

    oss << "  ret\n"; // return from function

    return oss.str();
}

std::string visit_branch_value(const BranchValue* value) {
    std::ostringstream oss;

    // oss << "  lw t0, " << get_local_var_index(value->cond->name) << "\n"; // load condition
    Position cond_index = get_local_var_index(value->cond->name);
    Position t0("t0");
    oss << move(cond_index, t0) << "\n"; // move condition to t0
    oss << "  beqz t0, " << value->false_block.substr(1) << "\n"; // if condition is zero, branch to false block
    oss << "  j " << value->true_block.substr(1) << "\n"; // otherwise, jump to true block

    return oss.str();
}

std::string visit_jump_value(const JumpValue* value) {
    std::ostringstream oss;

    oss << "  j " << value->target_block.substr(1) << "\n"; // jump to target block

    return oss.str();
}


