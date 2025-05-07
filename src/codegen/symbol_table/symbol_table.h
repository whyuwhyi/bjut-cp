/**
 * @file symbol_table.h
 * @brief Symbol table for code generation (internal)
 */

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>

/**
 * @brief Symbol type enumeration
 */
typedef enum {
  SYMBOL_VARIABLE,  /* Program variable */
  SYMBOL_TEMPORARY, /* Compiler-generated temporary */
  SYMBOL_CONSTANT,  /* Constant value */
  SYMBOL_LABEL      /* Label */
} SymbolType;

/**
 * @brief Symbol table entry
 */
typedef struct {
  char *name;      /* Symbol name */
  SymbolType type; /* Symbol type */
  char *value;     /* Symbol value (optional) */
} Symbol;

/**
 * @brief Symbol table scope
 */
typedef struct SymbolScope {
  Symbol **symbols;           /* Symbols in this scope */
  int symbol_count;           /* Number of symbols */
  int symbol_capacity;        /* Capacity of symbols array */
  struct SymbolScope *parent; /* Parent scope, or NULL for global scope */
} SymbolScope;

/**
 * @brief Symbol table
 */
typedef struct {
  SymbolScope *current_scope; /* Current active scope */
  int temp_counter;           /* Counter for generating temporary names */
} SymbolTable;

/**
 * @brief Create a new symbol table
 *
 * @return SymbolTable* Created symbol table, or NULL on failure
 */
SymbolTable *symbol_table_create(void);

/**
 * @brief Enter a new scope
 *
 * @param table Symbol table
 * @return bool Success status
 */
bool symbol_table_enter_scope(SymbolTable *table);

/**
 * @brief Exit the current scope
 *
 * @param table Symbol table
 * @return bool Success status
 */
bool symbol_table_exit_scope(SymbolTable *table);

/**
 * @brief Add a symbol to the current scope
 *
 * @param table Symbol table
 * @param name Symbol name
 * @param type Symbol type
 * @param value Symbol value (can be NULL)
 * @return Symbol* Added symbol, or NULL on failure
 */
Symbol *symbol_table_add(SymbolTable *table, const char *name, SymbolType type,
                         const char *value);

/**
 * @brief Look up a symbol in the symbol table
 *
 * @param table Symbol table
 * @param name Symbol name
 * @return Symbol* Found symbol, or NULL if not found
 */
Symbol *symbol_table_lookup(SymbolTable *table, const char *name);

/**
 * @brief Generate a new temporary variable name
 *
 * @param table Symbol table
 * @return char* Generated name, or NULL on failure
 */
char *symbol_table_new_temp(SymbolTable *table);

/**
 * @brief Free symbol table resources
 *
 * @param table Symbol table to destroy
 */
void symbol_table_destroy(SymbolTable *table);

#endif /* SYMBOL_TABLE_H */
