/**
 * @file codegen.c
 * @brief Three-address code generator implementation
 */

#include "codegen/codegen.h"
#include "codegen_internal.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create a new code attributes structure
 */
CodeAttributes *code_attributes_create(void) {
  CodeAttributes *attrs = (CodeAttributes *)safe_malloc(sizeof(CodeAttributes));
  if (!attrs) {
    return NULL;
  }

  attrs->code = NULL;
  attrs->place = NULL;
  attrs->true_label = NULL;
  attrs->false_label = NULL;
  attrs->next_label = NULL;
  attrs->begin_label = NULL;

  return attrs;
}

/**
 * @brief Free code attributes resources
 */
void code_attributes_destroy(CodeAttributes *attrs) {
  if (!attrs) {
    return;
  }

  if (attrs->code)
    free(attrs->code);
  if (attrs->place)
    free(attrs->place);
  if (attrs->true_label)
    free(attrs->true_label);
  if (attrs->false_label)
    free(attrs->false_label);
  if (attrs->next_label)
    free(attrs->next_label);
  if (attrs->begin_label)
    free(attrs->begin_label);

  free(attrs);
}

/**
 * @brief Create a new code generator
 */
CodeGenerator *codegen_create(void) {
  CodeGenerator *gen = (CodeGenerator *)safe_malloc(sizeof(CodeGenerator));
  if (!gen) {
    return NULL;
  }

  gen->symbol_table = NULL;
  gen->label_manager = NULL;
  gen->program = NULL;
  gen->has_error = false;
  memset(gen->error_message, 0, sizeof(gen->error_message));

  DEBUG_PRINT("Created code generator");
  return gen;
}

/**
 * @brief Initialize code generator
 */
bool codegen_init(CodeGenerator *gen) {
  if (!gen) {
    return false;
  }

  /* Create symbol table */
  gen->symbol_table = symbol_table_create();
  if (!gen->symbol_table) {
    fprintf(stderr, "Failed to create symbol table\n");
    return false;
  }

  /* Create label manager */
  gen->label_manager = label_manager_create();
  if (!gen->label_manager) {
    fprintf(stderr, "Failed to create label manager\n");
    symbol_table_destroy(gen->symbol_table);
    return false;
  }

  /* Create TAC program */
  gen->program = tac_program_create();
  if (!gen->program) {
    fprintf(stderr, "Failed to create TAC program\n");
    symbol_table_destroy(gen->symbol_table);
    label_manager_destroy(gen->label_manager);
    return false;
  }

  DEBUG_PRINT("Initialized code generator");
  return true;
}

/**
 * @brief Generate three-address code from a syntax tree
 */
TACProgram *codegen_generate(CodeGenerator *gen, SyntaxTree *tree) {
  if (!gen || !tree) {
    return NULL;
  }

  /* Reset error state */
  gen->has_error = false;
  memset(gen->error_message, 0, sizeof(gen->error_message));

  /* Get root node of syntax tree */
  SyntaxTreeNode *root = syntax_tree_get_root(tree);
  if (!root) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Syntax tree has no root node");
    return NULL;
  }

  /* Generate code for program */
  if (!codegen_program(gen, root)) {
    if (!gen->has_error) {
      gen->has_error = true;
      snprintf(gen->error_message, sizeof(gen->error_message),
               "Failed to generate code for program");
    }
    return NULL;
  }

  DEBUG_PRINT("Generated three-address code from syntax tree");
  return gen->program;
}

/**
 * @brief Generate three-address code directly from input
 */
TACProgram *codegen_generate_from_source(CodeGenerator *gen, Lexer *lexer,
                                         Parser *parser) {
  if (!gen || !lexer || !parser) {
    return NULL;
  }

  /* Parse input */
  SyntaxTree *tree = parser_parse(parser, lexer);
  if (!tree) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Failed to parse input");
    return NULL;
  }

  /* Generate code */
  TACProgram *program = codegen_generate(gen, tree);

  /* Clean up */
  syntax_tree_destroy(tree);

  return program;
}

/**
 * @brief Generate code for a program node
 */
bool codegen_program(CodeGenerator *gen, SyntaxTreeNode *node) {
  if (!gen || !node) {
    return false;
  }

  /* Create code attributes */
  CodeAttributes *attrs = code_attributes_create();
  if (!attrs) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Failed to create code attributes");
    return false;
  }

  /* Check node type */
  if (node->type != NODE_PROGRAM) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Expected program node, got %d", node->type);
    code_attributes_destroy(attrs);
    return false;
  }

  /* Process children */
  for (int i = 0; i < node->child_count; i++) {
    SyntaxTreeNode *child = node->children[i];

    /* Generate code for statement */
    if (!codegen_stmt(gen, child, attrs)) {
      code_attributes_destroy(attrs);
      return false;
    }
  }

  code_attributes_destroy(attrs);
  DEBUG_PRINT("Generated code for program");
  return true;
}

