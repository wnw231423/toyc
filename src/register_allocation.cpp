#define DEBUG

#include "register_allocation.h"
#include <iostream>
#include <algorithm>
#include <queue>

// 构建控制流图
void RegisterAllocator::buildControlFlowGraph(Function* func) {
    cfg_successors.clear();
    cfg_predecessors.clear();

    for (const auto& bb : func->bbs) {
        const std::string& bb_name = bb->name;
        cfg_successors[bb_name] = std::vector<std::string>();
        cfg_predecessors[bb_name] = std::vector<std::string>();
    }

    // 分析每个基本块的后继
    for (size_t i = 0; i < func->bbs.size(); i++) {
        const auto& bb = func->bbs[i];
        const std::string& bb_name = bb->name;

        if (!bb->insts.empty()) {
            auto& last_inst = bb->insts.back();

            if (last_inst->v_tag == IRValueTag::BRANCH) {
                auto* branch = dynamic_cast<BranchValue*>(last_inst.get());
                if (branch) {
                    cfg_successors[bb_name].push_back(branch->true_block);
                    cfg_successors[bb_name].push_back(branch->false_block);
                    cfg_predecessors[branch->true_block].push_back(bb_name);
                    cfg_predecessors[branch->false_block].push_back(bb_name);
                }
            } else if (last_inst->v_tag == IRValueTag::JUMP) {
                auto* jump = dynamic_cast<JumpValue*>(last_inst.get());
                if (jump) {
                    cfg_successors[bb_name].push_back(jump->target_block);
                    cfg_predecessors[jump->target_block].push_back(bb_name);
                }
            } else if (last_inst->v_tag != IRValueTag::RETURN && i + 1 < func->bbs.size()) {
                // 如果不是return指令，默认连接到下一个基本块
                const std::string& next_bb_name = func->bbs[i + 1]->name;
                cfg_successors[bb_name].push_back(next_bb_name);
                cfg_predecessors[next_bb_name].push_back(bb_name);
            }
        } else if (i + 1 < func->bbs.size()) {
            // 空基本块也连接到下一个基本块
            const std::string& next_bb_name = func->bbs[i + 1]->name;
            cfg_successors[bb_name].push_back(next_bb_name);
            cfg_predecessors[next_bb_name].push_back(bb_name);
        }
    }

#ifdef DEBUG
    std::cout << "=== 控制流图 ===" << std::endl;
    for (const auto& bb : func->bbs) {
        const std::string& bb_name = bb->name;
        std::cout << "基本块 " << bb_name << ":" << std::endl;
        std::cout << "  后继: ";
        for (const auto& succ : cfg_successors[bb_name]) {
            std::cout << succ << " ";
        }
        std::cout << std::endl;
        std::cout << "  前驱: ";
        for (const auto& pred : cfg_predecessors[bb_name]) {
            std::cout << pred << " ";
        }
        std::cout << std::endl;
    }
#endif
}

// 为指令编号
void RegisterAllocator::numberInstructions(Function* func) {
    instruction_numbers.clear();
    total_instructions = 0;

    for (const auto& bb : func->bbs) {
        for (const auto& inst : bb->insts) {
            // 使用指令的字符串表示作为唯一标识
            instruction_numbers[inst->toString()] = total_instructions++;
        }
    }

#ifdef DEBUG
    std::cout << "=== 指令编号 ===" << std::endl;
    int inst_num = 0;
    for (const auto& bb : func->bbs) {
        std::cout << "基本块 " << bb->name << ":" << std::endl;
        for (const auto& inst : bb->insts) {
            std::cout << "  " << inst_num << ": " << inst->toString() << std::endl;
            inst_num++;
        }
    }
#endif
}

// 检查变量是否在函数内定义（不包括参数）
bool RegisterAllocator::isVariableDefinedInFunction(const std::string& var_name, Function* func) {
    // 参数不算函数内定义的变量
    for (const auto& param : func->params) {
        if (param->name == var_name) {
            return false;
        }
    }

    // 遍历所有指令，查找定义该变量的指令
    for (const auto& bb : func->bbs) {
        for (const auto& inst : bb->insts) {
            auto defined_vars = getDefinedVars(inst.get());
            for (const auto& var : defined_vars) {
                if (var == var_name) {
                    return true;
                }
            }
        }
    }

    return false;
}

