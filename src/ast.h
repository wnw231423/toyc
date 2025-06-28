#ifndef AST_H
#define AST_H
#include <iostream>
#include <sstream>
#include <memory>

/** Dump out a string with level of indents, '\n' in the end. */
inline void dumpIndent(int level, std::string s) {
    for (int i = 0; i < level; ++i) {
        std::cout << "  ";
    }
    std::cout << s << std::endl;
}

class BaseAST {
public:
    virtual ~BaseAST() = default;

    virtual void Dump(int) const = 0;
};

class CompUnitAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_def;

    void Dump(int level) const override {
        dumpIndent(level, "CompUnitAST {");
        func_def -> Dump(level + 1);
        dumpIndent(level, "}");
    }
};

class FuncDefAST: public BaseAST {
public:
    std::string ident;
    std::string func_type;
    std::unique_ptr<BaseAST> block;

    void Dump(int level) const override {
        dumpIndent(level, "FuncDefAST {");
        dumpIndent(level + 1, "ident: " + ident);
        dumpIndent(level + 1, "func_type: " + func_type);
        dumpIndent(level + 1, "block: {");
        block -> Dump(level + 2);
        dumpIndent(level + 1, "}");
        dumpIndent(level, "}");
    }
};

class BlockAST: public BaseAST {
public:
    std::unique_ptr<BaseAST> stmt;

    void Dump(int level) const override {
        dumpIndent(level, "BlockAST {");
        stmt -> Dump(level + 1);
        dumpIndent(level, "}");
    }
};

class ReturnStmtAST: public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;

    void Dump(int level) const override {
        dumpIndent(level, "ReturnAST {");
        exp->Dump(level + 1);
        dumpIndent(level, "}");
    }
};

// Exp ::= LOrExp
class ExpAST: public BaseAST {
public:
    std::unique_ptr<BaseAST> lorExp;

    void Dump(int level) const override {
        dumpIndent(level, "ExpAST {");
        lorExp->Dump(level + 1);
        dumpIndent(level, "}");
    }
};

// LOrExp ::= LAndExp | LOrExp "||" LAndExp
class LOrExpAST: public BaseAST {
public:
    int type;
    std::unique_ptr<BaseAST> landExp_lorExp;
    std::unique_ptr<BaseAST> landExp;

    void Dump(int level) const override {
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
};

// LAndExp ::= EqExp | LAndExp "&&" EqExp
class LAndExpAST: public BaseAST {
public:
    int type;
    std::unique_ptr<BaseAST> eqExp_landExp;
    std::unique_ptr<BaseAST> eqExp;

    void Dump(int level) const override {
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
};

// EqExp ::= RelExp | EqExp ("==" | "!=") RelExp
class EqExpAST: public BaseAST {
public:
    int type;
    std::string eq_op;
    std::unique_ptr<BaseAST> relExp_eqExp;
    std::unique_ptr<BaseAST> relExp;

    void Dump(int level) const override {
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
};

// RelExp ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp
class RelExpAST: public BaseAST {
public:
    int type;
    std::string rel_op;
    std::unique_ptr<BaseAST> addExp_relExp;
    std::unique_ptr<BaseAST> addExp;

    void Dump(int level) const override {
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
};

// AddExp ::= MulExp | AddExp ("+" | "-") MulExp
class AddExpAST: public BaseAST {
public:
    int type;
    std::string add_op;
    std::unique_ptr<BaseAST> mulExp_addExp;
    std::unique_ptr<BaseAST> mulExp;

    void Dump(int level) const override {
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
};

// MulExp ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp
class MulExpAST: public BaseAST {
public:
    int type;
    std::string mul_op;
    std::unique_ptr<BaseAST> unaryExp_mulExp;
    std::unique_ptr<BaseAST> unaryExp;

    void Dump(int level) const override {
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
};

// UnaryExp ::= PrimaryExp | UnaryOp UnaryExp
class UnaryExpAST: public BaseAST {
public:
    int type;
    std::string unary_op;
    std::unique_ptr<BaseAST> primaryExp_unaryExp;

    void Dump(int level) const override {
        if (type == 1) {
            dumpIndent(level, "UnaryExpAST {");
            primaryExp_unaryExp->Dump(level + 1);
            dumpIndent(level, "}");
        } else if (type == 2) {
            dumpIndent(level, "UnaryExpAST {");
            dumpIndent(level + 1, "unary_op: " + unary_op);
            primaryExp_unaryExp->Dump(level + 1);
            dumpIndent(level, "}");
        }
    }
};

// PrimaryExp ::= "(" Exp ")" | Number
class PrimaryExpAST: public BaseAST {
public:
    int type;
    std::unique_ptr<BaseAST> exp_number;

    void Dump(int level) const override {
        dumpIndent(level, "PrimaryAST {");
        exp_number -> Dump(level + 1);
        dumpIndent(level, "}");
    }
};

class NumberAST: public BaseAST {
public:
    int value;

    void Dump(int level) const override {
        dumpIndent(level, "NumberAST {");
        dumpIndent(level + 1, "value: " + std::to_string(value));
        dumpIndent(level, "}");
    }
};

#endif //AST_H
