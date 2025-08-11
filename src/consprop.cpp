#include "consprop.h"
#include <memory>
#include <queue>
using namespace std;

struct ConstVal
{
    bool is_const;
    int val;
    bool operator==(const ConstVal &o) const
    {
        return is_const == o.is_const && (!is_const || val == o.val);
    }
    bool operator!=(const ConstVal &o) const
    {
        return !(*this == o);
    }
};

void ConstantPropagationOptimizer::optimize(Program *program)
{
    for (auto &func : program->funcs)
    {
        optimize_function(func.get());
    }
}

void ConstantPropagationOptimizer::optimize_function(Function *func)
{
    // === 1. 建立基本块索引 ===
    unordered_map<string, int> bb_index;
    for (int i = 0; i < (int)func->bbs.size(); ++i)
        bb_index[func->bbs[i]->get_name()] = i;
    int n = func->bbs.size();

    // === 2. 构造 CFG ===
    vector<vector<int>> succs(n), preds(n);
    for (int i = 0; i < n; ++i)
    {
        auto &bb = func->bbs[i];
        if (bb->insts.empty())
            continue;
        auto *last = bb->insts.back().get();
        if (last->v_tag == IRValueTag::BRANCH)
        {
            auto *br = static_cast<BranchValue *>(last);
            int t = bb_index[br->true_block.substr(1)];
            int f = bb_index[br->false_block.substr(1)];
            succs[i].push_back(t);
            succs[i].push_back(f);
            preds[t].push_back(i);
            preds[f].push_back(i);
        }
        else if (last->v_tag == IRValueTag::JUMP)
        {
            auto *j = static_cast<JumpValue *>(last);
            int t = bb_index[j->target_block.substr(1)];
            succs[i].push_back(t);
            preds[t].push_back(i);
        }
        else
        {
            if (i + 1 < n)
            {
                succs[i].push_back(i + 1);
                preds[i + 1].push_back(i);
            }
        }
    }

    // === 3. meet 规则 ===
    auto meet_val = [](ConstVal a, ConstVal b)
    {
        if (!a.is_const || !b.is_const)
            return ConstVal{false, 0};
        if (a.val == b.val)
            return ConstVal{true, a.val};
        return ConstVal{false, 0};
    };
    vector<unordered_map<string, ConstVal>> IN(n), OUT(n);

    // === 4. worklist 初始化 ===
    queue<int> q;
    vector<bool> in_queue(n, false);
    for (int i = 0; i < n; ++i)
    { // 改进：将所有块都加入，以处理多个入口或循环
        q.push(i);
        in_queue[i] = true;
    }

    // === 5. 获取常量工具 ===
    auto get_const = [&](const unordered_map<string, ConstVal> &tbl, IRValue *v, int &out)
    {
        if (v->v_tag == IRValueTag::INTEGER)
        {
            out = static_cast<IntergerValue *>(v)->value;
            return true;
        }
        auto it = tbl.find(v->name);
        if (it != tbl.end() && it->second.is_const)
        {
            out = it->second.val;
            return true;
        }
        return false;
    };

    // === 6. transfer ===
    auto transfer = [&](const unordered_map<string, ConstVal> &in_tbl, int bid)
    {
        unordered_map<string, ConstVal> tbl = in_tbl;
        auto &bb = func->bbs[bid];
        for (auto &inst : bb->insts)
        {
            // 目标寄存器/变量名
            string dest_name;
            if (inst->v_tag == IRValueTag::BINARY)
                dest_name = static_cast<BinaryValue *>(inst.get())->name;
            else if (inst->v_tag == IRValueTag::LOAD)
                dest_name = static_cast<LoadValue *>(inst.get())->name;

            // 默认所有赋值指令都会让目标变量变为不确定
            if (!dest_name.empty())
            {
                tbl.erase(dest_name);
            }

            switch (inst->v_tag)
            {
            case IRValueTag::BINARY:
            {
                auto *bin = static_cast<BinaryValue *>(inst.get());
                int lv, rv;
                if (get_const(tbl, bin->lhs.get(), lv) &&
                    get_const(tbl, bin->rhs.get(), rv))
                {
                    int res;
                    if (fold_binary(bin, res))
                    {
                        tbl[bin->name] = {true, res};
                    }
                }
                break;
            }
            case IRValueTag::STORE:
            {
                auto *st = static_cast<StoreValue *>(inst.get());
                int v;
                // 对于 store 指令，我们更新的是它目标地址的状态
                if (get_const(tbl, st->value.get(), v))
                    tbl[st->dest->name] = {true, v}; // 支持全局/局部
                else
                    tbl.erase(st->dest->name); // 如果存入一个不确定的值，那这个地址的值也就不确定了
                break;
            }
            case IRValueTag::LOAD:
            {
                auto *ld = static_cast<LoadValue *>(inst.get());
                int v;
                // load 的结果由其源地址的状态决定
                if (get_const(tbl, ld->src.get(), v))
                    tbl[ld->name] = {true, v};
                else
                    tbl.erase(ld->name);
                break;
            }
            default:
                break;
            }
        }
        return tbl;
    };

    // === 7. 数据流迭代 ===
    while (!q.empty())
    {
        int b = q.front();
        q.pop();
        in_queue[b] = false;

        // 计算 IN[b]
        unordered_map<string, ConstVal> new_in;
        if (!preds[b].empty())
        {
            new_in = OUT[preds[b][0]];
            for (size_t i = 1; i < preds[b].size(); ++i)
            {
                int p = preds[b][i];
                // 对 new_in 和 OUT[p] 做 meet 操作
                for (auto const &[key, val] : new_in)
                {
                    auto it = OUT[p].find(key);
                    if (it != OUT[p].end())
                    {
                        new_in[key] = meet_val(val, it->second);
                    }
                    else
                    {
                        // 如果前驱的 OUT 集不包含某个变量，则该变量在此路径上为 NAC
                        new_in[key] = {false, 0};
                    }
                }
            }
        }
        IN[b] = new_in;

        auto new_out = transfer(IN[b], b);
        if (new_out != OUT[b])
        {
            OUT[b] = new_out;
            for (int s : succs[b])
                if (!in_queue[s])
                {
                    q.push(s);
                    in_queue[s] = true;
                }
        }
    }

    // === 8. 替换 IR + 恒真/恒假剪枝 ===
    vector<bool> alive(n, true);
    for (int b = 0; b < n; ++b)
    {
        if (!alive[b])
            continue;
        auto env = IN[b]; // 使用数据流分析结果初始化块内常量环境
        vector<unique_ptr<IRValue>> new_insts;

        for (auto &inst : func->bbs[b]->insts)
        {
            switch (inst->v_tag)
            {
            case IRValueTag::BINARY:
            {
                auto *bin = static_cast<BinaryValue *>(inst.get());
                int l, r;
                // 替换操作数为常量
                if (get_const(env, bin->lhs.get(), l))
                    bin->lhs = make_unique<IntergerValue>(l);
                if (get_const(env, bin->rhs.get(), r))
                    bin->rhs = make_unique<IntergerValue>(r);

                int res;
                // 尝试常量折叠
                if (fold_binary(bin, res))
                {
                    // 折叠成功，此指令的结果是常量。
                    env[bin->name] = {true, res};
                    // 指令本身被优化掉，不加入 new_insts。
                    continue;
                }

                // 不能折叠，此指令结果不是常量
                env.erase(bin->name);
                new_insts.push_back(move(inst));
                break;
            }
            case IRValueTag::STORE:
            {
                auto *st = static_cast<StoreValue *>(inst.get());
                int v;
                // 尝试将要存储的值替换为常量
                if (get_const(env, st->value.get(), v))
                    st->value = make_unique<IntergerValue>(v);

                // 更新内存位置的常量状态
                if (st->value->v_tag == IRValueTag::INTEGER)
                    env[st->dest->name] = {true, static_cast<IntergerValue *>(st->value.get())->value};
                else
                    env.erase(st->dest->name);

                // **[核心修正]** STORE 指令有副作用，必须保留！
                new_insts.push_back(move(inst));
                break;
            }
            case IRValueTag::LOAD:
            {
                auto *ld = static_cast<LoadValue *>(inst.get());
                int v;
                // 尝试从环境中直接加载常量
                if (get_const(env, ld->src.get(), v))
                {
                    // 加载来源是常量，那么加载结果也是常量。
                    env[ld->name] = {true, v};
                    // LOAD指令本身被优化掉，因为它的结果会被直接传播。
                    continue;
                }

                // 加载来源不是常量，此指令结果也不是常量
                env.erase(ld->name);
                new_insts.push_back(move(inst));
                break;
            }
            case IRValueTag::BRANCH:
            {
                auto *br = static_cast<BranchValue *>(inst.get());
                int cond;
                if (get_const(env, br->cond.get(), cond))
                {
                    // 分支条件是常量，将 Branch 替换为 Jump
                    if (cond != 0) // true
                    {
                        new_insts.push_back(make_unique<JumpValue>(br->true_block));
                        alive[bb_index[br->false_block.substr(1)]] = false;
                    }
                    else // false
                    {
                        new_insts.push_back(make_unique<JumpValue>(br->false_block));
                        alive[bb_index[br->true_block.substr(1)]] = false;
                    }
                }
                else
                {
                    // 分支条件不是常量，保留原指令
                    new_insts.push_back(move(inst));
                }
                // branch是块的最后一条指令，处理完即可退出循环
                break;
            }
            case IRValueTag::RETURN:
            {
                auto *ret = static_cast<ReturnValue *>(inst.get());
                // 只有带返回值的 return 才需要优化
                if (ret->value)
                {
                    int v;
                    if (get_const(env, ret->value.get(), v))
                    {
                        // 将返回值替换为常量
                        ret->value = make_unique<IntergerValue>(v);
                    }
                }
                new_insts.push_back(move(inst));
                break;
            }
            default:
                // 其他指令（如 alloc, call, jump 等）直接保留
                new_insts.push_back(move(inst));
                break;
            }
        }
        func->bbs[b]->insts = move(new_insts);
    }

    // === 9. 删除不可达块 ===
    vector<unique_ptr<BasicBlock>> new_bbs;
    for (int i = 0; i < n; ++i)
        if (alive[i])
            new_bbs.push_back(move(func->bbs[i]));
    func->bbs = move(new_bbs);
}

