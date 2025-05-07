/**
 * @file ast.c
 * @brief Abstract Syntax Tree implementation
 */

#include "ast/ast.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create a new AST
 */
AST *ast_create(void) {
  AST *ast = (AST *)safe_malloc(sizeof(AST));
  if (!ast) {
    return NULL;
  }

  ast->root = NULL;
  DEBUG_PRINT("Created new AST");
  return ast;
}

/**
 * @brief Helper function to destroy an AST node recursively
 */
static void destroy_ast_node(ASTNode *node) {
  if (!node) {
    return;
  }

  /* Free node-specific resources */
  switch (node->type) {
  case AST_PROGRAM:
    destroy_ast_node(node->program.statement_list);
    break;

  case AST_STATEMENT_LIST:
    destroy_ast_node(node->statement_list.statement);
    destroy_ast_node(node->statement_list.next);
    break;

  case AST_ASSIGN_STMT:
    if (node->assign_stmt.variable_name) {
      free(node->assign_stmt.variable_name);
    }
    destroy_ast_node(node->assign_stmt.expression);
    break;

  case AST_IF_STMT:
    destroy_ast_node(node->if_stmt.condition);
    destroy_ast_node(node->if_stmt.then_branch);
    destroy_ast_node(node->if_stmt.else_branch);
    break;

  case AST_WHILE_STMT:
    destroy_ast_node(node->while_stmt.condition);
    destroy_ast_node(node->while_stmt.body);
    break;

  case AST_BINARY_EXPR:
    destroy_ast_node(node->binary_expr.left);
    destroy_ast_node(node->binary_expr.right);
    break;

  case AST_VARIABLE:
    if (node->variable.name) {
      free(node->variable.name);
    }
    break;

  case AST_CONSTANT:
    /* No dynamic resources to free */
    break;
  }

  /* Free the node itself */
  free(node);
}

/**
 * @brief Destroy an AST and free resources
 */
void ast_destroy(AST *ast) {
  if (!ast) {
    return;
  }

  destroy_ast_node(ast->root);
  free(ast);
  DEBUG_PRINT("Destroyed AST");
}

/**
 * @brief Create a program node
 */
ASTNode *ast_create_program(ASTNode *statements) {
  ASTNode *node = (ASTNode *)safe_malloc(sizeof(ASTNode));
  if (!node) {
    return NULL;
  }

  node->type = AST_PROGRAM;
  node->program.statement_list = statements;

  DEBUG_PRINT("Created program node");
  return node;
}

/**
 * @brief Create a statement list node
 */
ASTNode *ast_create_statement_list(ASTNode *statement, ASTNode *next) {
  ASTNode *node = (ASTNode *)safe_malloc(sizeof(ASTNode));
  if (!node) {
    return NULL;
  }

  node->type = AST_STATEMENT_LIST;
  node->statement_list.statement = statement;
  node->statement_list.next = next;

  DEBUG_PRINT("Created statement list node");
  return node;
}

/**
 * @brief Create an assignment statement node
 */
ASTNode *ast_create_assign_stmt(const char *variable_name,
                                ASTNode *expression) {
  ASTNode *node = (ASTNode *)safe_malloc(sizeof(ASTNode));
  if (!node) {
    return NULL;
  }

  node->type = AST_ASSIGN_STMT;
  node->assign_stmt.variable_name = safe_strdup(variable_name);
  if (!node->assign_stmt.variable_name) {
    free(node);
    return NULL;
  }
  node->assign_stmt.expression = expression;

  DEBUG_PRINT("Created assignment statement node for variable: %s",
              variable_name);
  return node;
}

/**
 * @brief Create an if statement node
 */
ASTNode *ast_create_if_stmt(ASTNode *condition, ASTNode *then_branch,
                            ASTNode *else_branch) {
  ASTNode *node = (ASTNode *)safe_malloc(sizeof(ASTNode));
  if (!node) {
    return NULL;
  }

  node->type = AST_IF_STMT;
  node->if_stmt.condition = condition;
  node->if_stmt.then_branch = then_branch;
  node->if_stmt.else_branch = else_branch;

  DEBUG_PRINT("Created if statement node%s",
              else_branch ? " with else branch" : "");
  return node;
}

/**
 * @brief Create a while statement node
 */
ASTNode *ast_create_while_stmt(ASTNode *condition, ASTNode *body) {
  ASTNode *node = (ASTNode *)safe_malloc(sizeof(ASTNode));
  if (!node) {
    return NULL;
  }

  node->type = AST_WHILE_STMT;
  node->while_stmt.condition = condition;
  node->while_stmt.body = body;

  DEBUG_PRINT("Created while statement node");
  return node;
}

/**
 * @brief Create a binary expression node
 */
ASTNode *ast_create_binary_expr(BinaryOpType op, ASTNode *left,
                                ASTNode *right) {
  ASTNode *node = (ASTNode *)safe_malloc(sizeof(ASTNode));
  if (!node) {
    return NULL;
  }

  node->type = AST_BINARY_EXPR;
  node->binary_expr.op = op;
  node->binary_expr.left = left;
  node->binary_expr.right = right;

  const char *op_str = "";
  switch (op) {
  case OP_ADD:
    op_str = "+";
    break;
  case OP_SUB:
    op_str = "-";
    break;
  case OP_MUL:
    op_str = "*";
    break;
  case OP_DIV:
    op_str = "/";
    break;
  case OP_GT:
    op_str = ">";
    break;
  case OP_LT:
    op_str = "<";
    break;
  case OP_EQ:
    op_str = "=";
    break;
  }

  DEBUG_PRINT("Created binary expression node with operator: %s", op_str);
  return node;
}

