/** This is the implementation of symbol table.
 * structure:
 *   1. A sym_t_stack (means symbol table stack, vector in practice) to
 * represent current scope and it's parents: unit: pair fst: table number snd:
 * table
 *   2. A sym_t (means symbol table, unordered_map in practice) to represent
 * symbol--symbol_value bindings: unit: map_entry key: symbol ident value: sym_v
 */

#include "symtable.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <bits/ostream.tcc>

// "sym_t" means symbol table
// A table is an map with k: symbol, value: sym_v
using sym_t = std::unordered_map<std::string, std::shared_ptr<sym_v>>;

// A sym_t_stack is a stack of pair with fst table number, snd table.
using sym_t_stack = std::vector<std::pair<int, sym_t *>>;

// How many symbol tables
static int symbol_table_cnt = 0;

// symbol table stack
static sym_t_stack sym_table_stack;

// Enter a new scope
// push a new table to the stack and counter++
void enter_scope() {
  sym_t *ptr = new sym_t();
  sym_table_stack.push_back(std::make_pair(symbol_table_cnt, ptr));
  symbol_table_cnt++;
}

// exit current scope
// pop from the table stack
void exit_scope() {
  delete sym_table_stack.back().second;
  sym_table_stack.pop_back();
}

std::string get_scope_number() {
  return "SYM_TABLE_" + std::to_string(sym_table_stack.back().first) + "_";
}

// given a symbol, return a pair
// first: the table number where the sym_t belongs to
// second: the sym_t iterator
// if doesn't find out, return number -1
static std::pair<int, sym_t::iterator> find_iter(const std::string &symbol, int type) {
  for (auto iit = sym_table_stack.rbegin(); iit != sym_table_stack.rend();
       ++iit) {
    auto it = iit->second->find(symbol);

    if (it == iit->second->end()) { // means that doesn't find the symbol in the iit
      continue;
    }

    if (type == 0) { // variable
      if (it->second->type == SYM_TYPE_VAR) {
        return std::make_pair(iit->first, it);
      }
    } else if (type == 1) { // function
      if (it->second->type == SYM_TYPE_INTF || it->second->type == SYM_TYPE_VOIDF) {
        return std::make_pair(iit->first, it);
      }
    }
  }

  return std::make_pair(-1, sym_table_stack.back().second->end());
}

void insert_sym(const std::string &symbol, sym_type t, int v) {
  auto sym_val = new sym_v();
  sym_val->type = t;
  sym_val->value = v;

  sym_t *cur_scope = sym_table_stack.back().second;
  (*cur_scope)[symbol] = std::shared_ptr<sym_v>(sym_val);
}

int exist_sym(const std::string &symbol, int type) {
  int table_number;
  sym_t::iterator it;
  std::tie(table_number, it) = find_iter(symbol, type);

  return (table_number != -1);
}

int exist_sym_local(const std::string &symbol, int type) {
  auto iit = sym_table_stack.rbegin();
  if (iit == sym_table_stack.rend()) {
    return 0; // no scope
  }
  auto it = iit->second->find(symbol);
  if (it == iit->second->end()) {
    return 0; // not found
  }
  if (type == 0) {
    if (it->second->type == SYM_TYPE_VAR) {
      return 1;
    }
    return 0;
  }
  if (type == 1) {
    if (it->second->type == SYM_TYPE_INTF || it->second->type == SYM_TYPE_VOIDF) {
      return 1;
    }
    return 0;
  }

  return 1; // found
}

std::pair<std::string, std::shared_ptr<const sym_v>>
query_sym(const std::string &symbol, int type) {
  int table_number;
  sym_t::iterator it;
  std::tie(table_number, it) = find_iter(symbol, type);

  std::string str = "SYM_TABLE_" + std::to_string(table_number) + "_";

  if (table_number == -1) {
    auto sym_val = new sym_v();
    sym_val->type = SYM_TYPE_UND;
    sym_val->value = -1;
    return std::make_pair(str, std::shared_ptr<const sym_v>(sym_val));
  }

  return std::make_pair(str, std::shared_ptr<const sym_v>(it->second));
}
