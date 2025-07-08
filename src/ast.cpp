#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "IR.h"
#include "ast.h"
#include "symtable.h"

static Function *current_func;
static BasicBlock *current_bb;

void dumpIndent(const int level, const std::string &s) {
  for (int i = 0; i < level; ++i) {
    std::cout << "  ";
  }
  std::cout << s << std::endl;
}

void CompUnitAST::Dump(int level) const {
  dumpIndent(level, "CompUnitAST {");
  for (const auto &func_def : *func_defs) {
    func_def->Dump(level + 1);
  }
  dumpIndent(level, "}");
}

void FuncDefAST::Dump(int level) const {
  dumpIndent(level, "FuncDefAST {");
  dumpIndent(level + 1, "ident: " + ident);
  dumpIndent(level + 1, "func_type: " + func_type);
  dumpIndent(level + 1, "params: {");
  for (const auto &param : *fparams) {
    param->Dump(level + 2);
  }
  dumpIndent(level + 1, "}");
  dumpIndent(level + 1, "block: {");
  block->Dump(level + 2);
  dumpIndent(level + 1, "}");
  dumpIndent(level, "}");
}

void FuncFParamAST::Dump(int level) const {
  dumpIndent(level, "FuncFParamAST {");
  dumpIndent(level + 1, "type: " + type);
  dumpIndent(level + 1, "ident: " + ident);
  dumpIndent(level, "}");
}

void BlockAST::Dump(int level) const {
  dumpIndent(level, "BlockAST {");
  for (const auto &stmt : *stmts) {
    stmt->Dump(level + 1);
  }
  dumpIndent(level, "}");
}

void StmtAST::Dump(int level) const {
  dumpIndent(level, "StmtAST {");
  if (type == 6) {
    dumpIndent(level + 1, "nop");
  } else if (type == 9) {
    dumpIndent(level + 1, "break");
  } else if (type == 10) {
    dumpIndent(level + 1, "continue");
  } else {
    stmt->Dump(level + 1);
  }
  dumpIndent(level, "}");
}

void VarDeclStmtAST::Dump(int level) const {
  dumpIndent(level, "VarDeclStmtAST {");
  dumpIndent(level + 1, "type: int");
  var_def->Dump(level + 1);
  dumpIndent(level, "}");
}

void VarDefAST::Dump(int level) const {
  dumpIndent(level, "VarDefAST {");
  dumpIndent(level + 1, "ident: " + ident);
  exp->Dump(level + 1);
  dumpIndent(level, "}");
}

void VarAssignStmtAST::Dump(int level) const {
  dumpIndent(level, "VarAssignStmtAST {");
  lval->Dump(level + 1);
  exp->Dump(level + 1);
  dumpIndent(level, "}");
}

void ReturnStmtAST::Dump(int level) const {
  dumpIndent(level, "ReturnAST {");
  exp->Dump(level + 1);
  dumpIndent(level, "}");
}

void IfStmtAST::Dump(int level) const {
  dumpIndent(level, "IfStmtAST {");
  dumpIndent(level + 1, "if condition: {");
  exp->Dump(level + 2);
  dumpIndent(level + 1, "}");
  dumpIndent(level + 1, "then block: {");
  stmt_then->Dump(level + 2);
  dumpIndent(level + 1, "}");
  if (stmt_else) {
    dumpIndent(level + 1, "else block: {");
    stmt_else->Dump(level + 2);
    dumpIndent(level + 1, "}");
  }
  dumpIndent(level, "}");
}

void WhileStmtAST::Dump(int level) const {
  dumpIndent(level, "WhileStmtAST {");
  dumpIndent(level + 1, "condition: {");
  exp->Dump(level + 2);
  dumpIndent(level + 1, "}");
  dumpIndent(level + 1, "body: {");
  stmt->Dump(level + 2);
  dumpIndent(level + 1, "}");
  dumpIndent(level, "}");
}

