/**
 * @file symbol_table.c
 * @brief Symbol table implementation for code generation
 */

#include "symbol_table.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Initial capacity for the symbol table */
#define INITIAL_CAPACITY 16

/**
 * @brief Create a new symbol table
 */
SymbolTable *symbol_table_create(void) {
  SymbolTable *table = (SymbolTable *)safe_malloc(sizeof(SymbolTable));
  if (!table) {
    return NULL;
  }

  /* Initialize fields */
  table->entries = (SymbolTableEntry *)safe_malloc(INITIAL_CAPACITY *
                                                   sizeof(SymbolTableEntry));
  if (!table->entries) {
    free(table);
    return NULL;
  }

  table->count = 0;
  table->capacity = INITIAL_CAPACITY;
  table->temp_count = 0;

  DEBUG_PRINT("Created symbol table with capacity %d", table->capacity);
  return table;
}

/**
 * @brief Ensure the symbol table has enough capacity
 */
static bool ensure_capacity(SymbolTable *table) {
  if (table->count >= table->capacity) {
    /* Grow the table */
    int new_capacity = table->capacity * 2;
    SymbolTableEntry *new_entries = (SymbolTableEntry *)safe_realloc(
        table->entries, new_capacity * sizeof(SymbolTableEntry));
    if (!new_entries) {
      return false;
    }

    table->entries = new_entries;
    table->capacity = new_capacity;
    DEBUG_PRINT("Expanded symbol table to capacity %d", table->capacity);
  }

  return true;
}

/**
 * @brief Add a variable to the symbol table
 */
bool symbol_table_add_variable(SymbolTable *table, const char *name, int size) {
  if (!table || !name) {
    return false;
  }

  /* Check if variable already exists */
  for (int i = 0; i < table->count; i++) {
    if (strcmp(table->entries[i].name, name) == 0) {
      return false; /* Already exists */
    }
  }

  /* Ensure capacity */
  if (!ensure_capacity(table)) {
    return false;
  }

  /* Add the variable */
  SymbolTableEntry *entry = &table->entries[table->count++];
  entry->name = safe_strdup(name);
  entry->type = SYM_VARIABLE;
  entry->size = size;
  entry->offset = 0; /* Will be computed later */
  entry->initialized = false;

  DEBUG_PRINT("Added variable %s to symbol table", name);
  return true;
}

/**
 * @brief Add a constant to the symbol table
 */
bool symbol_table_add_constant(SymbolTable *table, const char *name,
                               int value) {
  if (!table || !name) {
    return false;
  }

  /* Check if constant already exists */
  for (int i = 0; i < table->count; i++) {
    if (strcmp(table->entries[i].name, name) == 0) {
      return false; /* Already exists */
    }
  }

  /* Ensure capacity */
  if (!ensure_capacity(table)) {
    return false;
  }

  /* Add the constant */
  SymbolTableEntry *entry = &table->entries[table->count++];
  entry->name = safe_strdup(name);
  entry->type = SYM_CONSTANT;
  entry->value = value;
  entry->size = sizeof(int);
  entry->offset = 0; /* Constants don't have offsets */
  entry->initialized = true;

  DEBUG_PRINT("Added constant %s with value %d to symbol table", name, value);
  return true;
}

/**
 * @brief Generate a new temporary variable
 */
char *symbol_table_new_temp(SymbolTable *table) {
  if (!table) {
    return NULL;
  }

  /* Generate name */
  char name[32];
  snprintf(name, sizeof(name), "t%d", table->temp_count++);

  /* Ensure capacity */
  if (!ensure_capacity(table)) {
    return NULL;
  }

  /* Add the temporary */
  SymbolTableEntry *entry = &table->entries[table->count++];
  entry->name = safe_strdup(name);
  entry->type = SYM_TEMPORARY;
  entry->size = sizeof(int); /* Assume int for temporaries */
  entry->offset = 0;         /* Will be computed later */
  entry->initialized = false;

  DEBUG_PRINT("Added temporary %s to symbol table", name);
  return safe_strdup(name);
}

/**
 * @brief Look up a symbol by name
 */
SymbolTableEntry *symbol_table_lookup(SymbolTable *table, const char *name) {
  if (!table || !name) {
    return NULL;
  }

  /* Linear search for the symbol */
  for (int i = 0; i < table->count; i++) {
    if (strcmp(table->entries[i].name, name) == 0) {
      return &table->entries[i];
    }
  }

  return NULL; /* Not found */
}

/**
 * @brief Print the symbol table contents
 */
void symbol_table_print(const SymbolTable *table) {
  if (!table) {
    return;
  }

  printf("Symbol Table (%d entries):\n", table->count);
  printf("-----------------------------\n");
  printf("%-10s %-10s %-8s %-8s %-8s\n", "Name", "Type", "Value", "Size",
         "Offset");
  printf("-----------------------------\n");

  for (int i = 0; i < table->count; i++) {
    const SymbolTableEntry *entry = &table->entries[i];
    printf("%-10s ", entry->name);

    switch (entry->type) {
    case SYM_VARIABLE:
      printf("%-10s ", "Variable");
      printf("%-8s ", "-");
      break;
    case SYM_TEMPORARY:
      printf("%-10s ", "Temporary");
      printf("%-8s ", "-");
      break;
    case SYM_CONSTANT:
      printf("%-10s ", "Constant");
      printf("%-8d ", entry->value);
      break;
    }

    printf("%-8d %-8d\n", entry->size, entry->offset);
  }
  printf("-----------------------------\n");
}

/**
 * @brief Destroy a symbol table and free resources
 */
void symbol_table_destroy(SymbolTable *table) {
  if (!table) {
    return;
  }

  /* Free all symbol names */
  for (int i = 0; i < table->count; i++) {
    free(table->entries[i].name);
  }

  /* Free the table itself */
  free(table->entries);
  free(table);

  DEBUG_PRINT("Destroyed symbol table");
}
