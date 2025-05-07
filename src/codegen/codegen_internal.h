/**
 * @file codegen_internal.h
 * @brief Internal declarations for code generator
 *
 * This file contains the internal structures and function declarations
 * used by the code generator component. It defines the interfaces between
 * the code generator and other components like the AST and symbol table.
 */

#ifndef CODEGEN_INTERNAL_H
#define CODEGEN_INTERNAL_H

#include "ast/ast.h"
#include "codegen/codegen.h"
#include "codegen/tac.h"
#include "label_manager/label_manager.h"
#include "parser/syntax_tree.h"
#include "symbol_table/symbol_table.h"
#include <stdbool.h>

/**
 * @brief Code generator structure
 *
 * This structure contains all the state needed by the code generator,
 * including references to the symbol table, label manager, and the
 * generated TAC program. It also includes error handling fields.
 */
struct CodeGenerator {
  SymbolTable *symbol_table;   /* Symbol table for tracking variables */
  LabelManager *label_manager; /* Label manager for generating unique labels */
  TACProgram *program;         /* Generated TAC program output */

  /* Error handling */
  bool has_error;           /* Error flag indicating if an error occurred */
  char error_message[1024]; /* Detailed error message */
};

/**
 * @brief Code attributes for expression processing
 *
 * This structure is used to pass information between code generation
 * functions, particularly for expressions, conditions, and control flow.
 * It tracks temporary names, labels, and generated code segments.
 */
typedef struct {
  char *code;        /* Generated code string */
  char *place;       /* Place where result is stored (variable or temp) */
  char *true_label;  /* True label for conditional jumps */
  char *false_label; /* False label for conditional jumps */
  char *next_label;  /* Next label for control flow (after statements) */
  char *begin_label; /* Begin label for loops (jump back point) */
} CodeAttributes;

/**
 * @brief Create a new code attributes structure
 *
 * Allocates and initializes a new code attributes structure with all
 * fields set to NULL. This function is used to prepare for code generation
 * of expressions and statements.
 *
 * @return CodeAttributes* Created attributes or NULL on failure
 */
CodeAttributes *code_attributes_create(void);

/**
 * @brief Free code attributes resources
 *
 * Releases all resources associated with a code attributes structure,
 * including all dynamically allocated strings within it.
 *
 * @param attrs Attributes to free
 */
void code_attributes_destroy(CodeAttributes *attrs);

/*
 * Syntax tree-based code generation functions (legacy)
 * These functions work directly with the concrete syntax tree nodes
 * produced by the parser. They're maintained for backward compatibility.
 */

/**
 * @brief Generate code for a program node
 *
 * Processes the root node of a syntax tree representing a complete program
 * and generates the corresponding three-address code.
 *
 * @param gen Code generator state
 * @param node Root program node from syntax tree
 * @return bool Success status
 */
bool codegen_program(CodeGenerator *gen, SyntaxTreeNode *node);

/**
 * @brief Generate code for a statement node
 *
 * Processes a statement node in the syntax tree and delegates to the
 * appropriate specialized function based on statement type.
 *
 * @param gen Code generator state
 * @param node Statement node from syntax tree
 * @param attrs Code attributes for context
 * @return bool Success status
 */
bool codegen_stmt(CodeGenerator *gen, SyntaxTreeNode *node,
                  CodeAttributes *attrs);

/**
 * @brief Generate code for an assignment statement
 *
 * Processes an assignment statement node in the syntax tree,
 * generating code to evaluate the expression and store it in the variable.
 *
 * @param gen Code generator state
 * @param node Assignment statement node from syntax tree
 * @param attrs Code attributes for context
 * @return bool Success status
 */
bool codegen_assignment(CodeGenerator *gen, SyntaxTreeNode *node,
                        CodeAttributes *attrs);

/**
 * @brief Generate code for an if statement
 *
 * Processes an if statement node (without else clause) in the syntax tree,
 * generating code for conditional evaluation and the then branch.
 *
 * @param gen Code generator state
 * @param node If statement node from syntax tree
 * @param attrs Code attributes for context
 * @return bool Success status
 */
bool codegen_if_stmt(CodeGenerator *gen, SyntaxTreeNode *node,
                     CodeAttributes *attrs);

/**
 * @brief Generate code for an if-else statement
 *
 * Processes an if-else statement node in the syntax tree,
 * generating code for conditional evaluation and both branches.
 *
 * @param gen Code generator state
 * @param node If-else statement node from syntax tree
 * @param attrs Code attributes for context
 * @return bool Success status
 */
bool codegen_if_else_stmt(CodeGenerator *gen, SyntaxTreeNode *node,
                          CodeAttributes *attrs);

/**
 * @brief Generate code for a while statement
 *
 * Processes a while statement node in the syntax tree,
 * generating code for loop condition and body with appropriate labels.
 *
 * @param gen Code generator state
 * @param node While statement node from syntax tree
 * @param attrs Code attributes for context
 * @return bool Success status
 */
