#ifndef AST_H
#define AST_H
#include <iostream>
#include <memory>


class BaseAST {
public:
    virtual ~BaseAST() = default;

    virtual void Dump() const = 0;
};

class CompUnitAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override {
        std::cout << "CompUnitAST { ";
        func_def -> Dump();
        std::cout << " }\n";
    }
};

class FuncDefAST: public BaseAST {
public:
    std::string ident;
    std::string func_type;
    std::unique_ptr<BaseAST> block;

    void Dump() const override {
        std::cout << "FuncDefAST {";
        std::cout << "\t ident: " << ident << "\n";
        std::cout << "\t func_type: " << func_type << "\n";
        std::cout << "\t block: ";
        block -> Dump();
    }
};

class BlockAST: public BaseAST {
public:
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override {
        std::cout << "BlockAST {\n";
        stmt -> Dump();
    }
};

class ReturnAST: public BaseAST {
public:
    int ret_value;

    void Dump() const override {
        std::cout << "ReturnAST { return value: " << ret_value << " }\n";
    }
};

#endif //AST_H
