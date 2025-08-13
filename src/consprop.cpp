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
    unordered_map<string, int> bb_index;
    for (int i = 0; i < (int)func->bbs.size(); ++i)
        bb_index[func->bbs[i]->get_name()] = i;
    int n = func->bbs.size();

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

    auto meet_val = [](ConstVal a, ConstVal b)
    {
        if (!a.is_const || !b.is_const)
            return ConstVal{false, 0};
        return (a.val == b.val) ? ConstVal{true, a.val} : ConstVal{false, 0};
    };
    vector<unordered_map<string, ConstVal>> IN(n), OUT(n);

    queue<int> q;
    vector<bool> in_queue(n, false);
    for (int i = 0; i < n; ++i)
    {
        q.push(i);
        in_queue[i] = true;
    }

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

    auto transfer = [&](const unordered_map<string, ConstVal> &in_tbl, int bid)
    {
        auto tbl = in_tbl;
        auto &bb = func->bbs[bid];
        for (auto &inst : bb->insts)
        {
            string dest_name;
            if (inst->v_tag == IRValueTag::BINARY)
                dest_name = static_cast<BinaryValue *>(inst.get())->name;
            else if (inst->v_tag == IRValueTag::LOAD)
                dest_name = static_cast<LoadValue *>(inst.get())->name;

            if (!dest_name.empty())
                tbl.erase(dest_name);

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
                        tbl[bin->name] = {true, res};
                }
                break;
            }
            case IRValueTag::STORE:
            {
                auto *st = static_cast<StoreValue *>(inst.get());
                int v;
                if (get_const(tbl, st->value.get(), v))
                    tbl[st->dest->name] = {true, v};
                else
                    tbl.erase(st->dest->name);
                break;
            }
            case IRValueTag::LOAD:
            {
                auto *ld = static_cast<LoadValue *>(inst.get());
                int v;
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

    while (!q.empty())
    {
        int b = q.front();
        q.pop();
        in_queue[b] = false;
        unordered_map<string, ConstVal> new_in;
        if (!preds[b].empty())
        {
            new_in = OUT[preds[b][0]];
            for (size_t i = 1; i < preds[b].size(); ++i)
            {
                int p = preds[b][i];
                for (auto &[key, val] : new_in)
                {
                    auto it = OUT[p].find(key);
                    if (it != OUT[p].end())
                        val = meet_val(val, it->second);
                    else
                        val = {false, 0};
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

    // 新增通用操作数常量化工具
    auto replace_operands = [&](IRValue *v, unordered_map<string, ConstVal> &env)
    {
        switch (v->v_tag)
        {
        case IRValueTag::BINARY:
        {
            auto *bin = static_cast<BinaryValue *>(v);
            int l, r;
            if (get_const(env, bin->lhs.get(), l))
                bin->lhs = make_unique<IntergerValue>(l);
            if (get_const(env, bin->rhs.get(), r))
                bin->rhs = make_unique<IntergerValue>(r);
            break;
        }
        case IRValueTag::STORE:
        {
            auto *st = static_cast<StoreValue *>(v);
            int val;
            if (get_const(env, st->value.get(), val))
                st->value = make_unique<IntergerValue>(val);
            break;
        }
        case IRValueTag::LOAD:
        {
            auto *ld = static_cast<LoadValue *>(v);
            int src_val;
            if (get_const(env, ld->src.get(), src_val))
                ld->src = make_unique<IntergerValue>(src_val);
            break;
        }
        case IRValueTag::BRANCH:
        {
            auto *br = static_cast<BranchValue *>(v);
            int cond_val;
            if (get_const(env, br->cond.get(), cond_val))
                br->cond = make_unique<IntergerValue>(cond_val);
            break;
        }
        case IRValueTag::CALL:
        {
            auto *cv = static_cast<CallValue *>(v);
            for (auto &arg : cv->args)
            {
                int arg_val;
                if (get_const(env, arg.get(), arg_val))
                    arg = make_unique<IntergerValue>(arg_val);
            }
            break;
        }
        case IRValueTag::RETURN:
        {
            auto *ret = static_cast<ReturnValue *>(v);
            if (ret->value)
            {
                int rv;
                if (get_const(env, ret->value.get(), rv))
                    ret->value = make_unique<IntergerValue>(rv);
            }
            break;
        }
        default:
            break;
        }
    };

    vector<bool> alive(n, true);

    for (int b = 0; b < n; ++b)
    {
        if (!alive[b])
            continue;
        auto env = IN[b];
        vector<unique_ptr<IRValue>> new_insts;

        for (auto &inst : func->bbs[b]->insts)
        {
            // 先常量化所有操作数
            replace_operands(inst.get(), env);

            switch (inst->v_tag)
            {
            case IRValueTag::BINARY:
            {
                auto *bin = static_cast<BinaryValue *>(inst.get());
                int res;
                if (fold_binary(bin, res))
                {
                    env[bin->name] = {true, res};
                    continue;
                }
                env.erase(bin->name);
                new_insts.push_back(move(inst));
                break;
            }
            case IRValueTag::STORE:
            {
                auto *st = static_cast<StoreValue *>(inst.get());
                if (st->value->v_tag == IRValueTag::INTEGER)
                    env[st->dest->name] = {true, static_cast<IntergerValue *>(st->value.get())->value};
                else
                    env.erase(st->dest->name);
                new_insts.push_back(move(inst));
                break;
            }
            case IRValueTag::LOAD:
            {
                auto *ld = static_cast<LoadValue *>(inst.get());
                int v;
                if (get_const(env, ld->src.get(), v))
                {
                    env[ld->name] = {true, v};
                    continue;
                }
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
                    if (cond)
                    {
                        new_insts.push_back(make_unique<JumpValue>(br->true_block));
                        alive[bb_index[br->false_block.substr(1)]] = false;
                    }
                    else
                    {
                        new_insts.push_back(make_unique<JumpValue>(br->false_block));
                        alive[bb_index[br->true_block.substr(1)]] = false;
                    }
                }
                else
                    new_insts.push_back(move(inst));
                break;
            }
            default:
                new_insts.push_back(move(inst));
                break;
            }
        }

        func->bbs[b]->insts = move(new_insts);
    }

    vector<unique_ptr<BasicBlock>> new_bbs;
    for (int i = 0; i < n; ++i)
        if (alive[i])
            new_bbs.push_back(move(func->bbs[i]));
    func->bbs = move(new_bbs);

    // === 新增：清理终结指令之后的死代码 ===
    for (auto &bb : func->bbs)
    {
        auto &insts = bb->insts;
        for (size_t i = 0; i < insts.size(); ++i)
        {
            if (insts[i]->v_tag == IRValueTag::RETURN ||
                insts[i]->v_tag == IRValueTag::JUMP)
            {
                insts.resize(i + 1);
                break;
            }
        }
    }
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