/**
 * @brief Create a variable reference node
 */
ASTNode *ast_create_variable(const char *name) {
  ASTNode *node = (ASTNode *)safe_malloc(sizeof(ASTNode));
  if (!node) {
    return NULL;
  }

  node->type = AST_VARIABLE;
  node->variable.name = safe_strdup(name);
  if (!node->variable.name) {
    free(node);
    return NULL;
  }

  DEBUG_PRINT("Created variable node: %s", name);
  return node;
}

/**
 * @brief Create a constant value node
 */
ASTNode *ast_create_constant(int value, TokenType token_type) {
  ASTNode *node = (ASTNode *)safe_malloc(sizeof(ASTNode));
  if (!node) {
    return NULL;
  }

  node->type = AST_CONSTANT;
  node->constant.value = value;
  node->constant.token_type = token_type;

  const char *type_str = "";
  switch (token_type) {
  case TK_DEC:
    type_str = "decimal";
    break;
  case TK_OCT:
    type_str = "octal";
    break;
  case TK_HEX:
    type_str = "hexadecimal";
    break;
  default:
    type_str = "unknown";
    break;
  }

  DEBUG_PRINT("Created constant node: %d (%s)", value, type_str);
  return node;
}

/**
 * @brief Helper function to print an AST node with indentation
 */
static void print_ast_node(const ASTNode *node, int indent) {
  if (!node) {
    return;
  }

  /* Print indentation */
  for (int i = 0; i < indent; i++) {
    printf("  ");
  }

  /* Print node-specific information */
  switch (node->type) {
  case AST_PROGRAM:
    printf("Program\n");
    print_ast_node(node->program.statement_list, indent + 1);
    break;

  case AST_STATEMENT_LIST:
    printf("StatementList\n");
    print_ast_node(node->statement_list.statement, indent + 1);
    print_ast_node(node->statement_list.next, indent);
    break;

  case AST_ASSIGN_STMT:
    printf("AssignStmt: %s\n", node->assign_stmt.variable_name);
    print_ast_node(node->assign_stmt.expression, indent + 1);
    break;

  case AST_IF_STMT:
    printf("IfStmt\n");
    for (int i = 0; i < indent + 1; i++)
      printf("  ");
    printf("Condition:\n");
    print_ast_node(node->if_stmt.condition, indent + 2);
    for (int i = 0; i < indent + 1; i++)
      printf("  ");
    printf("Then:\n");
    print_ast_node(node->if_stmt.then_branch, indent + 2);
    if (node->if_stmt.else_branch) {
      for (int i = 0; i < indent + 1; i++)
        printf("  ");
      printf("Else:\n");
      print_ast_node(node->if_stmt.else_branch, indent + 2);
    }
    break;

  case AST_WHILE_STMT:
    printf("WhileStmt\n");
    for (int i = 0; i < indent + 1; i++)
      printf("  ");
    printf("Condition:\n");
    print_ast_node(node->while_stmt.condition, indent + 2);
    for (int i = 0; i < indent + 1; i++)
      printf("  ");
    printf("Body:\n");
    print_ast_node(node->while_stmt.body, indent + 2);
    break;

  case AST_BINARY_EXPR: {
    const char *op_str = "";
    switch (node->binary_expr.op) {
    case OP_ADD:
      op_str = "+";
      break;
    case OP_SUB:
      op_str = "-";
      break;
    case OP_MUL:
      op_str = "*";
      break;
    case OP_DIV:
      op_str = "/";
      break;
    case OP_GT:
      op_str = ">";
      break;
    case OP_LT:
      op_str = "<";
      break;
    case OP_EQ:
      op_str = "=";
      break;
    }
    printf("BinaryExpr: %s\n", op_str);
    for (int i = 0; i < indent + 1; i++)
      printf("  ");
    printf("Left:\n");
    print_ast_node(node->binary_expr.left, indent + 2);
    for (int i = 0; i < indent + 1; i++)
      printf("  ");
    printf("Right:\n");
    print_ast_node(node->binary_expr.right, indent + 2);
  } break;

  case AST_VARIABLE:
    printf("Variable: %s\n", node->variable.name);
    break;

  case AST_CONSTANT: {
    const char *type_str = "";
    switch (node->constant.token_type) {
    case TK_DEC:
      type_str = "decimal";
      break;
    case TK_OCT:
      type_str = "octal";
      break;
    case TK_HEX:
      type_str = "hexadecimal";
      break;
    default:
      type_str = "unknown";
      break;
    }
    printf("Constant: %d (%s)\n", node->constant.value, type_str);
  } break;
  }
}

/**
 * @brief Print the AST to stdout
 */
void ast_print(const AST *ast) {
  if (!ast) {
    printf("AST is NULL\n");
    return;
  }

  if (!ast->root) {
    printf("AST is empty\n");
    return;
  }

  printf("Abstract Syntax Tree:\n");
  print_ast_node(ast->root, 0);
}