/**
 * @brief Generate code for a statement node
 */
bool codegen_stmt(CodeGenerator *gen, SyntaxTreeNode *node,
                  CodeAttributes *attrs) {
  if (!gen || !node || !attrs) {
    return false;
  }

  /* Determine statement type and delegate */
  switch (node->type) {
  case NODE_ASSIGNMENT:
    return codegen_assignment(gen, node, attrs);

  case NODE_IF:
    return codegen_if_stmt(gen, node, attrs);

  case NODE_IF_ELSE:
    return codegen_if_else_stmt(gen, node, attrs);

  case NODE_WHILE:
    return codegen_while_stmt(gen, node, attrs);

  default:
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Unknown statement type: %d", node->type);
    return false;
  }
}

/**
 * @brief Generate code for an assignment statement
 */
bool codegen_assignment(CodeGenerator *gen, SyntaxTreeNode *node,
                        CodeAttributes *attrs) {
  if (!gen || !node || !attrs) {
    return false;
  }

  /* Check node type */
  if (node->type != NODE_ASSIGNMENT) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Expected assignment node, got %d", node->type);
    return false;
  }

  /* Get variable name (child 0) */
  SyntaxTreeNode *var_node = node->children[0];
  if (!var_node || var_node->type != NODE_IDENTIFIER) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Expected identifier node as assignment target");
    return false;
  }

  const char *var_name = var_node->value.string_val;

  /* Add variable to symbol table if not already there */
  if (!symbol_table_lookup(gen->symbol_table, var_name)) {
    if (!symbol_table_add(gen->symbol_table, var_name, SYMBOL_VARIABLE, NULL)) {
      gen->has_error = true;
      snprintf(gen->error_message, sizeof(gen->error_message),
               "Failed to add variable '%s' to symbol table", var_name);
      return false;
    }
  }

  /* Generate code for expression (child 1) */
  CodeAttributes expr_attrs;
  memset(&expr_attrs, 0, sizeof(CodeAttributes));

  if (!codegen_expression(gen, node->children[1], &expr_attrs)) {
    return false;
  }

  /* Add assignment instruction */
  tac_program_add_inst(gen->program, TAC_OP_ASSIGN, var_name, expr_attrs.place,
                       NULL, node->lineno);

  /* Set return attributes */
  attrs->place = safe_strdup(var_name);

  DEBUG_PRINT("Generated code for assignment: %s := %s", var_name,
              expr_attrs.place);

  return true;
}

/**
 * @brief Generate code for an if statement
 */
bool codegen_if_stmt(CodeGenerator *gen, SyntaxTreeNode *node,
                     CodeAttributes *attrs) {
  if (!gen || !node || !attrs) {
    return false;
  }

  /* Check node type */
  if (node->type != NODE_IF) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Expected if node, got %d", node->type);
    return false;
  }

  /* Generate labels */
  char *true_label = label_manager_new_label(gen->label_manager);
  char *next_label = attrs->next_label
                         ? safe_strdup(attrs->next_label)
                         : label_manager_new_label(gen->label_manager);

  /* Generate code for condition (child 0) */
  CodeAttributes cond_attrs;
  memset(&cond_attrs, 0, sizeof(CodeAttributes));
  cond_attrs.true_label = true_label;
  cond_attrs.false_label = next_label;

  if (!codegen_condition(gen, node->children[0], &cond_attrs)) {
    if (true_label)
      free(true_label);
    if (next_label && !attrs->next_label)
      free(next_label);
    return false;
  }

  /* Add true label */
  tac_program_add_inst(gen->program, TAC_OP_LABEL, true_label, NULL, NULL,
                       node->lineno);

  /* Generate code for then part (child 1) */
  CodeAttributes then_attrs;
  memset(&then_attrs, 0, sizeof(CodeAttributes));
  then_attrs.next_label = next_label;

  if (!codegen_stmt(gen, node->children[1], &then_attrs)) {
    if (true_label)
      free(true_label);
    if (next_label && !attrs->next_label)
      free(next_label);
    return false;
  }

  /* Add next label if it wasn't passed in */
  if (!attrs->next_label) {
    tac_program_add_inst(gen->program, TAC_OP_LABEL, next_label, NULL, NULL,
                         node->lineno);
  }

  /* Clean up */
  if (true_label)
    free(true_label);
  if (next_label && !attrs->next_label)
    free(next_label);

  DEBUG_PRINT("Generated code for if statement");
  return true;
}