// 从指令中提取定义的变量
std::vector<std::string> RegisterAllocator::getDefinedVars(const IRValue* inst) {
    std::vector<std::string> defined;

    switch (inst->v_tag) {
        case IRValueTag::ALLOC:
        case IRValueTag::LOAD:
        case IRValueTag::BINARY:
        case IRValueTag::CALL:
            if (!inst->name.empty()) {
                defined.push_back(inst->name);
            }
            break;
        default:
            break;
    }

    return defined;
}

// 从指令中提取使用的变量
std::vector<std::string> RegisterAllocator::getUsedVars(const IRValue* inst) {
    std::vector<std::string> used;

    switch (inst->v_tag) {
        case IRValueTag::LOAD: {
            auto* load = dynamic_cast<const LoadValue*>(inst);
            if (load && load->src) {
                if (load->src->v_tag == IRValueTag::VAR_REF && !load->src->name.empty()) {
                    used.push_back(load->src->name);
                }
            }
            break;
        }
        case IRValueTag::STORE: {
            auto* store = dynamic_cast<const StoreValue*>(inst);
            if (store) {
                if (store->value && store->value->v_tag == IRValueTag::VAR_REF && !store->value->name.empty()) {
                    used.push_back(store->value->name);
                }
                if (store->dest && store->dest->v_tag == IRValueTag::VAR_REF && !store->dest->name.empty()) {
                    used.push_back(store->dest->name);
                }
            }
            break;
        }
        case IRValueTag::BINARY: {
            auto* binary = dynamic_cast<const BinaryValue*>(inst);
            if (binary) {
                if (binary->lhs && binary->lhs->v_tag == IRValueTag::VAR_REF && !binary->lhs->name.empty()) {
                    used.push_back(binary->lhs->name);
                }
                if (binary->rhs && binary->rhs->v_tag == IRValueTag::VAR_REF && !binary->rhs->name.empty()) {
                    used.push_back(binary->rhs->name);
                }
            }
            break;
        }
        case IRValueTag::BRANCH: {
            auto* branch = dynamic_cast<const BranchValue*>(inst);
            if (branch && branch->cond && branch->cond->v_tag == IRValueTag::VAR_REF && !branch->cond->name.empty()) {
                used.push_back(branch->cond->name);
            }
            break;
        }
        case IRValueTag::RETURN: {
            auto* ret = dynamic_cast<const ReturnValue*>(inst);
            if (ret && ret->value && ret->value->v_tag == IRValueTag::VAR_REF && !ret->value->name.empty()) {
                used.push_back(ret->value->name);
            }
            break;
        }
        case IRValueTag::CALL: {
            auto* call = dynamic_cast<const CallValue*>(inst);
            if (call) {
                for (const auto& arg : call->args) {
                    if (arg->v_tag == IRValueTag::VAR_REF && !arg->name.empty()) {
                        used.push_back(arg->name);
                    }
                }
            }
            break;
        }
        default:
            break;
    }

    return used;
}

// 计算每个基本块的def和use集合
void RegisterAllocator::computeDefUse(Function* func, LivenessAnalysis& analysis) {
    for (const auto& bb : func->bbs) {
        const std::string& bb_name = bb->name;
        analysis.def[bb_name].clear();
        analysis.use[bb_name].clear();

        // 遍历基本块中的每条指令
        for (const auto& inst : bb->insts) {
            // 先处理use（在def之前使用的变量）
            auto used_vars = getUsedVars(inst.get());
            for (const std::string& var : used_vars) {
                if (analysis.def[bb_name].find(var) == analysis.def[bb_name].end()) {
                    analysis.use[bb_name].insert(var);
                }
            }

            // 再处理def
            auto defined_vars = getDefinedVars(inst.get());
            for (const std::string& var : defined_vars) {
                analysis.def[bb_name].insert(var);
            }
        }
    }

#ifdef DEBUG
    std::cout << "=== DEF/USE 集合 ===" << std::endl;
    for (const auto& bb : func->bbs) {
        const std::string& bb_name = bb->name;
        std::cout << "基本块 " << bb_name << ":" << std::endl;

        std::cout << "  def: { ";
        for (const std::string& var : analysis.def[bb_name]) {
            std::cout << var << " ";
        }
        std::cout << "}" << std::endl;

        std::cout << "  use: { ";
        for (const std::string& var : analysis.use[bb_name]) {
            std::cout << var << " ";
        }
        std::cout << "}" << std::endl;
    }
#endif
}

