#ifndef IR_H
#define IR_H

#include <algorithm>
#include <cstddef>
#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <vector>

/** Types in IR */

enum class IRTypeTag { INT32, FUNCTON, UNIT };

class IRType {
public:
  IRTypeTag t_tag;

  IRType(IRTypeTag type) : t_tag(type) {}
  virtual ~IRType() = default;

  IRType(const IRType &) = delete;
  IRType &operator=(const IRType &) = delete;
  IRType(IRType &&) = default;
  IRType &operator=(IRType &&) = default;

  virtual std::string toString() const = 0;
  virtual int equals(const IRType &other) const = 0;

  bool operator==(const IRType &other) const { return equals(other); }

  bool operator!=(const IRType &other) const { return !equals(other); }

  int isInt32() const { return t_tag == IRTypeTag::INT32; }
  int isUnit() const { return t_tag == IRTypeTag::UNIT; }
  int isFunction() const { return t_tag == IRTypeTag::FUNCTON; }
};

class Int32Type : public IRType {
public:
  Int32Type() : IRType(IRTypeTag::INT32) {}

  std::string toString() const override;
  int equals(const IRType &other) const override {
    return other.t_tag == IRTypeTag::INT32;
  }
};

class FunctionType : public IRType {
public:
  std::vector<std::unique_ptr<IRType>> param_types;
  std::unique_ptr<IRType> return_type;

  FunctionType(std::vector<std::unique_ptr<IRType>> param_t,
               std::unique_ptr<IRType> ret_t)
      : IRType(IRTypeTag::FUNCTON), param_types(std::move(param_t)),
        return_type(std::move(ret_t)) {}

  std::string toString() const override;
  int equals(const IRType &other) const override {
    if (other.t_tag != IRTypeTag::FUNCTON)
      return 0;

    const FunctionType &other_func_t = static_cast<const FunctionType &>(other);

    if (!(return_type->equals(*other_func_t.return_type)))
      return 0;

    if (param_types.size() != other_func_t.param_types.size())
      return 0;

    for (size_t i = 0; i < param_types.size(); ++i) {
      if (!param_types[i]->equals(*other_func_t.param_types[i]))
        return 0;
    }

    return 1;
  };
};

class UnitType : public IRType {
public:
  UnitType() : IRType(IRTypeTag::UNIT) {}

  std::string toString() const override;
  int equals(const IRType &other) const override {
    return other.t_tag == IRTypeTag::UNIT;
  }
};

/** values in IR */
enum class IRValueTag {
  INTEGER,
  FUNC_ARG_REF,
  VAR_REF,
  ALLOC,
  LOAD,
  STORE,
  BINARY,
  BRANCH,
  JUMP,
  CALL,
  RETURN
};

class IRValue {
public:
  IRValueTag v_tag;
  std::unique_ptr<IRType> type;
  std::string name;

  IRValue(IRValueTag t, std::unique_ptr<IRType> ty, const std::string &n = "")
      : v_tag(t), type(std::move(ty)), name(n) {}

  virtual ~IRValue() = default;
  virtual std::string toString() const = 0;
};

// e.g. 255
class IntergerValue : public IRValue {
public:
  int value;

  IntergerValue(int val)
      : IRValue(IRValueTag::INTEGER, std::make_unique<Int32Type>()),
        value(val) {}

  std::string toString() const override { return std::to_string(value); }
};

// Function argument reference.
// e.g. int f(int a, int b), a and b would be func_arg_ref.
class FuncArgRefValue : public IRValue {
public:
  size_t index;

  FuncArgRefValue(size_t i, const std::string &n)
      : IRValue(IRValueTag::FUNC_ARG_REF, std::make_unique<Int32Type>(), n),
        index(i) {}

  std::string toString() const override {
    return name + ": " + type->toString();
  }
};

class VarRefValue : public IRValue {
public:
  VarRefValue(const std::string &var_name)
      : IRValue(IRValueTag::VAR_REF, std::make_unique<Int32Type>(), var_name) {}

  std::string toString() const override { return name; }
};

// Binary op
enum class BinaryOp {
  NE = 0,
  EQ = 1,
  GT = 2,
  LT = 3,
  GE = 4,
  LE = 5,
  ADD = 6,
  SUB = 7,
  MUL = 8,
  DIV = 9,
  MOD = 10,
  AND = 11,
  OR = 12,
  XOR = 13,
  SHL = 14,
  SHR = 15,
  SAR = 16
};

class BinaryValue : public IRValue {
public:
  BinaryOp op;
  std::unique_ptr<IRValue> lhs;
  std::unique_ptr<IRValue> rhs;

  BinaryValue(const std::string &result_name, BinaryOp bop,
              std::unique_ptr<IRValue> left, std::unique_ptr<IRValue> right)
      : IRValue(IRValueTag::BINARY, std::make_unique<Int32Type>(), result_name),
        op(bop), lhs(std::move(left)), rhs(std::move(right)) {}

  std::string toString() const override {
    return "  " + name + " = " + bopToString(op) + " " + lhs->toString() +
           ", " + rhs->toString();
  }

private:
  std::string bopToString(BinaryOp op) const {
    switch (op) {
    case BinaryOp::ADD:
      return "add";
    case BinaryOp::SUB:
      return "sub";
    case BinaryOp::MUL:
      return "mul";
    case BinaryOp::DIV:
      return "div";
    case BinaryOp::MOD:
      return "mod";
    case BinaryOp::EQ:
      return "eq";
    case BinaryOp::NE:
      return "ne";
    case BinaryOp::LT:
      return "lt";
    case BinaryOp::LE:
      return "le";
    case BinaryOp::GT:
      return "gt";
    case BinaryOp::GE:
      return "ge";
    case BinaryOp::AND:
      return "and";
    case BinaryOp::OR:
      return "or";
    default:
      return "unknown";
    }
  }
};