void ExpAST::Dump(int level) const {
  dumpIndent(level, "ExpAST {");
  lorExp->Dump(level + 1);
  dumpIndent(level, "}");
}

void LOrExpAST::Dump(int level) const {
  dumpIndent(level, "LOrExpAST {");
  if (type == 1) {
    landExp_lorExp->Dump(level + 1);
  } else if (type == 2) {
    dumpIndent(level + 1, "op: ||");
    landExp_lorExp->Dump(level + 1);
    landExp->Dump(level + 1);
  }
  dumpIndent(level, "}");
}

void LAndExpAST::Dump(int level) const {
  dumpIndent(level, "LAndExpAST {");
  if (type == 1) {
    eqExp_landExp->Dump(level + 1);
  } else if (type == 2) {
    dumpIndent(level + 1, "op: &&");
    eqExp_landExp->Dump(level + 1);
    eqExp->Dump(level + 1);
  }
  dumpIndent(level, "}");
}

void EqExpAST::Dump(int level) const {
  dumpIndent(level, "EqExpAST {");
  if (type == 1) {
    relExp_eqExp->Dump(level + 1);
  } else if (type == 2) {
    dumpIndent(level + 1, "eq_op: " + eq_op);
    relExp_eqExp->Dump(level + 1);
    relExp->Dump(level + 1);
  }
  dumpIndent(level, "}");
}

void RelExpAST::Dump(int level) const {
  dumpIndent(level, "RelExpAST {");
  if (type == 1) {
    addExp_relExp->Dump(level + 1);
  } else if (type == 2) {
    dumpIndent(level + 1, "rel_op: " + rel_op);
    addExp_relExp->Dump(level + 1);
    addExp->Dump(level + 1);
  }
  dumpIndent(level, "}");
}

void AddExpAST::Dump(int level) const {
  dumpIndent(level, "AddExpAST {");
  if (type == 1) {
    mulExp_addExp->Dump(level + 1);
  } else if (type == 2) {
    dumpIndent(level + 1, "add_op: " + add_op);
    mulExp_addExp->Dump(level + 1);
    mulExp->Dump(level + 1);
  }
  dumpIndent(level, "}");
}

void MulExpAST::Dump(int level) const {
  dumpIndent(level, "MulExpAST {");
  if (type == 1) {
    unaryExp_mulExp->Dump(level + 1);
  } else if (type == 2) {
    dumpIndent(level + 1, "mul_op: " + mul_op);
    unaryExp->Dump(level + 1);
    unaryExp_mulExp->Dump(level + 1);
  }
  dumpIndent(level, "}");
}

void UnaryExpAST::Dump(int level) const {
  if (type == 1) {
    dumpIndent(level, "UnaryExpAST {");
    primaryExp_unaryExp_funcCall->Dump(level + 1);
    dumpIndent(level, "}");
  } else if (type == 2) {
    dumpIndent(level, "UnaryExpAST {");
    dumpIndent(level + 1, "unary_op: " + unary_op);
    primaryExp_unaryExp_funcCall->Dump(level + 1);
    dumpIndent(level, "}");
  } else if (type == 3) {
    dumpIndent(level, "UnaryExpAST {");
    dumpIndent(level + 1, "func_call: {");
    primaryExp_unaryExp_funcCall->Dump(level + 2);
    dumpIndent(level + 1, "}");
    dumpIndent(level, "}");
  }
}

void FuncCallAST::Dump(int level) const {
  dumpIndent(level, "FuncCallAST {");
  dumpIndent(level + 1, "ident: " + ident);
  for (const auto &param : *rparams) {
    param->Dump(level + 1);
  }
  dumpIndent(level, "}");
}

void PrimaryExpAST::Dump(int level) const {
  dumpIndent(level, "PrimaryAST {");
  exp_number_lval->Dump(level + 1);
  dumpIndent(level, "}");
}

void LValAST::Dump(int level) const {
  dumpIndent(level, "LValAST {");
  dumpIndent(level + 1, "ident: " + ident);
  dumpIndent(level, "}");
}