// 计算活跃变量的in和out集合（数据流分析）
void RegisterAllocator::computeLiveInOut(Function* func, LivenessAnalysis& analysis) {
    bool changed = true;

    // 初始化
    for (const auto& bb : func->bbs) {
        const std::string& bb_name = bb->name;
        analysis.live_in[bb_name].clear();
        analysis.live_out[bb_name].clear();
    }

    // 迭代直到收敛
    while (changed) {
        changed = false;

        // 逆向遍历基本块（为了更快收敛）
        for (auto it = func->bbs.rbegin(); it != func->bbs.rend(); ++it) {
            const std::string& bb_name = (*it)->name;

            // 保存旧值
            auto old_live_in = analysis.live_in[bb_name];
            auto old_live_out = analysis.live_out[bb_name];

            // 计算live_out[B] = ∪(live_in[S]) for all successors S of B
            analysis.live_out[bb_name].clear();
            for (const std::string& succ : cfg_successors[bb_name]) {
                for (const std::string& var : analysis.live_in[succ]) {
                    analysis.live_out[bb_name].insert(var);
                }
            }

            // 计算live_in[B] = use[B] ∪ (live_out[B] - def[B])
            analysis.live_in[bb_name] = analysis.use[bb_name];
            for (const std::string& var : analysis.live_out[bb_name]) {
                if (analysis.def[bb_name].find(var) == analysis.def[bb_name].end()) {
                    analysis.live_in[bb_name].insert(var);
                }
            }

            // 检查是否有变化
            if (old_live_in != analysis.live_in[bb_name] ||
                old_live_out != analysis.live_out[bb_name]) {
                changed = true;
            }
        }
    }

#ifdef DEBUG
    std::cout << "=== LIVE_IN/LIVE_OUT 集合 ===" << std::endl;
    for (const auto& bb : func->bbs) {
        const std::string& bb_name = bb->name;
        std::cout << "基本块 " << bb_name << ":" << std::endl;

        std::cout << "  live_in: { ";
        for (const std::string& var : analysis.live_in[bb_name]) {
            std::cout << var << " ";
        }
        std::cout << "}" << std::endl;

        std::cout << "  live_out: { ";
        for (const std::string& var : analysis.live_out[bb_name]) {
            std::cout << var << " ";
        }
        std::cout << "}" << std::endl;
    }
#endif
}

// 计算每条指令的活跃变量
void RegisterAllocator::computeInstructionLiveness(Function* func, LivenessAnalysis& analysis) {
    analysis.live_at_instruction.clear();

    int instruction_num = 0;

    // 遍历每个基本块
    for (const auto& bb : func->bbs) {
        const std::string& bb_name = bb->name;

        // 从live_out开始，逆向计算每条指令的活跃变量
        std::unordered_set<std::string> current_live = analysis.live_out[bb_name];

        // 逆序处理指令
        std::vector<int> inst_numbers;
        for (size_t i = 0; i < bb->insts.size(); i++) {
            inst_numbers.push_back(instruction_num + i);
        }

        for (int i = bb->insts.size() - 1; i >= 0; i--) {
            int global_inst_num = instruction_num + i;
            const auto& inst = bb->insts[i];

            // 指令执行后的活跃变量
            analysis.live_at_instruction[global_inst_num] = current_live;

            // 更新活跃变量：先移除定义的变量，再添加使用的变量
            auto defined_vars = getDefinedVars(inst.get());
            auto used_vars = getUsedVars(inst.get());

            // 移除定义的变量
            for (const auto& var : defined_vars) {
                current_live.erase(var);
            }

            // 添加使用的变量
            for (const auto& var : used_vars) {
                current_live.insert(var);
            }
        }

        instruction_num += bb->insts.size();
    }

#ifdef DEBUG
    std::cout << "=== 每条指令的活跃变量 ===" << std::endl;
    instruction_num = 0;
    for (const auto& bb : func->bbs) {
        std::cout << "基本块 " << bb->name << ":" << std::endl;
        for (size_t i = 0; i < bb->insts.size(); i++) {
            std::cout << "  指令 " << instruction_num << ": " << bb->insts[i]->toString() << std::endl;
            std::cout << "    活跃变量: { ";
            for (const auto& var : analysis.live_at_instruction[instruction_num]) {
                std::cout << var << " ";
            }
            std::cout << "}" << std::endl;
            instruction_num++;
        }
    }
#endif
}

