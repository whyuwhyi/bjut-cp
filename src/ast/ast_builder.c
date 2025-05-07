/**
 * @file ast_builder.c
 * @brief AST Builder implementation
 */

#include "ast/ast_builder.h"
#include "parser/grammar.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Build an AST from a syntax tree
 */
AST *ast_builder_build(const SyntaxTree *tree) {
  if (!tree || !tree->root) {
    return NULL;
  }

  AST *ast = ast_create();
  if (!ast) {
    return NULL;
  }

  ast->root = ast_builder_build_node(tree->root);
  if (!ast->root) {
    ast_destroy(ast);
    return NULL;
  }

  DEBUG_PRINT("Built AST from syntax tree");
  return ast;
}

/**
 * @brief Extract token value from node
 */
static const char *get_token_string_value(const SyntaxTreeNode *node) {
  if (!node || node->type != NODE_TERMINAL || node->token.type != TK_IDN) {
    return NULL;
  }
  return node->token.str_val;
}

/**
 * @brief Extract token numeric value from node
 */
static int get_token_numeric_value(const SyntaxTreeNode *node) {
  if (!node || node->type != NODE_TERMINAL) {
    return 0;
  }
  return node->token.num_val;
}

/**
 * @brief Map token type to binary operator type
 */
static BinaryOpType get_binary_op_type(TokenType token_type) {
  switch (token_type) {
  case TK_ADD:
    return OP_ADD;
  case TK_SUB:
    return OP_SUB;
  case TK_MUL:
    return OP_MUL;
  case TK_DIV:
    return OP_DIV;
  case TK_GT:
    return OP_GT;
  case TK_LT:
    return OP_LT;
  case TK_EQ:
    return OP_EQ;
  default:
    return OP_ADD; // Default
  }
}

/**
 * @brief Find child node by production ID
 */
static SyntaxTreeNode *find_child_with_production(const SyntaxTreeNode *node,
                                                  int prod_id) {
  if (!node || !node->first_child) {
    return NULL;
  }

  SyntaxTreeNode *child = node->first_child;
  while (child) {
    if (child->type == NODE_NONTERMINAL && child->production_id == prod_id) {
      return child;
    }
    child = child->next_sibling;
  }
  return NULL;
}

/**
 * @brief Find terminal child node by token type
 */
static SyntaxTreeNode *find_terminal_child(const SyntaxTreeNode *node,
                                           TokenType token_type) {
  if (!node || !node->first_child) {
    return NULL;
  }

  SyntaxTreeNode *child = node->first_child;
  while (child) {
    if (child->type == NODE_TERMINAL && child->token.type == token_type) {
      return child;
    }
    child = child->next_sibling;
  }
  return NULL;
}

/**
 * @brief Process assignment statement
 */
static ASTNode *process_assignment(const SyntaxTreeNode *node) {
  if (!node)
    return NULL;

  // For assignment, we need to find the identifier and expression
  SyntaxTreeNode *id_node = find_terminal_child(node, TK_IDN);
  if (!id_node)
    return NULL;

  // Find expression node - typically the third child after ID and equals
  SyntaxTreeNode *expr_node = NULL;
  SyntaxTreeNode *child = node->first_child;
  int count = 0;

  while (child && count < 2) {
    child = child->next_sibling;
    count++;
  }
  expr_node = child;

  if (!expr_node)
    return NULL;

  // Get variable name
  const char *var_name = get_token_string_value(id_node);
  if (!var_name)
    return NULL;

  // Process expression
  ASTNode *expr = ast_builder_build_node(expr_node);
  if (!expr)
    return NULL;

  return ast_create_assign_stmt(var_name, expr);
}

/**
 * @brief Process binary expression
 */
static ASTNode *process_binary_expr(const SyntaxTreeNode *node,
                                    TokenType op_token) {
  if (!node)
    return NULL;

  // Binary expressions typically have left operand, operator, and right operand
  // The structure depends on your grammar
  SyntaxTreeNode *left_node = node->first_child;
  SyntaxTreeNode *right_node = NULL;

  // Find operator node
  SyntaxTreeNode *op_node = find_terminal_child(node, op_token);
  if (!op_node) {
    // If op not found directly, it might be in a different structure
    // Try to navigate based on your specific grammar structure
    SyntaxTreeNode *temp = left_node;
    while (temp && temp->next_sibling) {
      if (temp->next_sibling->type == NODE_TERMINAL &&
          temp->next_sibling->token.type == op_token) {
        op_node = temp->next_sibling;
        right_node = op_node->next_sibling;
        break;
      }
      temp = temp->next_sibling;
    }
  } else {
    // If op found directly, right node might be after it
    right_node = op_node->next_sibling;
  }

  if (!left_node || !op_node || !right_node)
    return NULL;

  // Process operands
  ASTNode *left = ast_builder_build_node(left_node);
  if (!left)
    return NULL;

  ASTNode *right = ast_builder_build_node(right_node);
  if (!right) {
    // Clean up left node
    // This assumes your AST nodes can be freed individually
    return NULL;
  }

  // Create binary expression
  BinaryOpType op_type = get_binary_op_type(op_node->token.type);
  return ast_create_binary_expr(op_type, left, right);
}

/**
 * @brief Helper to build an AST node from a syntax tree node
 *
 * This is a simplified implementation. You'll need to adjust based on your
 * specific grammar and syntax tree structure.
 */