/**
 * @brief Generate code for an if-else statement
 */
bool codegen_if_else_stmt(CodeGenerator *gen, SyntaxTreeNode *node,
                          CodeAttributes *attrs) {
  if (!gen || !node || !attrs) {
    return false;
  }

  /* Check node type */
  if (node->type != NODE_IF_ELSE) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Expected if-else node, got %d", node->type);
    return false;
  }

  /* Generate labels */
  char *true_label = label_manager_new_label(gen->label_manager);
  char *false_label = label_manager_new_label(gen->label_manager);
  char *next_label = attrs->next_label
                         ? safe_strdup(attrs->next_label)
                         : label_manager_new_label(gen->label_manager);

  /* Generate code for condition (child 0) */
  CodeAttributes cond_attrs;
  memset(&cond_attrs, 0, sizeof(CodeAttributes));
  cond_attrs.true_label = true_label;
  cond_attrs.false_label = false_label;

  if (!codegen_condition(gen, node->children[0], &cond_attrs)) {
    if (true_label)
      free(true_label);
    if (false_label)
      free(false_label);
    if (next_label && !attrs->next_label)
      free(next_label);
    return false;
  }

  /* Add true label */
  tac_program_add_inst(gen->program, TAC_OP_LABEL, true_label, NULL, NULL,
                       node->lineno);

  /* Generate code for then part (child 1) */
  CodeAttributes then_attrs;
  memset(&then_attrs, 0, sizeof(CodeAttributes));
  then_attrs.next_label = next_label;

  if (!codegen_stmt(gen, node->children[1], &then_attrs)) {
    if (true_label)
      free(true_label);
    if (false_label)
      free(false_label);
    if (next_label && !attrs->next_label)
      free(next_label);
    return false;
  }

  /* Add goto next after then part */
  tac_program_add_inst(gen->program, TAC_OP_GOTO, next_label, NULL, NULL,
                       node->lineno);

  /* Add false label */
  tac_program_add_inst(gen->program, TAC_OP_LABEL, false_label, NULL, NULL,
                       node->lineno);

  /* Generate code for else part (child 2) */
  CodeAttributes else_attrs;
  memset(&else_attrs, 0, sizeof(CodeAttributes));
  else_attrs.next_label = next_label;

  if (!codegen_stmt(gen, node->children[2], &else_attrs)) {
    if (true_label)
      free(true_label);
    if (false_label)
      free(false_label);
    if (next_label && !attrs->next_label)
      free(next_label);
    return false;
  }

  /* Add next label if it wasn't passed in */
  if (!attrs->next_label) {
    tac_program_add_inst(gen->program, TAC_OP_LABEL, next_label, NULL, NULL,
                         node->lineno);
  }

  /* Clean up */
  if (true_label)
    free(true_label);
  if (false_label)
    free(false_label);
  if (next_label && !attrs->next_label)
    free(next_label);

  DEBUG_PRINT("Generated code for if-else statement");
  return true;
}

/**
 * @brief Generate code for a while statement
 */
bool codegen_while_stmt(CodeGenerator *gen, SyntaxTreeNode *node,
                        CodeAttributes *attrs) {
  if (!gen || !node || !attrs) {
    return false;
  }

  /* Check node type */
  if (node->type != NODE_WHILE) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Expected while node, got %d", node->type);
    return false;
  }

  /* Generate labels */
  char *begin_label = label_manager_new_label(gen->label_manager);
  char *true_label = label_manager_new_label(gen->label_manager);
  char *next_label = attrs->next_label
                         ? safe_strdup(attrs->next_label)
                         : label_manager_new_label(gen->label_manager);

  /* Add begin label */
  tac_program_add_inst(gen->program, TAC_OP_LABEL, begin_label, NULL, NULL,
                       node->lineno);

  /* Generate code for condition (child 0) */
  CodeAttributes cond_attrs;
  memset(&cond_attrs, 0, sizeof(CodeAttributes));
  cond_attrs.true_label = true_label;
  cond_attrs.false_label = next_label;

  if (!codegen_condition(gen, node->children[0], &cond_attrs)) {
    if (begin_label)
      free(begin_label);
    if (true_label)
      free(true_label);
    if (next_label && !attrs->next_label)
      free(next_label);
    return false;
  }

  /* Add true label */
  tac_program_add_inst(gen->program, TAC_OP_LABEL, true_label, NULL, NULL,
                       node->lineno);

  /* Generate code for body (child 1) */
  CodeAttributes body_attrs;
  memset(&body_attrs, 0, sizeof(CodeAttributes));
  body_attrs.next_label = begin_label;

  if (!codegen_stmt(gen, node->children[1], &body_attrs)) {
    if (begin_label)
      free(begin_label);
    if (true_label)
      free(true_label);
    if (next_label && !attrs->next_label)
      free(next_label);
    return false;
  }

  /* Add goto begin after body */
  tac_program_add_inst(gen->program, TAC_OP_GOTO, begin_label, NULL, NULL,
                       node->lineno);

  /* Add next label if it wasn't passed in */
  if (!attrs->next_label) {
    tac_program_add_inst(gen->program, TAC_OP_LABEL, next_label, NULL, NULL,
                         node->lineno);
  }

  /* Clean up */
  if (begin_label)
    free(begin_label);
  if (true_label)
    free(true_label);
  if (next_label && !attrs->next_label)
    free(next_label);

  DEBUG_PRINT("Generated code for while statement");
  return true;
}

