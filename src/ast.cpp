#include <iostream>
#include <sstream>

#include "ast.h"

void dumpIndent(int level, std::string s) {
    for (int i = 0; i < level; ++i) {
        std::cout << "  ";
    }
    std::cout << s << std::endl;
}

void CompUnitAST::Dump(int level) const {
    dumpIndent(level, "CompUnitAST {");
    func_def -> Dump(level + 1);
    dumpIndent(level, "}");
}

void FuncDefAST::Dump(int level) const {
    dumpIndent(level, "FuncDefAST {");
    dumpIndent(level + 1, "ident: " + ident);
    dumpIndent(level + 1, "func_type: " + func_type);
    dumpIndent(level + 1, "block: {");
    block -> Dump(level + 2);
    dumpIndent(level + 1, "}");
    dumpIndent(level, "}");
}

void BlockAST::Dump(int level) const {
    dumpIndent(level, "BlockAST {");
    stmt -> Dump(level + 1);
    dumpIndent(level, "}");
}

void ReturnStmtAST::Dump(int level) const {
    dumpIndent(level, "ReturnAST {");
    exp->Dump(level + 1);
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
        primaryExp_unaryExp->Dump(level + 1);
        dumpIndent(level, "}");
    } else if (type == 2) {
        dumpIndent(level, "UnaryExpAST {");
        dumpIndent(level + 1, "unary_op: " + unary_op);
        primaryExp_unaryExp->Dump(level + 1);
        dumpIndent(level, "}");
    }
}

void PrimaryExpAST::Dump(int level) const {
    dumpIndent(level, "PrimaryAST {");
    exp_number -> Dump(level + 1);
    dumpIndent(level, "}");
}

void NumberAST::Dump(int level) const {
    dumpIndent(level, "NumberAST {");
    dumpIndent(level + 1, "value: " + std::to_string(value));
    dumpIndent(level, "}");
}
