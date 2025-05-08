/**
 * @file sdt_internal.h
 * @brief Internal definitions for SDT code generator
 */

#ifndef SDT_INTERNAL_H
#define SDT_INTERNAL_H

#include "../label_manager/label_manager.h"
#include "../symbol_table/symbol_table.h"
#include "codegen/tac.h"
#include "sdt_actions.h"
#include "sdt_attributes.h"
#include <stdbool.h>

typedef void (*SDTActionFunc)(struct SDTCodeGen *gen, SyntaxTreeNode *node);

/**
 * @brief SDT code generator structure
 *
 * This structure holds all the state needed for SDT code generation,
 * including the symbol table, label manager, and the generated TAC program.
 */
struct SDTCodeGen {
  TACProgram *program;         /* Generated TAC program */
  SymbolTable *symbol_table;   /* Symbol table for tracking variables */
  LabelManager *label_manager; /* Label manager for generating unique labels */

  /* Semantic action mapping table */
  SDTActionFunc *action_table; /* Maps production indexes to semantic actions */
  int action_table_size;       /* Size of the action table */

  /* Current attributes being processed */
  SDTAttributes *curr_attr; /* Current node's attributes */

  /* Error handling */
  bool has_error;           /* Error flag */
  char error_message[1024]; /* Detailed error message */
};

typedef struct SDTCodeGen SDTCodeGen;

/**
 * @brief Generate a new temporary variable
 *
 * @param gen Code generator
 * @return char* Temporary variable name, or NULL on failure
 */
char *sdt_new_temp(struct SDTCodeGen *gen);

/**
 * @brief Generate a new label
 *
 * @param gen Code generator
 * @return char* Label name, or NULL on failure
 */
char *sdt_new_label(struct SDTCodeGen *gen);

/**
 * @brief Generate three-address code instruction
 *
 * @param gen Code generator
 * @param format Format string for the instruction
 * @param ... Additional arguments for the format string
 * @return char* Generated code string, or NULL on failure
 */
char *sdt_gen_code(struct SDTCodeGen *gen, const char *format, ...);

/**
 * @brief Helper function to execute semantic action for a production
 *
 * This function maps production IDs to semantic action functions
 *
 * @param gen Code generator
 * @param production_id Production ID
 * @param node Syntax tree node
 * @return bool Success status
 */
bool sdt_execute_production_action(struct SDTCodeGen *gen, int production_id,
                                   SyntaxTreeNode *node);

/**
 * @brief Add instruction to the program
 *
 * @param gen Code generator
 * @param op Operation type
 * @param result Result operand
 * @param arg1 First argument
 * @param arg2 Second argument
 * @param lineno Line number
 * @return bool Success status
 */
bool sdt_add_instruction(struct SDTCodeGen *gen, TACOpType op,
                         const char *result, const char *arg1, const char *arg2,
                         int lineno);

/**
 * @brief Set error message
 *
 * @param gen Code generator
 * @param format Format string for the error message
 * @param ... Additional arguments for the format string
 */
void sdt_set_error(struct SDTCodeGen *gen, const char *format, ...);

#endif /* SDT_INTERNAL_H */