/**
 * @brief Generate code for a condition
 */
bool codegen_condition(CodeGenerator *gen, SyntaxTreeNode *node,
                       CodeAttributes *attrs) {
  if (!gen || !node || !attrs) {
    return false;
  }

  /* Check for relational operator type */
  TACOpType op_type;

  switch (node->type) {
  case NODE_EQ:
    op_type = TAC_OP_EQ;
    break;
  case NODE_NE:
    op_type = TAC_OP_NE;
    break;
  case NODE_LT:
    op_type = TAC_OP_LT;
    break;
  case NODE_LE:
    op_type = TAC_OP_LE;
    break;
  case NODE_GT:
    op_type = TAC_OP_GT;
    break;
  case NODE_GE:
    op_type = TAC_OP_GE;
    break;
  default:
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Unknown condition type: %d", node->type);
    return false;
  }

  /* Generate code for left operand (child 0) */
  CodeAttributes left_attrs;
  memset(&left_attrs, 0, sizeof(CodeAttributes));

  if (!codegen_expression(gen, node->children[0], &left_attrs)) {
    return false;
  }

  /* Generate code for right operand (child 1) */
  CodeAttributes right_attrs;
  memset(&right_attrs, 0, sizeof(CodeAttributes));

  if (!codegen_expression(gen, node->children[1], &right_attrs)) {
    if (left_attrs.place)
      free(left_attrs.place);
    return false;
  }

  /* Add conditional jump instruction */
  tac_program_add_inst(gen->program, op_type, attrs->true_label,
                       left_attrs.place, right_attrs.place, node->lineno);

  /* Add jump to false label */
  tac_program_add_inst(gen->program, TAC_OP_GOTO, attrs->false_label, NULL,
                       NULL, node->lineno);

  /* Clean up */
  if (left_attrs.place)
    free(left_attrs.place);
  if (right_attrs.place)
    free(right_attrs.place);

  DEBUG_PRINT("Generated code for condition");
  return true;
}

/**
 * @brief Generate code for an expression
 */
bool codegen_expression(CodeGenerator *gen, SyntaxTreeNode *node,
                        CodeAttributes *attrs) {
  if (!gen || !node || !attrs) {
    return false;
  }

  /* Handle different expression types */
  switch (node->type) {
  case NODE_ADD:
  case NODE_SUB:
    /* Binary operation */
    {
      /* Generate code for left operand (child 0) */
      CodeAttributes left_attrs;
      memset(&left_attrs, 0, sizeof(CodeAttributes));

      if (!codegen_expression(gen, node->children[0], &left_attrs)) {
        return false;
      }

      /* Generate code for right operand (child 1) */
      CodeAttributes right_attrs;
      memset(&right_attrs, 0, sizeof(CodeAttributes));

      if (!codegen_term(gen, node->children[1], &right_attrs)) {
        if (left_attrs.place)
          free(left_attrs.place);
        return false;
      }

      /* Create temporary for result */
      char *temp = symbol_table_new_temp(gen->symbol_table);
      if (!temp) {
        if (left_attrs.place)
          free(left_attrs.place);
        if (right_attrs.place)
          free(right_attrs.place);
        return false;
      }

      /* Add operation instruction */
      TACOpType op_type = (node->type == NODE_ADD) ? TAC_OP_ADD : TAC_OP_SUB;
      tac_program_add_inst(gen->program, op_type, temp, left_attrs.place,
                           right_attrs.place, node->lineno);

      /* Set return attributes */
      attrs->place = temp;

      /* Clean up */
      if (left_attrs.place)
        free(left_attrs.place);
      if (right_attrs.place)
        free(right_attrs.place);

      DEBUG_PRINT("Generated code for binary expression: %s",
                  (node->type == NODE_ADD) ? "ADD" : "SUB");
      return true;
    }

  case NODE_TERM:
    /* Term */
    return codegen_term(gen, node, attrs);

  default:
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Unknown expression type: %d", node->type);
    return false;
  }
}