// 保留 fold_binary 和 get_constant（它们是辅助函数）
bool ConstantPropagationOptimizer::fold_binary(BinaryValue *bin, int &result)
{
    int lhs_const, rhs_const;
    if (!get_constant(bin->lhs.get(), lhs_const) ||
        !get_constant(bin->rhs.get(), rhs_const))
        return false;

    switch (bin->op)
    {
    case BinaryOp::ADD:
        result = lhs_const + rhs_const;
        return true;
    case BinaryOp::SUB:
        result = lhs_const - rhs_const;
        return true;
    case BinaryOp::MUL:
        result = lhs_const * rhs_const;
        return true;
    case BinaryOp::DIV:
        if (rhs_const != 0)
        {
            result = lhs_const / rhs_const;
            return true;
        }
        break;
    case BinaryOp::MOD:
        if (rhs_const != 0)
        {
            result = lhs_const % rhs_const;
            return true;
        }
        break;
    case BinaryOp::EQ:
        result = (lhs_const == rhs_const);
        return true;
    case BinaryOp::NE:
        result = (lhs_const != rhs_const);
        return true;
    case BinaryOp::LT:
        result = (lhs_const < rhs_const);
        return true;
    case BinaryOp::LE:
        result = (lhs_const <= rhs_const);
        return true;
    case BinaryOp::GT:
        result = (lhs_const > rhs_const);
        return true;
    case BinaryOp::GE:
        result = (lhs_const >= rhs_const);
        return true;
    default:
        break;
    }
    return false;
}

bool ConstantPropagationOptimizer::get_constant(IRValue *val, int &out_const)
{
    if (val->v_tag == IRValueTag::INTEGER)
    {
        out_const = static_cast<IntergerValue *>(val)->value;
        return true;
    }
    if (!val->name.empty())
    {
        auto it = const_table.find(val->name);
        if (it != const_table.end())
        {
            out_const = it->second;
            return true;
        }
    }
    if (val->v_tag == IRValueTag::VAR_REF)
    {
        auto it = const_table.find(val->name);
        if (it != const_table.end())
        {
            out_const = it->second;
            return true;
        }
    }
    return false;
}
