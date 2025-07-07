#include "IR.h"
#include <cstddef>
#include <string>

std::string Int32Type::toString() const { return "i32"; }

std::string FunctionType::toString() const {
  std::string res = "(";
  for (size_t i = 0; i < param_types.size(); ++i) {
    if (i > 0)
      res += ", ";
    res += param_types[i]->toString();
  }
  res += "): ";
  res += return_type->toString();
  return res;
}

std::string UnitType::toString() const { return "()"; }
