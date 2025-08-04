/** This file is to visit an IR program unit and return string of corresponding RV32 assembly code. */
#ifndef VISIT_H
#define VISIT_H
#include <string>
#include "IR.h"

std::string visit_program(std::unique_ptr<Program> program);

std::string visit_function(const std::unique_ptr<Function> &function);

std::string visit_basic_block(const std::unique_ptr<BasicBlock> &basic_block);

std::string visit_value(const std::unique_ptr<IRValue> &value);

void visit_alloc_value(const AllocValue * alloc_value);

std::string visit_load_value(const LoadValue * load_value);

std::string visit_binary_value(const BinaryValue * binary_value);

std::string visit_store_value(const StoreValue * store_value);

std::string visit_call_value(const CallValue * call_value);

std::string visit_return_value(const ReturnValue * return_value);

std::string visit_branch_value(const BranchValue * branch_value);

std::string visit_jump_value(const JumpValue * jump_value);

#endif //VISIT_H
