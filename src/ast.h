#ifndef AST_H
#define AST_H
#include <memory>

/** Dump out a string with level of indents, '\n' in the end. */
void dumpIndent(int level, std::string s);

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

class BlockAST: public BaseAST {
public:
    std::unique_ptr<BaseAST> stmt;

    void Dump(int level) const override;
};

class ReturnStmtAST: public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;

    void Dump(int level) const override;
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

// PrimaryExp ::= "(" Exp ")" | Number
class PrimaryExpAST: public BaseAST {
public:
    int type;
    std::unique_ptr<BaseAST> exp_number;

    void Dump(int level) const override;
};

class NumberAST: public BaseAST {
public:
    int value;

    void Dump(int level) const override;
};

#endif //AST_H