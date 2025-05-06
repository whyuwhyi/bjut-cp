/**
 * @file ast.h
 * @brief Abstract Syntax Tree definitions
 */

#ifndef AST_H
#define AST_H

#include "common.h"
#include "lexer-analyzer/token.h"
#include <stdbool.h>

/**
 * @brief AST node types
 */
typedef enum {
  AST_PROGRAM,        /* Program node */
  AST_STATEMENT_LIST, /* List of statements */
  AST_ASSIGN_STMT,    /* Assignment statement */
  AST_IF_STMT,        /* If statement */
  AST_WHILE_STMT,     /* While statement */
  AST_BINARY_EXPR,    /* Binary expression */
  AST_VARIABLE,       /* Variable reference */
  AST_CONSTANT        /* Constant value */
} ASTNodeType;

/**
 * @brief Binary operator types
 */
typedef enum {
  OP_ADD, /* Addition (+) */
  OP_SUB, /* Subtraction (-) */
  OP_MUL, /* Multiplication (*) */
  OP_DIV, /* Division (/) */
  OP_GT,  /* Greater than (>) */
  OP_LT,  /* Less than (<) */
  OP_EQ   /* Equal (=) */
} BinaryOpType;

/**
 * @brief Abstract syntax tree node
 */
typedef struct ASTNode ASTNode;

/**
 * @brief Abstract syntax tree
 */
typedef struct {
  ASTNode *root; /* Root node of the AST */
} AST;

/**
 * @brief Create a new AST
 *
 * @return AST* Newly created AST (must be freed with ast_destroy)
 */
AST *ast_create(void);

/**
 * @brief Destroy an AST and free resources
 *
 * @param ast AST to destroy
 */
void ast_destroy(AST *ast);

/**
 * @brief Create a program node
 *
 * @param statements Statement list node
 * @return ASTNode* Created node
 */
ASTNode *ast_create_program(ASTNode *statements);

/**
 * @brief Create a statement list node
 *
 * @param statement First statement
 * @param next Next statement list (can be NULL)
 * @return ASTNode* Created node
 */
ASTNode *ast_create_statement_list(ASTNode *statement, ASTNode *next);

/**
 * @brief Create an assignment statement node
 *
 * @param variable_name Variable name
 * @param expression Expression to assign
 * @return ASTNode* Created node
 */
ASTNode *ast_create_assign_stmt(const char *variable_name, ASTNode *expression);

/**
 * @brief Create an if statement node
 *
 * @param condition Condition expression
 * @param then_branch Then branch statements
 * @param else_branch Else branch statements (can be NULL)
 * @return ASTNode* Created node
 */
ASTNode *ast_create_if_stmt(ASTNode *condition, ASTNode *then_branch,
                            ASTNode *else_branch);

/**
 * @brief Create a while statement node
 *
 * @param condition Condition expression
 * @param body Loop body statements
 * @return ASTNode* Created node
 */
ASTNode *ast_create_while_stmt(ASTNode *condition, ASTNode *body);

/**
 * @brief Create a binary expression node
 *
 * @param op Operator type
 * @param left Left operand
 * @param right Right operand
 * @return ASTNode* Created node
 */
ASTNode *ast_create_binary_expr(BinaryOpType op, ASTNode *left, ASTNode *right);

/**
 * @brief Create a variable reference node
 *
 * @param name Variable name
 * @return ASTNode* Created node
 */
ASTNode *ast_create_variable(const char *name);

/**
 * @brief Create a constant value node
 *
 * @param value Constant value
 * @param token_type Token type (for determining the base)
 * @return ASTNode* Created node
 */
ASTNode *ast_create_constant(int value, TokenType token_type);

/**
 * @brief Print the AST to stdout (for debugging)
 *
 * @param ast AST to print
 */
void ast_print(const AST *ast);

#endif /* AST_H */