bool codegen_while_stmt(CodeGenerator *gen, SyntaxTreeNode *node,
                        CodeAttributes *attrs);

/**
 * @brief Generate code for a condition
 *
 * Processes a condition node in the syntax tree,
 * generating code for conditional evaluation with true/false branch labels.
 *
 * @param gen Code generator state
 * @param node Condition node from syntax tree
 * @param attrs Code attributes for context (including true/false labels)
 * @return bool Success status
 */
bool codegen_condition(CodeGenerator *gen, SyntaxTreeNode *node,
                       CodeAttributes *attrs);

/**
 * @brief Generate code for an expression
 *
 * Processes an expression node in the syntax tree,
 * generating code to evaluate the expression and store the result.
 *
 * @param gen Code generator state
 * @param node Expression node from syntax tree
 * @param attrs Code attributes for context
 * @return bool Success status
 */
bool codegen_expression(CodeGenerator *gen, SyntaxTreeNode *node,
                        CodeAttributes *attrs);

/**
 * @brief Generate code for a term
 *
 * Processes a term node in the syntax tree,
 * typically handling multiplication and division operations.
 *
 * @param gen Code generator state
 * @param node Term node from syntax tree
 * @param attrs Code attributes for context
 * @return bool Success status
 */
bool codegen_term(CodeGenerator *gen, SyntaxTreeNode *node,
                  CodeAttributes *attrs);

/**
 * @brief Generate code for a factor
 *
 * Processes a factor node in the syntax tree,
 * handling variables, constants, and parenthesized expressions.
 *
 * @param gen Code generator state
 * @param node Factor node from syntax tree
 * @param attrs Code attributes for context
 * @return bool Success status
 */
bool codegen_factor(CodeGenerator *gen, SyntaxTreeNode *node,
                    CodeAttributes *attrs);

/*
 * AST-based code generation functions
 * These functions work with the abstract syntax tree nodes that represent
 * the semantic structure of the program, independent of the concrete syntax.
 */

/**
 * @brief Generate code for a program node from AST
 *
 * Processes the root node of an abstract syntax tree representing a complete
 * program and generates the corresponding three-address code.
 *
 * @param gen Code generator state
 * @param node Root program node from AST
 * @return bool Success status
 */
bool codegen_program_ast(CodeGenerator *gen, ASTNode *node);

/**
 * @brief Generate code for a statement from AST
 *
 * Processes a statement node in the AST and delegates to the
 * appropriate specialized function based on statement type.
 *
 * @param gen Code generator state
 * @param node Statement node from AST
 * @param attrs Code attributes for context
 * @return bool Success status
 */
bool codegen_stmt_ast(CodeGenerator *gen, ASTNode *node, CodeAttributes *attrs);

/**
 * @brief Generate code for an assignment statement from AST
 *
 * Processes an assignment statement node in the AST,
 * generating code to evaluate the expression and store it in the variable.
 *
 * @param gen Code generator state
 * @param node Assignment statement node from AST
 * @param attrs Code attributes for context
 * @return bool Success status
 */
bool codegen_assignment_ast(CodeGenerator *gen, ASTNode *node,
                            CodeAttributes *attrs);

/**
 * @brief Generate code for an if statement from AST
 *
 * Processes an if statement node in the AST, generating code for
 * conditional evaluation and both then and else branches (if present).
 *
 * @param gen Code generator state
 * @param node If statement node from AST
 * @param attrs Code attributes for context
 * @return bool Success status
 */
bool codegen_if_stmt_ast(CodeGenerator *gen, ASTNode *node,
                         CodeAttributes *attrs);

/**
 * @brief Generate code for a while statement from AST
 *
 * Processes a while statement node in the AST,
 * generating code for loop condition and body with appropriate labels.
 *
 * @param gen Code generator state
 * @param node While statement node from AST
 * @param attrs Code attributes for context
 * @return bool Success status
 */
bool codegen_while_stmt_ast(CodeGenerator *gen, ASTNode *node,
                            CodeAttributes *attrs);

/**
 * @brief Generate code for a condition from AST
 *
 * Processes a condition node in the AST,
 * generating code for conditional evaluation with true/false branch labels.
 *
 * @param gen Code generator state
 * @param node Condition node from AST
 * @param attrs Code attributes for context (including true/false labels)
 * @return bool Success status
 */
bool codegen_condition_ast(CodeGenerator *gen, ASTNode *node,
                           CodeAttributes *attrs);

/**
 * @brief Generate code for an expression from AST
 *
 * Processes an expression node in the AST,
 * generating code to evaluate the expression and store the result.
 *
 * @param gen Code generator state
 * @param node Expression node from AST
 * @param attrs Code attributes for context
 * @return bool Success status
 */
bool codegen_expression_ast(CodeGenerator *gen, ASTNode *node,
                            CodeAttributes *attrs);

#endif /* CODEGEN_INTERNAL_H */
