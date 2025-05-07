/**
 * @file symbol_table.c
 * @brief Symbol table for code generation implementation
 */

#include "symbol_table.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 64

/**
 * @brief Create a new symbol scope
 */
static SymbolScope *symbol_scope_create(SymbolScope *parent) {
  SymbolScope *scope = (SymbolScope *)safe_malloc(sizeof(SymbolScope));
  if (!scope) {
    return NULL;
  }

  scope->symbols = (Symbol **)safe_malloc(INITIAL_CAPACITY * sizeof(Symbol *));
  if (!scope->symbols) {
    free(scope);
    return NULL;
  }

  scope->symbol_count = 0;
  scope->symbol_capacity = INITIAL_CAPACITY;
  scope->parent = parent;

  DEBUG_PRINT("Created symbol scope");
  return scope;
}

/**
 * @brief Destroy a symbol scope
 */
static void symbol_scope_destroy(SymbolScope *scope) {
  if (!scope) {
    return;
  }

  /* Free all symbols */
  for (int i = 0; i < scope->symbol_count; i++) {
    Symbol *symbol = scope->symbols[i];

    if (symbol) {
      if (symbol->name)
        free(symbol->name);
      if (symbol->value)
        free(symbol->value);
      free(symbol);
    }
  }

  /* Free symbol array and scope */
  free(scope->symbols);
  free(scope);

  DEBUG_PRINT("Destroyed symbol scope");
}

/**
 * @brief Add a symbol to a scope
 */
static Symbol *symbol_scope_add(SymbolScope *scope, const char *name,
                                SymbolType type, const char *value) {
  if (!scope || !name) {
    return NULL;
  }

  /* Check if scope needs to be resized */
  if (scope->symbol_count >= scope->symbol_capacity) {
    int new_capacity = scope->symbol_capacity * 2;
    Symbol **new_symbols = (Symbol **)safe_realloc(
        scope->symbols, new_capacity * sizeof(Symbol *));
    if (!new_symbols) {
      return NULL;
    }

    scope->symbols = new_symbols;
    scope->symbol_capacity = new_capacity;
  }

  /* Create new symbol */
  Symbol *symbol = (Symbol *)safe_malloc(sizeof(Symbol));
  if (!symbol) {
    return NULL;
  }

  symbol->name = safe_strdup(name);
  if (!symbol->name) {
    free(symbol);
    return NULL;
  }

  symbol->type = type;
  symbol->value = value ? safe_strdup(value) : NULL;

  /* Add symbol to scope */
  scope->symbols[scope->symbol_count] = symbol;
  scope->symbol_count++;

  DEBUG_PRINT("Added symbol '%s' to scope", name);
  return symbol;
}

/**
 * @brief Look up a symbol in a scope
 */
static Symbol *symbol_scope_lookup(SymbolScope *scope, const char *name) {
  if (!scope || !name) {
    return NULL;
  }

  /* Search symbols in this scope */
  for (int i = 0; i < scope->symbol_count; i++) {
    Symbol *symbol = scope->symbols[i];

    if (symbol && symbol->name && strcmp(symbol->name, name) == 0) {
      return symbol;
    }
  }

  return NULL;
}

/**
 * @brief Create a new symbol table
 */
SymbolTable *symbol_table_create(void) {
  SymbolTable *table = (SymbolTable *)safe_malloc(sizeof(SymbolTable));
  if (!table) {
    return NULL;
  }

  /* Create global scope */
  table->current_scope = symbol_scope_create(NULL);
  if (!table->current_scope) {
    free(table);
    return NULL;
  }

  table->temp_counter = 0;

  DEBUG_PRINT("Created symbol table");
  return table;
}

/**
 * @brief Enter a new scope
 */
bool symbol_table_enter_scope(SymbolTable *table) {
  if (!table) {
    return false;
  }

  /* Create new scope with current scope as parent */
  SymbolScope *new_scope = symbol_scope_create(table->current_scope);
  if (!new_scope) {
    return false;
  }

  table->current_scope = new_scope;

  DEBUG_PRINT("Entered new scope");
  return true;
}

/**
 * @brief Exit the current scope
 */
bool symbol_table_exit_scope(SymbolTable *table) {
  if (!table || !table->current_scope || !table->current_scope->parent) {
    return false;
  }

  SymbolScope *parent = table->current_scope->parent;
  symbol_scope_destroy(table->current_scope);
  table->current_scope = parent;

  DEBUG_PRINT("Exited scope");
  return true;
}

/**
 * @brief Add a symbol to the current scope
 */
Symbol *symbol_table_add(SymbolTable *table, const char *name, SymbolType type,
                         const char *value) {
  if (!table || !table->current_scope) {
    return NULL;
  }

  return symbol_scope_add(table->current_scope, name, type, value);
}

/**
 * @brief Look up a symbol in the symbol table
 */
Symbol *symbol_table_lookup(SymbolTable *table, const char *name) {
  if (!table || !name) {
    return NULL;
  }

  /* Start with current scope */
  SymbolScope *scope = table->current_scope;

  /* Search up the scope chain */
  while (scope) {
    Symbol *symbol = symbol_scope_lookup(scope, name);

    if (symbol) {
      return symbol;
    }

    scope = scope->parent;
  }

  return NULL;
}

/**
 * @brief Generate a new temporary variable name
 */
char *symbol_table_new_temp(SymbolTable *table) {
  if (!table) {
    return NULL;
  }

  char temp_name[32];
  snprintf(temp_name, sizeof(temp_name), "t%d", table->temp_counter++);

  /* Add temporary to symbol table */
  Symbol *symbol = symbol_table_add(table, temp_name, SYMBOL_TEMPORARY, NULL);
  if (!symbol) {
    return NULL;
  }

  DEBUG_PRINT("Generated temporary variable '%s'", temp_name);
  return safe_strdup(temp_name);
}

/**
 * @brief Free symbol table resources
 */
void symbol_table_destroy(SymbolTable *table) {
  if (!table) {
    return;
  }

  /* Free scopes */
  SymbolScope *scope = table->current_scope;

  /* Clean up all nested scopes */
  while (scope) {
    SymbolScope *parent = scope->parent;
    symbol_scope_destroy(scope);
    scope = parent;
  }

  free(table);

  DEBUG_PRINT("Destroyed symbol table");
}