ASTNode *ast_builder_build_node(const SyntaxTreeNode *node) {
  if (!node)
    return NULL;

  switch (node->type) {
  case NODE_NONTERMINAL: {
    // Handle non-terminal node based on grammar symbol
    switch (node->nonterminal_id) {
    // Program node
    case NT_P:
      return ast_create_program(ast_builder_build_node(node->first_child));

    // Statement list
    case NT_L: {
      // Process first statement
      SyntaxTreeNode *stmt = node->first_child;
      if (!stmt)
        return NULL;

      // Process next statements if any
      SyntaxTreeNode *next = stmt->next_sibling;
      if (next) {
        // For sibling structure, check if next is part of statement list
        ASTNode *stmt_node = ast_builder_build_node(stmt);
        ASTNode *next_node = ast_builder_build_node(next);
        if (!stmt_node || !next_node) {
          // Handle error
          return NULL;
        }
        return ast_create_statement_list(stmt_node, next_node);
      } else {
        // Single statement
        return ast_builder_build_node(stmt);
      }
    }

    // Statement
    case NT_S: {
      switch (node->production_id) {
      // Assignment: id = E
      case 3:
        return process_assignment(node);

      // If statement: if C then S
      case 4: {
        // Extract condition and then-branch
        SyntaxTreeNode *cond = NULL;
        SyntaxTreeNode *then_branch = NULL;

        // Navigate to find condition and then branch
        SyntaxTreeNode *temp = node->first_child;
        if (temp && temp->next_sibling && temp->next_sibling->next_sibling) {
          cond = temp->next_sibling;
          then_branch = temp->next_sibling->next_sibling->next_sibling;
        }

        if (!cond || !then_branch)
          return NULL;

        ASTNode *cond_node = ast_builder_build_node(cond);
        ASTNode *then_node = ast_builder_build_node(then_branch);

        if (!cond_node || !then_node)
          return NULL;

        return ast_create_if_stmt(cond_node, then_node, NULL);
      }

      // If-else statement: if C then S else S
      case 5: {
        // Extract condition, then-branch, and else-branch
        // Similar to above but with additional navigation for else branch
        // Implement based on your grammar structure
        return NULL; // Placeholder
      }

      // While statement: while C do S
      case 6: {
        // Extract condition and body
        // Implement based on your grammar structure
        return NULL; // Placeholder
      }

      default:
        DEBUG_PRINT("Unknown statement production: %d", node->production_id);
        return NULL;
      }
    }

    // Expression
    case NT_E: {
      switch (node->production_id) {
      // Binary expressions E + T, E - T
      case 10:
        return process_binary_expr(node, TK_ADD);
      case 11:
        return process_binary_expr(node, TK_SUB);

      // Term: E -> T
      case 12:
        return ast_builder_build_node(node->first_child);

      default:
        DEBUG_PRINT("Unknown expression production: %d", node->production_id);
        return NULL;
      }
    }

    // Term
    case NT_T: {
      switch (node->production_id) {
      // Binary terms: T * F, T / F
      case 14:
        return process_binary_expr(node, TK_MUL);
      case 15:
        return process_binary_expr(node, TK_DIV);

      // Factor: T -> F
      case 13:
        return ast_builder_build_node(node->first_child);

      default:
        DEBUG_PRINT("Unknown term production: %d", node->production_id);
        return NULL;
      }
    }

    // Factor
    case NT_F: {
      switch (node->production_id) {
      // Parenthesized expression: ( E )
      case 16: {
        // Find expression inside parentheses
        SyntaxTreeNode *expr = NULL;
        SyntaxTreeNode *temp = node->first_child;

        // Skip the opening parenthesis
        if (temp && temp->next_sibling) {
          expr = temp->next_sibling;
        }

        if (!expr)
          return NULL;

        return ast_builder_build_node(expr);
      }

      // Variable: id
      case 17: {
        SyntaxTreeNode *id_node = find_terminal_child(node, TK_IDN);
        if (!id_node)
          return NULL;

        const char *name = get_token_string_value(id_node);
        if (!name)
          return NULL;

        return ast_create_variable(name);
      }

      // Constants: int
      case 18: // octal
      case 19: // decimal
      case 20: // hex
      {
        SyntaxTreeNode *const_node = node->first_child;
        if (!const_node || const_node->type != NODE_TERMINAL)
          return NULL;

        int value = get_token_numeric_value(const_node);
        return ast_create_constant(value, const_node->token.type);
      }

      default:
        DEBUG_PRINT("Unknown factor production: %d", node->production_id);
        return NULL;
      }
    }

    default:
      DEBUG_PRINT("Unknown non-terminal ID: %d", node->nonterminal_id);
      return NULL;
    }
  }

  case NODE_TERMINAL: {
    // Terminal nodes should generally be handled by their parent non-terminals
    switch (node->token.type) {
    case TK_IDN:
      return ast_create_variable(node->token.str_val);

    case TK_DEC:
    case TK_OCT:
    case TK_HEX:
      return ast_create_constant(node->token.num_val, node->token.type);

    default:
      DEBUG_PRINT("Terminal node reached directly: %d", node->token.type);
      return NULL;
    }
  }

  case NODE_EPSILON:
    // Epsilon nodes typically don't translate to AST nodes
    return NULL;

  default:
    DEBUG_PRINT("Unknown node type: %d", node->type);
    return NULL;
  }
}
