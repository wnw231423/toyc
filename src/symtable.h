#ifndef SYMTABLE_H
#define SYMTABLE_H
#include <memory>
#include <string>

enum sym_type {
  SYM_TYPE_VAR,   // variable
  SYM_TYPE_VOIDF, // void function
  SYM_TYPE_INTF,  // int function
  SYM_TYPE_UND    // symbol doesn't exist
};

// A symbol entry includes
// sym_k (symbol), key, string
// sym_v, value, struct sym_v
struct sym_v {
  sym_type type;
  int value;
};

void enter_scope();

void exit_scope();

// insert an entry.
void insert_sym(const std::string &symbol, sym_type type, int value);

// check if a symbol exist. 0 for false, 1 for true.
int exist_sym(const std::string &symbol);

// get table number in string form
// e.g. "SYM_TABLE_42_"
std::string get_scope_number();

// given a symbol, return a pair which includes
// fst: symbol number, string
// snd: sym_v
std::pair<std::string, std::shared_ptr<const sym_v>>
query_sym(const std::string &symbol);

#endif // !SYMTABLE_H
