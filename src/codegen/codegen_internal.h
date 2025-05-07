/**
 * @file codegen_internal.h
 * @brief Internal definitions for code generator
 */

#ifndef CODEGEN_INTERNAL_H
#define CODEGEN_INTERNAL_H

#include "codegen/codegen.h"
#include "codegen/tac.h"
#include "label_manager/label_manager.h"
#include "parser/syntax_tree.h"
#include "symbol_table/symbol_table.h"

/**
 * @brief Structure to track code generation attributes
 */
typedef struct {
  char *code;        /* Generated code */
  char *place;       /* Place to store the result */
  char *true_label;  /* True branch label (for conditions) */
  char *false_label; /* False branch label (for conditions) */
  char *next_label;  /* Next label (for control flow) */
  char *begin_label; /* Begin label (for loops) */
} CodeAttributes;

/**
 * @brief Code generator structure
 */
struct CodeGenerator {
  SymbolTable *symbol_table;   /* Symbol table */
  LabelManager *label_manager; /* Label manager */
  TACProgram *program;         /* Generated TAC program */

  /* Error handling */
  bool has_error;          /* Error flag */
  char error_message[256]; /* Error message */
};

/**
 * @brief Generate code for a program node
 *
 * @param gen Code generator
 * @param node Program node
 * @return bool Success status
 */
bool codegen_program(CodeGenerator *gen, SyntaxTreeNode *node);

/**
 * @brief Generate code for a statement list node
 *
 * @param gen Code generator
 * @param node Statement list node
 * @param attrs Code attributes
 * @return bool Success status
 */
bool codegen_stmt_list(CodeGenerator *gen, SyntaxTreeNode *node,
                       CodeAttributes *attrs);

/**
 * @brief Generate code for a statement node
 *
 * @param gen Code generator
 * @param node Statement node
 * @param attrs Code attributes
 * @return bool Success status
 */
bool codegen_stmt(CodeGenerator *gen, SyntaxTreeNode *node,
                  CodeAttributes *attrs);

/**
 * @brief Generate code for an assignment statement
 *
 * @param gen Code generator
 * @param node Assignment node
 * @param attrs Code attributes
 * @return bool Success status
 */
bool codegen_assignment(CodeGenerator *gen, SyntaxTreeNode *node,
                        CodeAttributes *attrs);

/**
 * @brief Generate code for an if statement
 *
 * @param gen Code generator
 * @param node If statement node
 * @param attrs Code attributes
 * @return bool Success status
 */
bool codegen_if_stmt(CodeGenerator *gen, SyntaxTreeNode *node,
                     CodeAttributes *attrs);

/**
 * @brief Generate code for an if-else statement
 *
 * @param gen Code generator
 * @param node If-else statement node
 * @param attrs Code attributes
 * @return bool Success status
 */
bool codegen_if_else_stmt(CodeGenerator *gen, SyntaxTreeNode *node,
                          CodeAttributes *attrs);

/**
 * @brief Generate code for a while statement
 *
 * @param gen Code generator
 * @param node While statement node
 * @param attrs Code attributes
 * @return bool Success status
 */
bool codegen_while_stmt(CodeGenerator *gen, SyntaxTreeNode *node,
                        CodeAttributes *attrs);

/**
 * @brief Generate code for a condition
 *
 * @param gen Code generator
 * @param node Condition node
 * @param attrs Code attributes
 * @return bool Success status
 */
bool codegen_condition(CodeGenerator *gen, SyntaxTreeNode *node,
                       CodeAttributes *attrs);

/**
 * @brief Generate code for an expression
 *
 * @param gen Code generator
 * @param node Expression node
 * @param attrs Code attributes
 * @return bool Success status
 */
bool codegen_expression(CodeGenerator *gen, SyntaxTreeNode *node,
                        CodeAttributes *attrs);

/**
 * @brief Generate code for a term
 *
 * @param gen Code generator
 * @param node Term node
 * @param attrs Code attributes
 * @return bool Success status
 */
bool codegen_term(CodeGenerator *gen, SyntaxTreeNode *node,
                  CodeAttributes *attrs);

/**
 * @brief Generate code for a factor
 *
 * @param gen Code generator
 * @param node Factor node
 * @param attrs Code attributes
 * @return bool Success status
 */
bool codegen_factor(CodeGenerator *gen, SyntaxTreeNode *node,
                    CodeAttributes *attrs);

/**
 * @brief Create a new code attributes structure
 *
 * @return CodeAttributes* Created attributes, or NULL on failure
 */
CodeAttributes *code_attributes_create(void);

/**
 * @brief Free code attributes resources
 *
 * @param attrs Code attributes to destroy
 */
void code_attributes_destroy(CodeAttributes *attrs);

#endif /* CODEGEN_INTERNAL_H */
