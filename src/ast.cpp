#include <cstddef>
#include <iostream>
#include <sstream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "IR.h"
#include "ast.h"
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

// the counter and the function for %then_xxx, %else_xxx and %if_end_xxx.
// should inc if_cnt on your own.
static int if_cnt = 0;
std::string get_then_temp() {
  return "%then_" + std::to_string(if_cnt);
}

std::string get_else_temp() {
  return "%else_" + std::to_string(if_cnt);
}

std::string get_end_temp() {
  return "%if_end_" + std::to_string(if_cnt);
}

void inc_if_cnt() {
  if_cnt++;
}

// the counter and the function for %while_entry_xxx, %while_body_xxx and %while_end_xxx.
// should inc while_cnt on your own.
static int while_cnt = 0;
std::string get_while_entry_temp() {
  return "%while_entry_" + std::to_string(while_cnt);
}

std::string get_while_body_temp() {
  return "%while_body_" + std::to_string(while_cnt);
}

std::string get_while_end_temp() {
  return "%while_end_" + std::to_string(while_cnt);
}

void inc_while_cnt() {
  while_cnt++;
}

// to check if we are in a while loop.
static int in_while = 0;
// used for break and continue.
static int cur_while = 0;


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
  current_func->add_basic_block(std::move(entry_block));
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
  for (const auto &stmt_base : *stmts) {
    auto *stmt = dynamic_cast<StmtAST *>(stmt_base.get());
    if (!stmt) {
      throw std::runtime_error("Expecting StmtAST in BlockAST");
    }

    switch (stmt->type) {
      case 1: {
        // ReturnStmt
        auto *return_stmt =
            dynamic_cast<ReturnStmtAST *>(stmt->stmt.get());
        return_stmt->to_IR();
        break;
      }
      case 2: {
        auto *var_decl_stmt =
            dynamic_cast<VarDeclStmtAST *>(stmt->stmt.get());
        var_decl_stmt->to_IR();
        break;
      }
      case 3: {
        auto *var_assign_stmt =
            dynamic_cast<VarAssignStmtAST *>(stmt->stmt.get());
        var_assign_stmt->to_IR();
        break;
      }
      case 4: {
        auto *exp_stmt =
            dynamic_cast<ExpAST *>(stmt->stmt.get());
        exp_stmt->to_IR();
        break;
      }
      case 5: {
        auto *block =
            dynamic_cast<BlockAST *>(stmt->stmt.get());
        enter_scope();
        block->to_IR();
        exit_scope();
        break;
      }
      case 6:
        break;
      case 7: {
        auto *if_stmt =
            dynamic_cast<IfStmtAST *>(stmt->stmt.get());
        if_stmt->to_IR();
        break;
      }
      case 8: {
        auto *while_stmt =
            dynamic_cast<WhileStmtAST *>(stmt->stmt.get());
        while_stmt->to_IR();
        break;
      }
      case 9: {
        // jump %while_end_xxx
        std::string while_end_name = "%while_end_" + std::to_string(cur_while);
        auto jump_inst_break = std::make_unique<JumpValue>(while_end_name);
        current_bb->add_inst(std::move(jump_inst_break));
        break;
      }
      case 10: {
        // jump %while_entry_xxx
        std::string while_entry_name = "%while_entry_" + std::to_string(cur_while);
        auto jump_inst_continue = std::make_unique<JumpValue>(while_entry_name);
        current_bb->add_inst(std::move(jump_inst_continue));
        break;
      }
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
    auto ret_v = std::make_unique<VarRefValue>(ret_temp_name);
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
  auto *exp_ast = dynamic_cast<ExpAST *>(var_def_ast->exp.get());
  auto exp_temp_name = exp_ast->to_IR();
  auto alloc_inst = std::make_unique<AllocValue>("@"+var_def_ast->ident);
  current_bb->add_inst(std::move(alloc_inst));

  auto source = std::make_unique<VarRefValue>(exp_temp_name);
  auto dest = std::make_unique<VarRefValue>("@" + var_def_ast->ident);
  auto store_inst = std::make_unique<StoreValue>(std::move(source), std::move(dest));
  current_bb->add_inst(std::move(store_inst));

  // insert the variable into the symbol table
  if (exist_sym("@" + var_def_ast->ident)) {
    throw std::runtime_error("Variable " + var_def_ast->ident + " already exists.");
  }
  insert_sym("@" + var_def_ast->ident, sym_type::SYM_TYPE_VAR, 0);
}

void VarAssignStmtAST::to_IR() {
  // i.e. "a = 5;"
  // %1 = add 0, 5
  // store %1, @a
  auto *lval_ast = dynamic_cast<LValAST *>(lval.get());
  if (!lval_ast) {
    throw std::runtime_error("Expecting LValAST in VarAssignStmtAST");
  }

  // check if var exists in the current scope.
  if (!exist_sym(lval_ast->ident)) {
    throw std::runtime_error("Variable " + lval_ast->ident + " has no declaration.");
  }

  auto *exp_ast = dynamic_cast<ExpAST *>(exp.get());
  auto exp_temp_name = exp_ast->to_IR();
  auto source = std::make_unique<VarRefValue>(exp_temp_name);
  auto dest = std::make_unique<VarRefValue>("@" + lval_ast->ident);
  auto store_inst = std::make_unique<StoreValue>(std::move(source), std::move(dest));
  current_bb->add_inst(std::move(store_inst));
}

void handle_then_block(std::unique_ptr<BaseAST> stmt_then, const std::string &then_block_name, const std::string &end_block_name) {
  auto then_block = std::make_unique<BasicBlock>(then_block_name);
  current_func->add_basic_block(std::move(then_block));
  current_bb = current_func->bbs.back().get();

  // enter a new scope for the then block
  enter_scope();
  auto *block_ast = dynamic_cast<BlockAST *>(stmt_then.get());
  if (!block_ast) {
    throw std::runtime_error("Expecting BlockAST in IfStmtAST");
  }
  block_ast->to_IR();

  // add a jump to the end block
  auto jump_inst = std::make_unique<JumpValue>(end_block_name);
  current_bb->add_inst(std::move(jump_inst));

  exit_scope(); // exit the then block scope
}

void handle_else_block(std::unique_ptr<BaseAST> stmt_else, const std::string &else_block_name, const std::string &end_block_name) {
  auto else_block = std::make_unique<BasicBlock>(else_block_name);
  current_func->add_basic_block(std::move(else_block));
  current_bb = current_func->bbs.back().get();

  // enter a new scope for the else block
  enter_scope();
  auto *block_ast = dynamic_cast<BlockAST *>(stmt_else.get());
  if (!block_ast) {
    throw std::runtime_error("Expecting BlockAST in IfStmtAST");
  }
  block_ast->to_IR();

  // add a jump to the end block
  auto jump_inst = std::make_unique<JumpValue>(end_block_name);
  current_bb->add_inst(std::move(jump_inst));

  exit_scope(); // exit the else block scope
}

void IfStmtAST::to_IR() {
  auto *exp_ast = dynamic_cast<ExpAST *>(exp.get());
  auto exp_temp_name = exp_ast->to_IR();
  auto exp_value = std::make_unique<VarRefValue>(exp_temp_name);
  auto then_block_name = get_then_temp();
  auto else_block_name = get_else_temp();
  auto end_block_name = get_end_temp();
  inc_if_cnt();

  if (stmt_else) {
    // br %1, %then_1, %else_1
    auto branch_inst = std::make_unique<BranchValue>(
        std::move(exp_value), then_block_name, else_block_name);

    // handle then block
    handle_then_block(std::move(stmt_then), then_block_name, end_block_name);
    // handle else block
    handle_else_block(std::move(stmt_else), else_block_name, end_block_name);
  } else {
    // br %1, %then_1, %end_1
    auto branch_inst = std::make_unique<BranchValue>(
        std::move(exp_value), then_block_name, end_block_name);
    // handle then block
    handle_then_block(std::move(stmt_then), then_block_name, end_block_name);
  }

  // change the current basic block to the end block
  auto end_block = std::make_unique<BasicBlock>(end_block_name);
  current_func->add_basic_block(std::move(end_block));
  current_bb = current_func->bbs.back().get();
}

void WhileStmtAST::to_IR() {
  auto while_entry_name = get_while_entry_temp();
  auto while_body_name = get_while_body_temp();
  auto end_block_name = get_while_end_temp();
  cur_while = while_cnt;
  inc_while_cnt();

  // jump %while_entry_1
  auto jump_inst = std::make_unique<JumpValue>(while_entry_name);
  current_bb->add_inst(std::move(jump_inst));

  // create the while entry block
  auto while_entry_block = std::make_unique<BasicBlock>(while_entry_name);
  current_func->add_basic_block(std::move(while_entry_block));
  current_bb = current_func->bbs.back().get();
  auto *exp_ast = dynamic_cast<ExpAST *>(exp.get());
  std::string exp_temp_name = exp_ast->to_IR();
  auto exp_value = std::make_unique<VarRefValue>(exp_temp_name);
  // br %1, %while_body_1, %while_end_1
  auto branch_inst = std::make_unique<BranchValue>(
      std::move(exp_value), while_body_name, end_block_name);
  current_bb->add_inst(std::move(branch_inst));

  // create the while body block
  in_while = 1;
  auto while_body_block = std::make_unique<BasicBlock>(while_body_name);
  current_func->add_basic_block(std::move(while_body_block));
  current_bb = current_func->bbs.back().get();
  // enter a new scope for the while body
  enter_scope();
  auto *block_ast = dynamic_cast<BlockAST *>(stmt.get());
  if (!block_ast) {
    throw std::runtime_error("Expecting BlockAST in WhileStmtAST");
  }
  block_ast->to_IR();
  // add a jump to the while entry block
  auto jump_inst_body = std::make_unique<JumpValue>(while_entry_name);
  current_bb->add_inst(std::move(jump_inst_body));
  exit_scope(); // exit the while body scope
  in_while = 0;

  // create the end block
  auto end_block = std::make_unique<BasicBlock>(end_block_name);
  current_func->add_basic_block(std::move(end_block));
  current_bb = current_func->bbs.back().get();
}

std::string ExpAST::to_IR() {
  // Exp ::= LOrExp
  auto *lor_exp = dynamic_cast<LOrExpAST *>(lorExp.get());
  if (!lor_exp) {
    throw std::runtime_error("Expecting LOrExpAST in ExpAST");
  }
  return lor_exp->to_IR();
}

std::string LOrExpAST::to_IR() {
  if (type == 1) {
    // LAndExp
    auto *res =
        dynamic_cast<LAndExpAST *>(landExp_lorExp.get());
    return res->to_IR();
  } else if (type == 2) {
    // LOrExp "||" LAndExp
    auto *l = dynamic_cast<LOrExpAST *>(landExp_lorExp.get());
    auto left_temp_name = l->to_IR();
    auto lhs = std::make_unique<VarRefValue>(left_temp_name);

    auto *r = dynamic_cast<LAndExpAST *>(landExp.get());
    auto right_temp_name = r->to_IR();
    auto rhs = std::make_unique<VarRefValue>(right_temp_name);

    auto temp_name = get_temp();
    auto binary_inst = std::make_unique<BinaryValue>(
        temp_name, BinaryOp::OR, std::move(lhs), std::move(rhs));
    current_bb->add_inst(std::move(binary_inst));
    return temp_name;
  }
}

std::string LAndExpAST::to_IR() {
  if (type == 1) {
    // EqExp
    auto *res = dynamic_cast<EqExpAST *>(eqExp_landExp.get());
    return res->to_IR();
  } else if (type == 2) {
    // LAndExp "&&" EqExp
    auto *l = dynamic_cast<LAndExpAST *>(eqExp_landExp.get());
    auto left_temp_name = l->to_IR();
    auto lhs = std::make_unique<VarRefValue>(left_temp_name);

    auto *r = dynamic_cast<EqExpAST *>(eqExp.get());
    auto right_temp_name = r->to_IR();
    auto rhs = std::make_unique<VarRefValue>(right_temp_name);
    auto temp_name = get_temp();
    auto binary_inst = std::make_unique<BinaryValue>(
        temp_name, BinaryOp::AND, std::move(lhs), std::move(rhs));
    current_bb->add_inst(std::move(binary_inst));
    return temp_name;
  }
}

std::string EqExpAST::to_IR() {
  if (type == 1) {
    // RelExp
    auto *res = dynamic_cast<RelExpAST *>(relExp_eqExp.get());
    return res->to_IR();
  } else if (type == 2) {
    // EqExp "==" RelExp
    auto *l = dynamic_cast<EqExpAST *>(relExp_eqExp.get());
    auto left_temp_name = l->to_IR();
    auto lhs = std::make_unique<VarRefValue>(left_temp_name);

    auto *r = dynamic_cast<RelExpAST *>(relExp.get());
    auto right_temp_name = r->to_IR();
    auto rhs = std::make_unique<VarRefValue>(right_temp_name);
    auto temp_name = get_temp();
    BinaryOp op = (eq_op == "==") ? BinaryOp::EQ : BinaryOp::NE;
    auto binary_inst = std::make_unique<BinaryValue>(
        temp_name, op, std::move(lhs), std::move(rhs));
    current_bb->add_inst(std::move(binary_inst));
    return temp_name;
  }
}

std::string RelExpAST::to_IR() {
  if (type == 1) {
    // AddExp
    auto *res = dynamic_cast<AddExpAST *>(addExp_relExp.get());
    return res->to_IR();
  } else if (type == 2) {
    // RelExp "<" AddExp
    auto *l = dynamic_cast<RelExpAST *>(addExp_relExp.get());
    auto left_temp_name = l->to_IR();
    auto lhs = std::make_unique<VarRefValue>(left_temp_name);

    auto *r = dynamic_cast<AddExpAST *>(addExp.get());
    auto right_temp_name = r->to_IR();
    auto rhs = std::make_unique<VarRefValue>(right_temp_name);
    auto temp_name = get_temp();
    BinaryOp op;
    if (rel_op == "<") {
      op = BinaryOp::LT;
    } else if (rel_op == ">") {
      op = BinaryOp::GT;
    } else if (rel_op == "<=") {
      op = BinaryOp::LE;
    } else if (rel_op == ">=") {
      op = BinaryOp::GE;
    } else {
      throw std::runtime_error("Unsupported relational operator: " + rel_op);
    }
    auto binary_inst = std::make_unique<BinaryValue>(
        temp_name, op, std::move(lhs), std::move(rhs));
    current_bb->add_inst(std::move(binary_inst));
    return temp_name;
  }
}

std::string AddExpAST::to_IR() {
  if (type == 1) {
    // MulExp
    auto *res = dynamic_cast<MulExpAST *>(mulExp_addExp.get());
    return res->to_IR();
  } else if (type == 2) {
    // AddExp ("+" | "-") MulExp
    auto *l = dynamic_cast<AddExpAST *>(mulExp_addExp.get());
    auto left_temp_name = l->to_IR();
    auto lhs = std::make_unique<VarRefValue>(left_temp_name);

    auto *r = dynamic_cast<MulExpAST *>(mulExp.get());
    auto right_temp_name = r->to_IR();
    auto rhs = std::make_unique<VarRefValue>(right_temp_name);
    auto temp_name = get_temp();
    BinaryOp op = (add_op == "+") ? BinaryOp::ADD : BinaryOp::SUB;
    auto binary_inst = std::make_unique<BinaryValue>(
        temp_name,op,  std::move(lhs), std::move(rhs));
    current_bb->add_inst(std::move(binary_inst));
    return temp_name;
  } else {
    throw std::runtime_error("Unknown AddExp type.");
  }
}

std::string MulExpAST::to_IR() {
  if (type == 1) {
    // UnaryExp
    auto *res = dynamic_cast<UnaryExpAST *>(unaryExp_mulExp.get());
    return res->to_IR();
  } else if (type == 2) {
    // MulExp "*" UnaryExp
    auto *l = dynamic_cast<MulExpAST *>(unaryExp_mulExp.get());
    auto left_temp_name = l->to_IR();
    auto lhs = std::make_unique<VarRefValue>(left_temp_name);

    auto *r = dynamic_cast<UnaryExpAST *>(unaryExp.get());
    auto right_temp_name = r->to_IR();
    auto rhs = std::make_unique<VarRefValue>(right_temp_name);

    auto temp_name = get_temp();
    BinaryOp op;
    if (mul_op == "*") {
      op = BinaryOp::MUL;
    } else if (mul_op == "/") {
      op = BinaryOp::DIV;
    } else if (mul_op == "%") {
      op = BinaryOp::MOD;
    } else {
      throw std::runtime_error("Unsupported multiplication operator: " + mul_op);
    }
    auto binary_inst = std::make_unique<BinaryValue>(
        temp_name, op, std::move(lhs), std::move(rhs));
    current_bb->add_inst(std::move(binary_inst));
    return temp_name;
  } else {
    throw std::runtime_error("Unknown MulExp type.");
  }
}

std::string UnaryExpAST::to_IR() {
  if (type == 1) {
    // PrimaryExp
    auto *res = dynamic_cast<PrimaryExpAST *>(primaryExp_unaryExp_funcCall.get());
    return res->to_IR();
  } else if (type == 2) {
    // UnaryOp PrimaryExp
    auto *r = dynamic_cast<PrimaryExpAST *>(primaryExp_unaryExp_funcCall.get());
    auto rhs = std::make_unique<VarRefValue>(r->to_IR());
    auto temp_name = get_temp();
    if (unary_op == "-") {
      // negate the value
      auto lhs = std::make_unique<IntergerValue>(0);
      auto binary_inst = std::make_unique<BinaryValue>(
        temp_name, BinaryOp::SUB, std::move(lhs), std::move(rhs));
    } else if (unary_op == "!") {
      // logical not
      auto lhs = std::make_unique<IntergerValue>(0);
      auto binary_inst = std::make_unique<BinaryValue>(
        temp_name,BinaryOp::EQ, std::move(lhs), std::move(rhs));
    } else {
      throw std::runtime_error("Unsupported unary operator: " + unary_op);
    }
    return temp_name;
  } else if (type == 3) {
    // FuncCall
    auto *res = dynamic_cast<FuncCallAST *>(primaryExp_unaryExp_funcCall.get());
    return res->to_IR();
  } else {
    throw std::runtime_error("unknown unary exp ast.");
  }
}

std::string FuncCallAST::to_IR() {
  // FuncCall ::= Ident "(" [RParams] ")"
  // RParams ::= Exp {"," Exp}
  std::string func_name = "@" + ident;
  if (!exist_sym(func_name)) {
    throw std::runtime_error("Function " + ident + " has no declaration.");
  }

  auto func_type = query_sym(func_name).second->type;
  if ((func_type != sym_type::SYM_TYPE_INTF) && (func_type != sym_type::SYM_TYPE_VOIDF)) {
    throw std::runtime_error(ident + " is not a function.");
  }

  std::vector<std::unique_ptr<IRValue>> args;
  if (rparams) {
    for (const auto &param_base : *rparams) {
      auto *param = dynamic_cast<ExpAST *>(param_base.get());
      if (!param) {
        throw std::runtime_error("Expecting ExpAST in FuncCallAST RParams");
      }
      auto param_temp_name = param->to_IR();
      args.push_back(std::make_unique<VarRefValue>(param_temp_name));
    }
  }

  auto res_name = get_temp();
  std::unique_ptr<IRType> ret_type;
  if (func_type == sym_type::SYM_TYPE_INTF) {
    ret_type = std::make_unique<Int32Type>();
  } else if (func_type == sym_type::SYM_TYPE_VOIDF) {
    ret_type = std::make_unique<UnitType>();
  }

  auto func_call_inst = std::make_unique<CallValue>(
      res_name, func_name, args, std::move(ret_type));
  current_bb->add_inst(std::move(func_call_inst));

  return res_name;
}

std::string PrimaryExpAST::to_IR() {
  if (type == 1) {
    // Exp
    auto *res = dynamic_cast<ExpAST *>(exp_number_lval.get());
    return res->to_IR();
  } else if (type == 2) {
    // Number
    auto *number_ast = dynamic_cast<NumberAST *>(exp_number_lval.get());
    if (!number_ast) {
      throw std::runtime_error("Expecting NumberAST in PrimaryExpAST");
    }
    auto temp_name = get_temp();
    auto number_value = std::make_unique<IntergerValue>(number_ast->value);
    auto number_inst = std::make_unique<BinaryValue>(
        temp_name, BinaryOp::ADD, std::make_unique<IntergerValue>(0), std::move(number_value));
    current_bb->add_inst(std::move(number_inst));
    return temp_name;
  } else if (type == 3) {
    // LVal
    auto *res = dynamic_cast<LValAST *>(exp_number_lval.get());
    return res->to_IR();
  }
}

std::string LValAST::to_IR() {
  // LVal ::= Ident
  if (!exist_sym(ident)) {
    throw std::runtime_error("Variable " + ident + " has no declaration.");
  }
  return "@" + ident; // return the variable name
}