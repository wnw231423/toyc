/** This is AST header file.
 *  Comments are in the form of EBNF, where [] means optional, {} means
 * repetition.
 */
#ifndef AST_H
#define AST_H
#include "IR.h"
#include <memory>
#include <vector>

/** Dump out a string with level of indents, '\n' in the end. */
void dumpIndent(int level, const std::string &s);

class BaseAST {
public:
  virtual ~BaseAST() = default;

  virtual void Dump(int) const = 0;
};

// CompUnit ::= { FuncDef }
class CompUnitAST : public BaseAST {
public:
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> func_defs;

  std::unique_ptr<Program> to_IR();
  void Dump(int level) const override;
};

// FuncDef ::= "int" Ident "(" [FuncFParams] ")" Block
// FuncFParams ::= FuncFParam {"," FuncFParam}
class FuncDefAST : public BaseAST {
public:
  std::string ident;
  std::string func_type;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> fparams;
  std::unique_ptr<BaseAST> block;

  std::unique_ptr<Function> to_IR();
  void Dump(int level) const override;
};

// FuncFParam ::= "int" Ident
class FuncFParamAST : public BaseAST {
public:
  std::string type;
  std::string ident;

  void Dump(int level) const override;
};

// Block ::= "{" Stmt* "}"
// In the parser
// Block ::= "{" StmtList "}"
// StmtList ::= ε | StmtList Stmt
class BlockAST : public BaseAST {
public:
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> stmts;

  void to_IR();
  void Dump(int level) const override;
};

// Stmt ::= ReturnStmt | VarDeclStmt | VarAssignStmt | Exp ";" | Block | ";" |
// IfStmt | WhileStmt | break | continue
class StmtAST : public BaseAST {
public:
  int type;
  std::unique_ptr<BaseAST>
      stmt; // Too much variant so use "stmt" as a generic name

  void Dump(int) const override;
  void to_IR();
};

// VarDeclStmt ::= "int" VarDef ";"
class VarDeclStmtAST : public BaseAST {
public:
  // std::string type = "int";  // only int, so hardcode it
  std::unique_ptr<BaseAST> var_def;

  void Dump(int) const override;
  void to_IR();
};

// VarDef ::= Ident "=" Exp
class VarDefAST : public BaseAST {
public:
  std::string ident;
  std::unique_ptr<BaseAST> exp;

  void Dump(int level) const override;
};

// VarAssignStmt ::= LVal "=" Exp ";"
class VarAssignStmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> lval;
  std::unique_ptr<BaseAST> exp;

  void Dump(int level) const override;
  void to_IR();
};

// ReturnStmt ::= "return" Exp? ";"
class ReturnStmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> exp;  // nullptr when return nothing.

  void Dump(int level) const override;
  void to_IR();
};

// IfStmt ::= "if" "(" Exp ")" stmt ("else" stmt)?
class IfStmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> stmt_then;
  std::unique_ptr<BaseAST> stmt_else; // Optional, can be nullptr

  void Dump(int level) const override;
  void to_IR();
};

// WhileStmt ::= "while" "(" Exp ")" stmt
class WhileStmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> stmt;

  void Dump(int) const override;
  void to_IR();
};

// Exp ::= LOrExp
class ExpAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> lorExp;

  void Dump(int level) const override;
  std::string to_IR();
};

// LOrExp ::= LAndExp | LOrExp "||" LAndExp
class LOrExpAST : public BaseAST {
public:
  int type;  // 1 for LAndExp, 2 for LOrExp "||" LAndExp
  std::unique_ptr<BaseAST> landExp_lorExp;
  std::unique_ptr<BaseAST> landExp;

  void Dump(int level) const override;
  std::string to_IR();
};

// LAndExp ::= EqExp | LAndExp "&&" EqExp
class LAndExpAST : public BaseAST {
public:
  int type;
  std::unique_ptr<BaseAST> eqExp_landExp;
  std::unique_ptr<BaseAST> eqExp;

  void Dump(int level) const override;
  std::string to_IR();
};

// EqExp ::= RelExp | EqExp ("==" | "!=") RelExp
class EqExpAST : public BaseAST {
public:
  int type;
  std::string eq_op;
  std::unique_ptr<BaseAST> relExp_eqExp;
  std::unique_ptr<BaseAST> relExp;

  void Dump(int level) const override;
  std::string to_IR();
};

// RelExp ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp
class RelExpAST : public BaseAST {
public:
  int type;
  std::string rel_op;
  std::unique_ptr<BaseAST> addExp_relExp;
  std::unique_ptr<BaseAST> addExp;

  void Dump(int level) const override;
  std::string to_IR();
};

// AddExp ::= MulExp | AddExp ("+" | "-") MulExp
class AddExpAST : public BaseAST {
public:
  int type;
  std::string add_op;
  std::unique_ptr<BaseAST> mulExp_addExp;
  std::unique_ptr<BaseAST> mulExp;

  void Dump(int level) const override;
  std::string to_IR();
};

// MulExp ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp
class MulExpAST : public BaseAST {
public:
  int type;
  std::string mul_op;
  std::unique_ptr<BaseAST> unaryExp_mulExp;
  std::unique_ptr<BaseAST> unaryExp;

  void Dump(int level) const override;
  std::string to_IR();
};

// UnaryExp ::= PrimaryExp | UnaryOp UnaryExp | FuncCall
// FuncRParams ::= Exp {"," Exp}
class UnaryExpAST : public BaseAST {
public:
  int type;
  std::string unary_op;
  std::unique_ptr<BaseAST> primaryExp_unaryExp_funcCall;

  void Dump(int level) const override;
  std::string to_IR();
};

// FuncCall ::= Ident "(" [FuncRParams] ")"
// FuncRParams ::= Exp {"," Exp}
class FuncCallAST : public BaseAST {
public:
  std::string ident;
  std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> rparams;

  void Dump(int level) const override;
  std::string to_IR();
};

// PrimaryExp ::= "(" Exp ")" | Number | LVal
class PrimaryExpAST : public BaseAST {
public:
  int type;
  std::unique_ptr<BaseAST> exp_number_lval;

  void Dump(int level) const override;
  std::string to_IR();
};

class LValAST : public BaseAST {
public:
  std::string ident;

  void Dump(int level) const override;
  std::string to_IR();
};

class NumberAST : public BaseAST {
public:
  int value;

  void Dump(int level) const override;
  std::string to_IR() {return "should not reach";};
};

#endif // AST_H
