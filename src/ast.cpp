#include <iostream>
#include <sstream>

#include "ast.h"

void dumpIndent(const int level, const std::string& s) {
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
    for (const auto& stmt : *stmts) {
        stmt->Dump(level + 1);
    }
    dumpIndent(level, "}");
}

void StmtAST::Dump(int level) const {
    dumpIndent(level, "StmtAST {");
    if (type == 6) {
        dumpIndent(level + 1, "nop");
    }
    else if (type == 9) {
        dumpIndent(level + 1, "break");
    }
    else if (type == 10) {
        dumpIndent(level + 1, "continue");
    }
    else {
        stmt->Dump(level + 1);
    }
    dumpIndent(level, "}");
}

void VarDeclStmtAST::Dump(int level) const {
    dumpIndent(level, "VarDeclStmtAST {");
    dumpIndent(level + 1, "type: int");
    var_def -> Dump(level + 1);
    dumpIndent(level, "}");
}

void VarDefAST::Dump(int level) const {
    dumpIndent(level, "VarDefAST {");
    dumpIndent(level + 1, "ident: " + ident);
    exp -> Dump(level + 1);
    dumpIndent(level, "}");
}

void VarAssignStmtAST::Dump(int level) const {
    dumpIndent(level, "VarAssignStmtAST {");
    lval -> Dump(level + 1);
    exp -> Dump(level + 1);
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
    exp -> Dump(level + 2);
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
    exp_number_lval -> Dump(level + 1);
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