void NumberAST::Dump(int level) const {
  dumpIndent(level, "NumberAST {");
  dumpIndent(level + 1, "value: " + std::to_string(value));
  dumpIndent(level, "}");
}

/** ast value to IR part */
std::unique_ptr<Program> CompUnitAST::to_IR() {
  enter_scope();

  auto program = std::make_unique<Program>();

  for (auto &func_def_base : *func_defs) {
    auto *func_def = dynamic_cast<FuncDefAST *>(func_def_base.get());
    if (!func_def) {
      throw std::runtime_error("Expected FuncDefAST in CompUnitAST");
    }

    std::string symbol = "@" + func_def->ident;
    sym_type ty;
    if (func_def->func_type == "int") {
      ty = sym_type::SYM_TYPE_INTF;
    } else if (func_def->func_type == "void") {
      ty = sym_type::SYM_TYPE_VOIDF;
    } else {
      throw std::runtime_error("Unsupported function type.");
    }
    insert_sym(symbol, ty, 0);

    auto func_ir = func_def->to_IR();
    program->add_function(std::move(func_ir));
  }

  exit_scope();

  return program;
}

std::unique_ptr<Function> FuncDefAST::to_IR() {
  enter_scope();

  // func name
  std::string name = "@" + ident;

  // func type
  std::vector<std::unique_ptr<IRType>> params_ty;
  if (fparams->size() != 0) {
    for (auto &fparam_base : *fparams) {
      auto *fparam = dynamic_cast<FuncFParamAST *>(fparam_base.get());
      if (!fparam) {
        throw std::runtime_error(
            "Expected FuncFParamAST in function parameters");
      }

      if (fparam->type == "int") {
        params_ty.push_back(std::make_unique<Int32Type>());
      } else {
        throw std::runtime_error("Unsupported parameter type: " + fparam->type);
      }
    }
  }

  // func ret type
  std::unique_ptr<IRType> ret_t;
  if (func_type == "int") {
    ret_t = std::make_unique<Int32Type>();
  } else if (func_type == "void") {
    ret_t = std::make_unique<UnitType>();
  } else {
    throw std::runtime_error("Unsupported functon return type: " + func_type);
  }

  auto func_ty =
      std::make_unique<FunctionType>(std::move(params_ty), std::move(ret_t));

  auto func = std::make_unique<Function>(name, std::move(func_ty));

  if (!fparams->empty()) {
    for (size_t i = 0; i < fparams->size(); ++i) {
      auto *fparam = dynamic_cast<FuncFParamAST *>((*fparams)[i].get());
      auto func_arg_ref =
          std::make_unique<FuncArgRefValue>(i, "@" + fparam->ident);
      func->add_param(std::move(func_arg_ref));
    }
  }

  current_func = func.get();

  ////
  // basic block part
  ////
  auto entry_block = std::make_unique<BasicBlock>("%entry");
  current_bb = entry_block.get();
  // alloc for parameters
  if (!fparams->empty()) {
    for (auto &fparam_base : *fparams) {
      auto *fparam = dynamic_cast<FuncFParamAST *>(fparam_base.get());
      if (!fparam)
        continue;

      if (fparam->type == "int") {
        std::string param_local_name = "%" + get_scope_number() + fparam->ident;
        auto alloc_inst = std::make_unique<AllocValue>(param_local_name);
        current_bb->add_inst(std::move(alloc_inst));

        std::string param_arg_name = "@" + fparam->ident;
        auto store_inst = std::make_unique<StoreValue>(
            std::make_unique<VarRefValue>(param_arg_name),
            std::make_unique<VarRefValue>(param_local_name));
        current_bb->add_inst(std::move(store_inst));

        insert_sym(param_local_name, sym_type::SYM_TYPE_VAR, 0);
      }
    }
  }
  auto *b = dynamic_cast<BlockAST *>(block.get());
  b->to_IR();

  return func;
}

void BlockAST::to_IR() {}