/**
 * @brief Generate code for a term
 */
bool codegen_term(CodeGenerator *gen, SyntaxTreeNode *node,
                  CodeAttributes *attrs) {
  if (!gen || !node || !attrs) {
    return false;
  }

  /* Handle different term types */
  switch (node->type) {
  case NODE_MUL:
  case NODE_DIV:
    /* Binary operation */
    {
      /* Generate code for left operand (child 0) */
      CodeAttributes left_attrs;
      memset(&left_attrs, 0, sizeof(CodeAttributes));

      if (!codegen_term(gen, node->children[0], &left_attrs)) {
        return false;
      }

      /* Generate code for right operand (child 1) */
      CodeAttributes right_attrs;
      memset(&right_attrs, 0, sizeof(CodeAttributes));

      if (!codegen_factor(gen, node->children[1], &right_attrs)) {
        if (left_attrs.place)
          free(left_attrs.place);
        return false;
      }

      /* Create temporary for result */
      char *temp = symbol_table_new_temp(gen->symbol_table);
      if (!temp) {
        if (left_attrs.place)
          free(left_attrs.place);
        if (right_attrs.place)
          free(right_attrs.place);
        return false;
      }

      /* Add operation instruction */
      TACOpType op_type = (node->type == NODE_MUL) ? TAC_OP_MUL : TAC_OP_DIV;
      tac_program_add_inst(gen->program, op_type, temp, left_attrs.place,
                           right_attrs.place, node->lineno);

      /* Set return attributes */
      attrs->place = temp;

      /* Clean up */
      if (left_attrs.place)
        free(left_attrs.place);
      if (right_attrs.place)
        free(right_attrs.place);

      DEBUG_PRINT("Generated code for binary term: %s",
                  (node->type == NODE_MUL) ? "MUL" : "DIV");
      return true;
    }

  case NODE_FACTOR:
    /* Factor */
    return codegen_factor(gen, node, attrs);

  default:
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Unknown term type: %d", node->type);
    return false;
  }
}

/**
 * @brief Generate code for a factor
 */
bool codegen_factor(CodeGenerator *gen, SyntaxTreeNode *node,
                    CodeAttributes *attrs) {
  if (!gen || !node || !attrs) {
    return false;
  }

  /* Handle different factor types */
  switch (node->type) {
  case NODE_IDENTIFIER:
    /* Variable reference */
    {
      const char *var_name = node->value.string_val;

      /* Check if variable exists in symbol table */
      Symbol *symbol = symbol_table_lookup(gen->symbol_table, var_name);
      if (!symbol) {
        /* Add variable to symbol table */
        if (!symbol_table_add(gen->symbol_table, var_name, SYMBOL_VARIABLE,
                              NULL)) {
          gen->has_error = true;
          snprintf(gen->error_message, sizeof(gen->error_message),
                   "Failed to add variable '%s' to symbol table", var_name);
          return false;
        }
      }

      /* Set return attributes */
      attrs->place = safe_strdup(var_name);

      DEBUG_PRINT("Generated code for identifier: %s", var_name);
      return true;
    }

  case NODE_INT_LITERAL:
    /* Integer literal */
    {
      char value_str[32];
      int value = node->value.int_val;

      /* Convert value to string */
      snprintf(value_str, sizeof(value_str), "%d", value);

      /* Set return attributes */
      attrs->place = safe_strdup(value_str);

      DEBUG_PRINT("Generated code for integer literal: %d", value);
      return true;
    }

  case NODE_EXPRESSION:
    /* Parenthesized expression */
    return codegen_expression(gen, node->children[0], attrs);

  default:
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Unknown factor type: %d", node->type);
    return false;
  }
}

/**
 * @brief Free code generator resources
 */
void codegen_destroy(CodeGenerator *gen) {
  if (!gen) {
    return;
  }

  /* Free symbol table */
  if (gen->symbol_table) {
    symbol_table_destroy(gen->symbol_table);
  }

  /* Free label manager */
  if (gen->label_manager) {
    label_manager_destroy(gen->label_manager);
  }

  /* Program is returned to caller, not freed here */

  free(gen);

  DEBUG_PRINT("Destroyed code generator");
}
