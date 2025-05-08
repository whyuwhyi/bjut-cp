/**
 * @file symbol_table.h
 * @brief Symbol table for code generation
 */

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>

/**
 * @brief Types of symbols in the table
 */
typedef enum SymbolTableEntryType {
  SYM_VARIABLE,  /* User-defined variable */
  SYM_TEMPORARY, /* Compiler-generated temporary */
  SYM_CONSTANT   /* Constant value */
} SymbolTableEntryType;

/**
 * @brief Symbol table entry
 */
typedef struct SymbolTableEntry {
  char *name;                /* Symbol name */
  SymbolTableEntryType type; /* Symbol type */
  int value;                 /* Value for constants */
  int size;                  /* Size in bytes */
  int offset;                /* Memory offset */
  bool initialized;          /* Initialization status */
} SymbolTableEntry;

/**
 * @brief Symbol table structure
 */
typedef struct SymbolTable {
  SymbolTableEntry *entries; /* Array of symbols */
  int count;                 /* Number of symbols */
  int capacity;              /* Capacity of the symbol array */
  int temp_count;            /* Counter for generating temporaries */
} SymbolTable;

/**
 * @brief Create a new symbol table
 *
 * @return SymbolTable* New symbol table, or NULL on failure
 */
SymbolTable *symbol_table_create(void);

/**
 * @brief Add a variable to the symbol table
 *
 * @param table Symbol table
 * @param name Variable name
 * @param size Size in bytes
 * @return bool Success status
 */
bool symbol_table_add_variable(SymbolTable *table, const char *name, int size);

/**
 * @brief Add a constant to the symbol table
 *
 * @param table Symbol table
 * @param name Constant name
 * @param value Constant value
 * @return bool Success status
 */
bool symbol_table_add_constant(SymbolTable *table, const char *name, int value);

/**
 * @brief Generate a new temporary variable
 *
 * @param table Symbol table
 * @return char* Name of the temporary, or NULL on failure
 */
char *symbol_table_new_temp(SymbolTable *table);

/**
 * @brief Look up a symbol by name
 *
 * @param table Symbol table
 * @param name Symbol name
 * @return SymbolTableEntry* Found symbol, or NULL if not found
 */
SymbolTableEntry *symbol_table_lookup(SymbolTable *table, const char *name);

/**
 * @brief Print the symbol table contents
 *
 * @param table Symbol table
 */
void symbol_table_print(const SymbolTable *table);

/**
 * @brief Destroy a symbol table and free resources
 *
 * @param table Symbol table to destroy
 */
void symbol_table_destroy(SymbolTable *table);

#endif /* SYMBOL_TABLE_H */
