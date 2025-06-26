#ifndef AST_H
#define AST_H
#include <iostream>
#include <sstream>
#include <memory>

inline void dumpIndent(int level, std::string s) {
    for (int i = 0; i < level; ++i) {
        std::cout << "\t";
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

class ReturnAST: public BaseAST {
public:
    int ret_value;

    void Dump(int level) const override {
        std::ostringstream oss;
        oss << "ReturnAST { ret_value: " << ret_value << " }";
        dumpIndent(level, oss.str());
    }
};

#endif //AST_H
