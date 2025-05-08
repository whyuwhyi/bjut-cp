/**
 * @file sdt_codegen.c
 * @brief Syntax-directed translation code generator implementation
 */

#include "codegen/sdt_codegen.h"
#include "codegen/sdt_attributes.h"
#include "label_manager/label_manager.h"
#include "parser/grammar.h"
#include "parser/parser.h"
#include "sdt/sdt_actions.h"
#include "sdt_internal.h"
#include "symbol_table/symbol_table.h"
#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create a new SDT code generator
 */
SDTCodeGen *sdt_codegen_create(void) {
  SDTCodeGen *gen = (SDTCodeGen *)safe_malloc(sizeof(SDTCodeGen));
  if (!gen) {
    return NULL;
  }

  /* Initialize fields */
  gen->program = tac_program_create();
  gen->symbol_table = symbol_table_create();
  gen->label_manager = label_manager_create();
  gen->curr_attr = NULL;
  gen->has_error = false;
  memset(gen->error_message, 0, sizeof(gen->error_message));

  if (!gen->program || !gen->symbol_table || !gen->label_manager) {
    sdt_codegen_destroy(gen);
    return NULL;
  }

  DEBUG_PRINT("Created SDT code generator");
  return gen;
}

/**
 * @brief Initialize SDT code generator
 */
bool sdt_codegen_init(SDTCodeGen *gen) {
  if (!gen) {
    return false;
  }

  /* Reset state */
  gen->has_error = false;
  memset(gen->error_message, 0, sizeof(gen->error_message));

  DEBUG_PRINT("Initialized SDT code generator");
  return true;
}

/**
 * @brief Generate three-address code during parsing
 */
TACProgram *sdt_codegen_generate(SDTCodeGen *gen, Parser *parser,
                                 Lexer *lexer) {
  if (!gen || !parser || !lexer) {
    return NULL;
  }

  /* Reset state */
  gen->has_error = false;
  memset(gen->error_message, 0, sizeof(gen->error_message));

  /* Connect code generator to parser */
  parser->sdt_gen = gen;

  /* Parse input and generate code on-the-fly */
  SyntaxTree *syntax_tree = parser_parse(parser, lexer);
  if (!syntax_tree) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Failed to parse input");
    return NULL;
  }

  /* Clean up */
  syntax_tree_destroy(syntax_tree);

  /* Return generated program */
  if (gen->has_error) {
    return NULL;
  }

  return gen->program;
}

/**
 * @brief Execute semantic action for a production
 */
bool sdt_execute_action(SDTCodeGen *gen, int production_id,
                        SyntaxTreeNode *node) {
  if (!gen || !node) {
    return false;
  }

  /* Delegate to the action handler */
  return sdt_execute_production_action(gen, production_id, node);
}

/**
 * @brief Get error message from the code generator
 */
const char *sdt_codegen_get_error(const SDTCodeGen *gen) {
  if (!gen || !gen->has_error) {
    return NULL;
  }

  return gen->error_message;
}

/**
 * @brief Free SDT code generator resources
 */
void sdt_codegen_destroy(SDTCodeGen *gen) {
  if (!gen) {
    return;
  }

  /* Free allocated resources */
  if (gen->program) {
    tac_program_destroy(gen->program);
  }

  if (gen->symbol_table) {
    symbol_table_destroy(gen->symbol_table);
  }

  if (gen->label_manager) {
    label_manager_destroy(gen->label_manager);
  }

  if (gen->curr_attr) {
    sdt_attributes_destroy(gen->curr_attr);
  }

  /* Free the generator itself */
  free(gen);

  DEBUG_PRINT("Destroyed SDT code generator");
}

/**
 * @brief Generate a new temporary variable
 */
char *sdt_new_temp(SDTCodeGen *gen) {
  if (!gen || !gen->symbol_table) {
    return NULL;
  }

  return symbol_table_new_temp(gen->symbol_table);
}

/**
 * @brief Generate a new label
 */
char *sdt_new_label(SDTCodeGen *gen) {
  if (!gen || !gen->label_manager) {
    return NULL;
  }

  return label_manager_new_label(gen->label_manager);
}

/**
 * @brief Generate three-address code instruction
 */
char *sdt_gen_code(SDTCodeGen *gen, const char *format, ...) {
  if (!gen || !format) {
    return NULL;
  }

  /* Format the instruction string */
  char buffer[1024];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  /* Return a copy of the formatted string */
  return safe_strdup(buffer);
}

/**
 * @brief Add instruction to the program
 */
bool sdt_add_instruction(SDTCodeGen *gen, TACOpType op, const char *result,
                         const char *arg1, const char *arg2, int lineno) {
  if (!gen || !gen->program) {
    return false;
  }

  int index =
      tac_program_add_inst(gen->program, op, result, arg1, arg2, lineno);
  return (index >= 0);
}

/**
 * @brief Set error message
 */
void sdt_set_error(SDTCodeGen *gen, const char *format, ...) {
  if (!gen) {
    return;
  }

  va_list args;
  va_start(args, format);
  vsnprintf(gen->error_message, sizeof(gen->error_message), format, args);
  va_end(args);

  gen->has_error = true;
  DEBUG_PRINT("SDT error: %s", gen->error_message);
}