// 计算活跃区间
void RegisterAllocator::computeLiveIntervals(Function* func, LivenessAnalysis& analysis) {
    std::unordered_map<std::string, int> var_first_def;    // 第一次定义位置
    std::unordered_map<std::string, int> var_last_live;    // 最晚活跃位置

    int instruction_num = 0;

    // 第一步：找到每个变量的第一次定义位置
    for (const auto& bb : func->bbs) {
        for (const auto& inst : bb->insts) {
            auto defined_vars = getDefinedVars(inst.get());
            for (const std::string& var : defined_vars) {
                // 只考虑函数内定义的变量（不包括参数）
                if (isVariableDefinedInFunction(var, func) &&
                    var_first_def.find(var) == var_first_def.end()) {
                    var_first_def[var] = instruction_num;
                }
            }
            instruction_num++;
        }
    }

    // 第二步：找到每个变量的最晚活跃位置
    // 遍历每条指令的活跃变量信息
    for (const auto& pair : analysis.live_at_instruction) {
        int inst_num = pair.first;
        const auto& live_vars = pair.second;

        for (const std::string& var : live_vars) {
            // 只考虑函数内定义的变量
            if (isVariableDefinedInFunction(var, func)) {
                if (var_last_live.find(var) == var_last_live.end()) {
                    var_last_live[var] = inst_num;
                } else {
                    var_last_live[var] = std::max(var_last_live[var], inst_num);
                }
            }
        }
    }

    // 第三步：创建活跃区间
    analysis.live_intervals.clear();
    for (const auto& pair : var_first_def) {
        const std::string& var = pair.first;
        int start = pair.second;

        // 如果变量从来没有活跃过，区间就是定义位置
        int end = var_last_live.find(var) != var_last_live.end() ?
                  var_last_live[var] : start;

        analysis.live_intervals.emplace_back(var, start, end);
    }

    // 按开始位置排序
    std::sort(analysis.live_intervals.begin(), analysis.live_intervals.end());

#ifdef DEBUG
    std::cout << "=== 活跃区间 ===" << std::endl;
    for (const auto& interval : analysis.live_intervals) {
        std::cout << "变量 " << interval.var_name
                  << ": [" << interval.start << ", " << interval.end << "]" << std::endl;
    }
#endif
}

// 执行活跃变量分析
LivenessAnalysis RegisterAllocator::performLivenessAnalysis(Function* func) {
    LivenessAnalysis analysis;

#ifdef DEBUG
    std::cout << "\n=== 开始活跃变量分析，函数: " << func->name << " ===" << std::endl;
#endif

    // 1. 构建控制流图
    buildControlFlowGraph(func);

    // 2. 为指令编号
    numberInstructions(func);

    // 3. 计算def和use集合
    computeDefUse(func, analysis);

    // 4. 计算live_in和live_out
    computeLiveInOut(func, analysis);

    // 5. 计算每条指令的活跃变量
    computeInstructionLiveness(func, analysis);

    // 6. 计算活跃区间
    computeLiveIntervals(func, analysis);

    return analysis;
}

// 释放过期的区间
void RegisterAllocator::expireOldIntervals(int current_start,
                                          std::vector<LiveInterval*>& active,
                                          std::set<std::string>& free_regs) {
    auto it = active.begin();
    while (it != active.end()) {
        if ((*it)->end < current_start) {
            // 区间已经结束，释放寄存器
            free_regs.insert((*it)->assigned_reg);
            it = active.erase(it);
        } else {
            ++it;
        }
    }
}

