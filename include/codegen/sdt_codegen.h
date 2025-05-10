/**
 * @file codegen/sdt_codegen.h
 * @brief Syntax-directed translation code generator interface
 */

#ifndef SDT_CODEGEN_H
#define SDT_CODEGEN_H

#include "codegen/tac.h"
#include "lexer/lexer.h"
#include "parser/syntax_tree.h"
#include <stdbool.h>

/**
 * @brief SDT code generator structure
 *
 * This structure holds all the state needed for SDT code generation,
 * including the symbol table, label manager, and the generated TAC program.
 */
typedef struct SDTCodeGen {
  struct TACProgram *program;       /* Generated three-address code program */
  struct SymbolTable *symbol_table; /* Symbol table for tracking variables */
  struct LabelManager
      *label_manager; /* Label manager for generating unique labels */

  /* Current attributes being processed */
  struct SDTAttributes *curr_attr; /* Current node's attributes */

  /* Error handling */
  bool has_error;           /* Error flag */
  char error_message[1024]; /* Detailed error message */
} SDTCodeGen;

/**
 * @brief Create a new SDT code generator
 *
 * @return SDTCodeGen* Created code generator, or NULL on failure
 */
SDTCodeGen *sdt_codegen_create(void);

/**
 * @brief Initialize SDT code generator
 *
 * @param gen Code generator to initialize
 * @return bool Success status
 */
bool sdt_codegen_init(SDTCodeGen *gen);

/**
 * @brief Generate three-address code for a syntax tree
 *
 * @param gen Initialized code generator
 * @param node Root of the syntax tree
 */
void sdt_codegen_generate(SDTCodeGen *gen, SyntaxTreeNode *node);

/**
 * @brief Get error message from the code generator
 *
 * @param gen Code generator
 * @return const char* Error message, or NULL if no error occurred
 */
const char *sdt_codegen_get_error(const SDTCodeGen *gen);

/**
 * @brief Free SDT code generator resources
 *
 * @param gen Code generator to destroy
 */
void sdt_codegen_destroy(SDTCodeGen *gen);

/**
 * @brief Generate a new temporary variable
 *
 * @param gen Code generator
 * @return char* Temporary variable name, or NULL on failure
 */
char *sdt_new_temp(SDTCodeGen *gen);

/**
 * @brief Generate a new label
 *
 * @param gen Code generator
 * @return char* Label name, or NULL on failure
 */
char *sdt_new_label(SDTCodeGen *gen);

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
bool sdt_add_instruction(SDTCodeGen *gen, TACOpType op, const char *result,
                         const char *arg1, const char *arg2, int lineno);

/**
 * @brief Set error message
 *
 * @param gen Code generator
 * @param format Format string for the error message
 * @param ... Additional arguments for the format string
 */
void sdt_set_error(SDTCodeGen *gen, const char *format, ...);

#endif /* SDT_CODEGEN_H */
