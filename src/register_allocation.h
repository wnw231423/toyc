#ifndef REGISTER_ALLOCATION_H
#define REGISTER_ALLOCATION_H

#define DEBUG

#include "IR.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <set>

// 活跃区间结构
struct LiveInterval {
    std::string var_name;           // 变量名
    int start;                      // 开始位置（第一次定义的指令序号）
    int end;                        // 结束位置（最晚活跃的指令序号）
    std::string assigned_reg;       // 分配的寄存器，空表示溢出到内存
    int spill_location;             // 如果溢出，在栈中的位置

    LiveInterval(const std::string& name, int s, int e)
        : var_name(name), start(s), end(e), spill_location(-1) {}

    // 判断两个区间是否重叠
    bool overlaps(const LiveInterval& other) const {
        return !(end < other.start || other.end < start);
    }

    // 比较函数，用于排序（按开始位置排序）
    bool operator<(const LiveInterval& other) const {
        return start < other.start;
    }
};

// 活跃变量分析结果
struct LivenessAnalysis {
    // 每个基本块的活跃变量信息
    std::unordered_map<std::string, std::unordered_set<std::string>> live_in;   // 块入口活跃变量
    std::unordered_map<std::string, std::unordered_set<std::string>> live_out;  // 块出口活跃变量
    std::unordered_map<std::string, std::unordered_set<std::string>> def;       // 块内定义的变量
    std::unordered_map<std::string, std::unordered_set<std::string>> use;       // 块内使用的变量

    // 每个指令位置的活跃变量
    std::unordered_map<int, std::unordered_set<std::string>> live_at_instruction;

    // 变量的活跃区间
    std::vector<LiveInterval> live_intervals;
};

// 寄存器分配结果
struct RegisterAllocation {
    std::unordered_map<std::string, std::string> var_to_reg;        // 变量到寄存器的映射
    std::unordered_map<std::string, int> var_to_spill_location;     // 溢出变量到栈位置的映射
    std::vector<std::string> spilled_vars;                          // 溢出的变量列表
    int max_spill_slots;                                            // 需要的栈槽数量
};

// 寄存器分配器类
class RegisterAllocator {
private:
    // 可用的寄存器（RISC-V临时寄存器）
    std::vector<std::string> available_registers = {
        "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11"  // 保存寄存器
    };

    // 控制流图
    std::unordered_map<std::string, std::vector<std::string>> cfg_successors;
    std::unordered_map<std::string, std::vector<std::string>> cfg_predecessors;

    // 指令到序号的映射
    std::unordered_map<std::string, int> instruction_numbers;
    int total_instructions;

public:
    // 主要接口函数
    LivenessAnalysis performLivenessAnalysis(Function* func);
    RegisterAllocation performLinearScanAllocation(const LivenessAnalysis& liveness);

    // 完整的寄存器分配流程
    RegisterAllocation allocateRegisters(Function* func);

    // 辅助函数
    void buildControlFlowGraph(Function* func);
    void computeDefUse(Function* func, LivenessAnalysis& analysis);
    void computeLiveInOut(Function* func, LivenessAnalysis& analysis);
    void computeInstructionLiveness(Function* func, LivenessAnalysis& analysis);
    void computeLiveIntervals(Function* func, LivenessAnalysis& analysis);
    void numberInstructions(Function* func);

    // 从指令中提取定义和使用的变量
    std::vector<std::string> getDefinedVars(const IRValue* inst);
    std::vector<std::string> getUsedVars(const IRValue* inst);

    // 检查变量是否在函数内定义（不包括参数）
    bool isVariableDefinedInFunction(const std::string& var_name, Function* func);
    
    // 线性扫描算法的核心函数
    void linearScanAlgorithm(LivenessAnalysis& liveness, RegisterAllocation& allocation);
    void expireOldIntervals(int current_start, std::vector<LiveInterval*>& active, 
                           std::set<std::string>& free_regs);
    void spillAtInterval(LiveInterval& current, std::vector<LiveInterval*>& active, 
                        RegisterAllocation& allocation);
};

#endif // REGISTER_ALLOCATION_H