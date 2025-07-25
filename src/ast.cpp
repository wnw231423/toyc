#include <cstddef>
#include <iostream>
#include <sstream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "IR.h"
#include "ast.h"

#include <math.h>
#include <bits/valarray_after.h>

#include "symtable.h"

// keep track of the current function and bbs
static Function *current_func;
static BasicBlock *current_bb;

// the counter and the function for %xxx.
static int temp_cnt = 0;
std::string get_temp() {
  std::ostringstream oss;
  oss << "%" << temp_cnt++;
  return oss.str();
}

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
  if (exp) {
    exp->Dump(level + 1);
  }
  else {
    dumpIndent(level + 1, "return nothing");
  }
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

/*************************/
/** ast value to IR part */
/*************************/

std::unique_ptr<Program> CompUnitAST::to_IR() {
  enter_scope();  // Global scope

  auto program = std::make_unique<Program>();

  for (auto &func_def_base : *func_defs) {
    auto *func_def = dynamic_cast<FuncDefAST *>(func_def_base.get());
    if (!func_def) {
      throw std::runtime_error("Expecting FuncDefAST in CompUnitAST");
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
  // value a FuncDefAst to Functon IR
  // do the following things:
  //   1. enter a new scope, which is the function scope
  //   2. name of the function
  //   3. type of the function, which includes parameters and return type.
  //      something like "int, int -> int"
  //   4. parameters of the function
  //   5. basic blocks.

  enter_scope(); // Function scope

  // func name
  std::string name = "@" + ident;

  // func params type
  std::vector<std::unique_ptr<IRType>> params_ty;
  if (fparams->size() != 0) {
    for (auto &fparam_base : *fparams) {
      auto *fparam = dynamic_cast<FuncFParamAST *>(fparam_base.get());
      if (!fparam) {
        throw std::runtime_error(
            "Expecting FuncFParamAST in function parameters");
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

  // function params
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

void BlockAST::to_IR() {
  auto current_block = current_bb;
  for (const auto &stmt_base : *stmts) {
    auto *stmt = dynamic_cast<StmtAST *>(stmt_base.get());
    if (!stmt) {
      throw std::runtime_error("Expecting StmtAST in BlockAST");
    }

    switch (stmt->type) {
      case 1: // ReturnStmt
        auto *return_stmt =
            dynamic_cast<ReturnStmtAST *>(stmt->stmt.get());
        return_stmt->to_IR();
        break;
      case 2:
        auto *var_decl_stmt =
            dynamic_cast<VarDeclStmtAST *>(stmt->stmt.get());
        var_decl_stmt->to_IR();
        break;
      case 3:
        auto *var_assign_stmt =
            dynamic_cast<VarAssignStmtAST *>(stmt->stmt.get());
        var_assign_stmt->to_IR();
        break;
      case 4:
        auto *exp_stmt =
            dynamic_cast<ExpAST *>(stmt->stmt.get());
        exp_stmt->to_IR();
        break;
      case 5:
        auto *block =
            dynamic_cast<BlockAST *>(stmt->stmt.get());
        // TODO: handle block
        break;
      case 6:
        auto *if_stmt =
            dynamic_cast<IfStmtAST *>(stmt->stmt.get());
        // TODO: handle if stmt
        break;
      case 7:
        auto *while_stmt =
            dynamic_cast<WhileStmtAST *>(stmt->stmt.get());
        // TODO: handle while stmt
        break;
      case 8:
        // TODO: handle break stmt
        break;
      case 9:
        // TODO: handle continue stmt
        break;
    }
  }
}

void ReturnStmtAST::to_IR() {
  auto *return_exp = dynamic_cast<LOrExpAST *>(exp.get());

  // ReturnStmt ::= "return" ";"
  if (!return_exp) {
    if (current_func->f_type->return_type->equals(*std::make_unique<UnitType>())) {
      // void function, just return
      current_bb->add_inst(std::make_unique<ReturnValue>());
    } else {
      throw std::runtime_error("Return nothing in non-void function");
    }
  }

  // ReturnStmt ::= "return" Exp ";"
  else {
    auto ret_temp_name = return_exp->to_IR();
    // create a return value
    auto ret_v = std::make_unique<ReturnValue>(std::move(ret_temp_name));
    auto ret_inst = std::make_unique<ReturnValue>(std::move(ret_v));
    current_bb->add_inst(std::move(ret_inst));
  }
}

void VarDeclStmtAST::to_IR() {
  // i.e. "int a = 5;"
  // %1 = add 0, 5
  // alloc @a i32
  // store %1, @a
  auto *var_def_ast = dynamic_cast<VarDefAST *>(var_def.get());
  if (!var_def_ast) {
    throw std::runtime_error("Expecting VarDefAST in VarDeclStmtAST");
  }

  // VarDef ::= Ident "=" Exp
  auto exp_temp_name = exp->to_IR();
  auto alloc_inst = std::make_unique<AllocValue>("@"+var_def_ast->ident);
  current_bb->add_inst(std::move(alloc_inst));

  auto source = std::make_unique<VarRefValue>(exp_temp_name);
  auto dest = std::make_unique<VarRefValue>("@" + var_def_ast->ident);
  auto store_inst = std::make_unique<StoreValue>(std::move(source), std::move(dest));
  current_bb->add_inst(std::move(store_inst));
}

void VarAssignStmtAST::to_IR() {
  // i.e. "a = 5;"
  // %1 = add 0, 5
  // store %1, @a
  auto *lval_ast = dynamic_cast<LValAST *>(lval.get());
  if (!lval_ast) {
    throw std::runtime_error("Expecting LValAST in VarAssignStmtAST");
  }

  // TODO: check if var exists in the current scope.

  auto exp_temp_name = exp->to_IR();
  auto source = std::make_unique<VarRefValue>(exp_temp_name);
  auto dest = std::make_unique<VarRefValue>("@" + lval_ast->ident);
  auto store_inst = std::make_unique<StoreValue>(std::move(source), std::move(dest));
  current_bb->add_inst(std::move(store_inst));
}
