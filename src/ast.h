#ifndef AST_H
#define AST_H
#include <memory>
#include <vector>

/** Dump out a string with level of indents, '\n' in the end. */
void dumpIndent(int level, const std::string& s);

class BaseAST {
public:
    virtual ~BaseAST() = default;

    virtual void Dump(int) const = 0;
};

class CompUnitAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_def;

    void Dump(int level) const override;
};

class FuncDefAST: public BaseAST {
public:
    std::string ident;
    std::string func_type;
    std::unique_ptr<BaseAST> block;

    void Dump(int level) const override;
};

// Block ::= "{" Stmt* "}"
// In the parser
// Block ::= "{" StmtList "}"
// StmtList ::= ε | StmtList Stmt
class BlockAST: public BaseAST {
public:
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> stmts;

    void Dump(int level) const override;
};

// Stmt ::= ReturnStmt | VarDeclStmt | VarAssignStmt | Exp ";" | Block | ";" | IfStmt | WhileStmt | break | continue
class StmtAST: public BaseAST {
public:
    int type;
    std::unique_ptr<BaseAST> stmt;  // Too much variant so use "stmt" as a generic name

    void Dump(int) const override;
};

// VarDeclStmt ::= "int" VarDef ";"
class VarDeclStmtAST: public BaseAST {
public:
    // std::string type = "int";  // only int, so hardcode it
    std::unique_ptr<BaseAST> var_def;

    void Dump(int) const override;
};

// VarDef ::= Ident "=" Exp
class VarDefAST: public BaseAST {
public:
    std::string ident;
    std::unique_ptr<BaseAST> exp;

    void Dump(int level) const override;
};

// VarAssignStmt ::= LVal "=" Exp ";"
class VarAssignStmtAST: public BaseAST {
public:
    std::unique_ptr<BaseAST> lval;
    std::unique_ptr<BaseAST> exp;

    void Dump(int level) const override;
};

// ReturnStmt ::= "return" Exp ";"
class ReturnStmtAST: public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;

    void Dump(int level) const override;
};

// IfStmt ::= "if" "(" Exp ")" stmt ("else" stmt)?
class IfStmtAST: public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> stmt_then;
    std::unique_ptr<BaseAST> stmt_else;  // Optional, can be nullptr

    void Dump(int level) const override;
};

// WhileStmt ::= "while" "(" Exp ")" stmt
class WhileStmtAST: public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> stmt;

    void Dump(int) const override;
};

// Exp ::= LOrExp
class ExpAST: public BaseAST {
public:
    std::unique_ptr<BaseAST> lorExp;

    void Dump(int level) const override;
};

// LOrExp ::= LAndExp | LOrExp "||" LAndExp
class LOrExpAST: public BaseAST {
public:
    int type;
    std::unique_ptr<BaseAST> landExp_lorExp;
    std::unique_ptr<BaseAST> landExp;

    void Dump(int level) const override;
};

// LAndExp ::= EqExp | LAndExp "&&" EqExp
class LAndExpAST: public BaseAST {
public:
    int type;
    std::unique_ptr<BaseAST> eqExp_landExp;
    std::unique_ptr<BaseAST> eqExp;

    void Dump(int level) const override;
};

// EqExp ::= RelExp | EqExp ("==" | "!=") RelExp
class EqExpAST: public BaseAST {
public:
    int type;
    std::string eq_op;
    std::unique_ptr<BaseAST> relExp_eqExp;
    std::unique_ptr<BaseAST> relExp;

    void Dump(int level) const override;
};

// RelExp ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp
class RelExpAST: public BaseAST {
public:
    int type;
    std::string rel_op;
    std::unique_ptr<BaseAST> addExp_relExp;
    std::unique_ptr<BaseAST> addExp;

    void Dump(int level) const override;
};

// AddExp ::= MulExp | AddExp ("+" | "-") MulExp
class AddExpAST: public BaseAST {
public:
    int type;
    std::string add_op;
    std::unique_ptr<BaseAST> mulExp_addExp;
    std::unique_ptr<BaseAST> mulExp;

    void Dump(int level) const override;
};

// MulExp ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp
class MulExpAST: public BaseAST {
public:
    int type;
    std::string mul_op;
    std::unique_ptr<BaseAST> unaryExp_mulExp;
    std::unique_ptr<BaseAST> unaryExp;

    void Dump(int level) const override;
};

// UnaryExp ::= PrimaryExp | UnaryOp UnaryExp
class UnaryExpAST: public BaseAST {
public:
    int type;
    std::string unary_op;
    std::unique_ptr<BaseAST> primaryExp_unaryExp;

    void Dump(int level) const override;
};

// PrimaryExp ::= "(" Exp ")" | Number | LVal
class PrimaryExpAST: public BaseAST {
public:
    int type;
    std::unique_ptr<BaseAST> exp_number_lval;

    void Dump(int level) const override;
};

class LValAST: public BaseAST {
public:
    std::string ident;

    void Dump(int level) const override;
};

class NumberAST: public BaseAST {
public:
    int value;

    void Dump(int level) const override;
};

#endif //AST_H