// 溢出处理
void RegisterAllocator::spillAtInterval(LiveInterval& current,
                                       std::vector<LiveInterval*>& active,
                                       RegisterAllocation& allocation) {
    // 找到结束最晚的活跃区间
    auto spill_candidate = std::max_element(active.begin(), active.end(),
                                           [](const LiveInterval* a, const LiveInterval* b) {
                                               return a->end < b->end;
                                           });

    if (spill_candidate != active.end() && (*spill_candidate)->end > current.end) {
        // 溢出候选区间，将其寄存器分配给当前区间
        current.assigned_reg = (*spill_candidate)->assigned_reg;
        allocation.var_to_reg[current.var_name] = current.assigned_reg;
        allocation.var_to_reg.erase((*spill_candidate)->var_name);

        // 为被溢出的变量分配栈位置
        (*spill_candidate)->spill_location = allocation.max_spill_slots++;
        allocation.var_to_spill_location[(*spill_candidate)->var_name] = (*spill_candidate)->spill_location;
        allocation.spilled_vars.push_back((*spill_candidate)->var_name);

        // 更新活跃列表
        **spill_candidate = current;
        std::sort(active.begin(), active.end(),
                 [](const LiveInterval* a, const LiveInterval* b) {
                     return a->end < b->end;
                 });
    } else {
        // 溢出当前区间
        current.spill_location = allocation.max_spill_slots++;
        allocation.var_to_spill_location[current.var_name] = current.spill_location;
        allocation.spilled_vars.push_back(current.var_name);
    }
}

// 线性扫描算法核心
void RegisterAllocator::linearScanAlgorithm(LivenessAnalysis& liveness, RegisterAllocation& allocation) {
    std::vector<LiveInterval*> active;  // 当前活跃的区间
    std::set<std::string> free_regs(available_registers.begin(), available_registers.end());

    allocation.max_spill_slots = 0;

    for (auto& interval : liveness.live_intervals) {
        // 释放已经结束的区间
        expireOldIntervals(interval.start, active, free_regs);

        // 如果有空闲寄存器，分配给当前区间
        if (!free_regs.empty()) {
            std::string reg = *free_regs.begin();
            free_regs.erase(free_regs.begin());
            interval.assigned_reg = reg;
            allocation.var_to_reg[interval.var_name] = reg;
            active.push_back(&interval);

            // 按结束位置排序
            std::sort(active.begin(), active.end(),
                     [](const LiveInterval* a, const LiveInterval* b) {
                         return a->end < b->end;
                     });
        } else {
            // 需要溢出
            spillAtInterval(interval, active, allocation);
        }
    }
}

// 执行线性扫描寄存器分配
RegisterAllocation RegisterAllocator::performLinearScanAllocation(const LivenessAnalysis& liveness) {
    RegisterAllocation allocation;

#ifdef DEBUG
    std::cout << "\n=== 开始线性扫描寄存器分配 ===" << std::endl;
#endif

    // 复制活跃区间（因为需要修改）
    LivenessAnalysis mutable_liveness = liveness;

    // 执行线性扫描算法
    linearScanAlgorithm(mutable_liveness, allocation);

#ifdef DEBUG
    // 输出分配结果
    std::cout << "\n=== 寄存器分配结果 ===" << std::endl;
    std::cout << "寄存器分配:" << std::endl;
    for (const auto& pair : allocation.var_to_reg) {
        std::cout << "  " << pair.first << " -> " << pair.second << std::endl;
    }

    std::cout << "\n溢出变量 (" << allocation.spilled_vars.size() << " 个):" << std::endl;
    for (const auto& var : allocation.spilled_vars) {
        int location = allocation.var_to_spill_location[var];
        std::cout << "  " << var << " -> 栈位置 " << location << " (偏移: " << (location * 4) << ")" << std::endl;
    }

    std::cout << "\n总共需要 " << allocation.max_spill_slots << " 个栈槽位 ("
              << (allocation.max_spill_slots * 4) << " 字节)" << std::endl;
#endif

    return allocation;
}

// 完整的寄存器分配流程
RegisterAllocation RegisterAllocator::allocateRegisters(Function* func) {
    // 1. 进行活跃变量分析
    LivenessAnalysis liveness = performLivenessAnalysis(func);

    // 2. 执行线性扫描寄存器分配
    RegisterAllocation allocation = performLinearScanAllocation(liveness);

    return allocation;
}