class AllocValue : public IRValue {
public:
  AllocValue(const std::string &var_name)
      : IRValue(IRValueTag::ALLOC, std::make_unique<Int32Type>(), var_name) {}

  std::string toString() const override { return "  " + name + " = alloc i32"; }
};

class LoadValue : public IRValue {
public:
  std::unique_ptr<IRValue> src;

  LoadValue(const std::string &result_name, std::unique_ptr<IRValue> source)
      : IRValue(IRValueTag::LOAD, std::make_unique<Int32Type>(), result_name),
        src(std::move(source)) {}

  std::string toString() const override {
    return "  " + name + " = load " + src->toString();
  }
};

class StoreValue : public IRValue {
public:
  std::unique_ptr<IRValue> value;
  std::unique_ptr<IRValue> dest;

  StoreValue(std::unique_ptr<IRValue> val, std::unique_ptr<IRValue> de)
      : IRValue(IRValueTag::STORE, std::make_unique<UnitType>()),
        value(std::move(val)), dest(std::move(de)) {}

  std::string toString() const override {
    return "  store" + value->toString() + ", " + dest->toString();
  }
};

class CallValue : public IRValue {
public:
  std::string callee; // function name
  std::vector<std::unique_ptr<IRValue>> args;

  CallValue(const std::string &result_name, const std::string &func_name,
            std::vector<std::unique_ptr<IRValue>> &arguments,
            std::unique_ptr<IRType> ret_type)
      : IRValue(IRValueTag::CALL, std::move(ret_type), result_name),
        callee(func_name), args(std::move(arguments)) {}

  std::string toString() const override {
    std::string res = "  ";
    if (!name.empty()) {
      res += name + " = ";
    }
    res += "call  " + callee + " (";
    for (size_t i = 0; i < args.size(); ++i) {
      if (i > 0)
        res += " ,";
      res += args[i]->toString();
    }
    res += ")";
    return res;
  }
};

class ReturnValue : public IRValue {
public:
  std::unique_ptr<IRValue> value;

  ReturnValue(std::unique_ptr<IRValue> val = nullptr)
      : IRValue(IRValueTag::RETURN, std::make_unique<UnitType>()),
        value(std::move(val)) {}

  std::string toString() const override {
    std::string res = "  ret";
    if (value != nullptr) {
      return res + " " + value->toString();
    } else {
      return res;
    }
  }
};

class BranchValue : public IRValue {
public:
  std::unique_ptr<IRValue> cond;
  std::string true_block;
  std::string false_block;

  BranchValue(std::unique_ptr<IRValue> condition, const std::string &true_b,
              const std::string &false_b)
      : IRValue(IRValueTag::BRANCH, std::make_unique<UnitType>()),
        cond(std::move(condition)), true_block(true_b), false_block(false_b) {}

  std::string toString() const override {
    return "  br " + cond->toString() + ", " + true_block + ", " +
           false_block;
  }
};

class JumpValue : public IRValue {
public:
  std::string target_block;

  JumpValue(const std::string &target_b)
      : IRValue(IRValueTag::JUMP, std::make_unique<UnitType>()),
        target_block(target_b) {}

  std::string toString() const override { return "  jump " + target_block; }
};

/** program components in IR */
class BasicBlock {
public:
  std::string name;
  std::vector<std::unique_ptr<IRValue>> insts;

  BasicBlock(const std::string &block_name) : name(block_name) {}

  void add_inst(std::unique_ptr<IRValue> inst) {
    insts.push_back(std::move(inst));
  }

  std::string toString() const {
    std::string res = name + ":\n";
    for (const auto &inst : insts) {
      res += inst->toString() + "\n";
    }
    return res;
  }
};

class Function {
public:
  std::string name;
  std::unique_ptr<FunctionType> f_type;
  std::vector<std::unique_ptr<IRValue>> params;
  std::vector<std::unique_ptr<BasicBlock>> bbs; // basic blocks

  Function(const std::string &func_name,
           std::unique_ptr<FunctionType> func_type)
      : name(func_name), f_type(std::move(func_type)) {}

  void add_param(std::unique_ptr<FuncArgRefValue> param) {
    params.push_back(std::move(param));
  }

  void add_basic_block(std::unique_ptr<BasicBlock> bb) {
    bbs.push_back(std::move(bb));
  }

  std::string toString() const {
    std::string res = "fun " + name + "(";
    for (size_t i = 0; i < params.size(); ++i) {
      if (i > 0)
        res += ", ";
      res += params[i]->toString();
    }
    res += "): ";
    res += f_type->return_type->toString();
    res += "{\n";

    for (const auto &bb : bbs) {
      res += bb->toString();
    }
    res += "}\n";

    return res;
  }
};

class Program {
public:
  std::vector<std::unique_ptr<Function>> funcs;

  void add_function(std::unique_ptr<Function> func) {
    funcs.push_back(std::move(func));
  }

  std::string toString() const {
    std::string result;
    for (const auto &func : funcs) {
      result += func->toString() + "\n";
    }
    return result;
  }
};

#endif // !IR_H
