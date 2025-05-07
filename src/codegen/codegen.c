/**
 * @file codegen.c
 * @brief Three-address code generator implementation
 */

#include "codegen/codegen.h"
#include "ast/ast_builder.h"
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

  /* Build AST from syntax tree */
  AST *ast = ast_builder_build(tree);
  if (!ast) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Failed to build AST from syntax tree");
    return NULL;
  }

  /* Generate code for program */
  if (!codegen_program_ast(gen, ast->root)) {
    if (!gen->has_error) {
      gen->has_error = true;
      snprintf(gen->error_message, sizeof(gen->error_message),
               "Failed to generate code for program");
    }
    ast_destroy(ast);
    return NULL;
  }

  /* Clean up AST */
  ast_destroy(ast);

  DEBUG_PRINT("Generated three-address code from AST");
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
 * @brief Generate code for a program node from AST
 */
bool codegen_program_ast(CodeGenerator *gen, ASTNode *node) {
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
  if (node->type != AST_PROGRAM) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Expected AST program node, got %d", node->type);
    code_attributes_destroy(attrs);
    return false;
  }

  /* Process statements */
  ASTNode *stmt_list = node->program.statement_list;
  while (stmt_list) {
    if (stmt_list->type == AST_STATEMENT_LIST) {
      /* Process current statement */
      if (!codegen_stmt_ast(gen, stmt_list->statement_list.statement, attrs)) {
        code_attributes_destroy(attrs);
        return false;
      }

      /* Move to next statement */
      stmt_list = stmt_list->statement_list.next;
    } else {
      /* Single statement without a list */
      if (!codegen_stmt_ast(gen, stmt_list, attrs)) {
        code_attributes_destroy(attrs);
        return false;
      }
      break;
    }
  }

  code_attributes_destroy(attrs);
  DEBUG_PRINT("Generated code for program from AST");
  return true;
}

/**
 * @brief Generate code for a statement from AST
 */
bool codegen_stmt_ast(CodeGenerator *gen, ASTNode *node,
                      CodeAttributes *attrs) {
  if (!gen || !node || !attrs) {
    return false;
  }

  /* Determine statement type and delegate */
  switch (node->type) {
  case AST_ASSIGN_STMT:
    return codegen_assignment_ast(gen, node, attrs);

  case AST_IF_STMT:
    return codegen_if_stmt_ast(gen, node, attrs);

  case AST_WHILE_STMT:
    return codegen_while_stmt_ast(gen, node, attrs);

  default:
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Unknown AST statement type: %d", node->type);
    return false;
  }
}

/**
 * @brief Generate code for an assignment statement from AST
 */
bool codegen_assignment_ast(CodeGenerator *gen, ASTNode *node,
                            CodeAttributes *attrs) {
  if (!gen || !node || !attrs) {
    return false;
  }

  /* Check node type */
  if (node->type != AST_ASSIGN_STMT) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Expected AST assignment node, got %d", node->type);
    return false;
  }

  const char *var_name = node->assign_stmt.variable_name;

  /* Add variable to symbol table if not already there */
  if (!symbol_table_lookup(gen->symbol_table, var_name)) {
    if (!symbol_table_add(gen->symbol_table, var_name, SYMBOL_VARIABLE, NULL)) {
      gen->has_error = true;
      snprintf(gen->error_message, sizeof(gen->error_message),
               "Failed to add variable '%s' to symbol table", var_name);
      return false;
    }
  }

  /* Generate code for expression */
  CodeAttributes expr_attrs;
  memset(&expr_attrs, 0, sizeof(CodeAttributes));

  if (!codegen_expression_ast(gen, node->assign_stmt.expression, &expr_attrs)) {
    return false;
  }

  /* Add assignment instruction */
  tac_program_add_inst(gen->program, TAC_OP_ASSIGN, var_name, expr_attrs.place,
                       NULL, 0); // Line number not available in AST

  /* Set return attributes */
  attrs->place = safe_strdup(var_name);

  DEBUG_PRINT("Generated code for assignment: %s := %s", var_name,
              expr_attrs.place);

  if (expr_attrs.place) {
    free(expr_attrs.place);
  }

  return true;
}

/**
 * @brief Generate code for an if statement from AST
 */
bool codegen_if_stmt_ast(CodeGenerator *gen, ASTNode *node,
                         CodeAttributes *attrs) {
  if (!gen || !node || !attrs) {
    return false;
  }

  /* Check node type */
  if (node->type != AST_IF_STMT) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Expected AST if node, got %d", node->type);
    return false;
  }

  /* Generate labels */
  char *true_label = label_manager_new_label(gen->label_manager);
  char *next_label = attrs->next_label
                         ? safe_strdup(attrs->next_label)
                         : label_manager_new_label(gen->label_manager);
  char *false_label = node->if_stmt.else_branch
                          ? label_manager_new_label(gen->label_manager)
                          : next_label;

  /* Generate code for condition */
  CodeAttributes cond_attrs;
  memset(&cond_attrs, 0, sizeof(CodeAttributes));
  cond_attrs.true_label = true_label;
  cond_attrs.false_label = false_label;

  if (!codegen_condition_ast(gen, node->if_stmt.condition, &cond_attrs)) {
    if (true_label)
      free(true_label);
    if (next_label && !attrs->next_label)
      free(next_label);
    if (false_label && false_label != next_label)
      free(false_label);
    return false;
  }

  /* Add true label */
  tac_program_add_inst(gen->program, TAC_OP_LABEL, true_label, NULL, NULL, 0);

  /* Generate code for then part */
  CodeAttributes then_attrs;
  memset(&then_attrs, 0, sizeof(CodeAttributes));
  then_attrs.next_label = next_label;

  if (!codegen_stmt_ast(gen, node->if_stmt.then_branch, &then_attrs)) {
    if (true_label)
      free(true_label);
    if (next_label && !attrs->next_label)
      free(next_label);
    if (false_label && false_label != next_label)
      free(false_label);
    return false;
  }

  /* Handle else part if it exists */
  if (node->if_stmt.else_branch) {
    /* Add goto next after then part */
    tac_program_add_inst(gen->program, TAC_OP_GOTO, next_label, NULL, NULL, 0);

    /* Add false label */
    tac_program_add_inst(gen->program, TAC_OP_LABEL, false_label, NULL, NULL,
                         0);

    /* Generate code for else part */
    CodeAttributes else_attrs;
    memset(&else_attrs, 0, sizeof(CodeAttributes));
    else_attrs.next_label = next_label;

    if (!codegen_stmt_ast(gen, node->if_stmt.else_branch, &else_attrs)) {
      if (true_label)
        free(true_label);
      if (next_label && !attrs->next_label)
        free(next_label);
      if (false_label && false_label != next_label)
        free(false_label);
      return false;
    }
  }

  /* Add next label if it wasn't passed in */
  if (!attrs->next_label) {
    tac_program_add_inst(gen->program, TAC_OP_LABEL, next_label, NULL, NULL, 0);
  }

  /* Clean up */
  if (true_label)
    free(true_label);
  if (next_label && !attrs->next_label)
    free(next_label);
  if (false_label && false_label != next_label)
    free(false_label);

  DEBUG_PRINT("Generated code for if statement from AST");
  return true;
}

/**
 * @brief Generate code for a while statement from AST
 */
bool codegen_while_stmt_ast(CodeGenerator *gen, ASTNode *node,
                            CodeAttributes *attrs) {
  if (!gen || !node || !attrs) {
    return false;
  }

  /* Check node type */
  if (node->type != AST_WHILE_STMT) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Expected AST while node, got %d", node->type);
    return false;
  }

  /* Generate labels */
  char *begin_label = label_manager_new_label(gen->label_manager);
  char *true_label = label_manager_new_label(gen->label_manager);
  char *next_label = attrs->next_label
                         ? safe_strdup(attrs->next_label)
                         : label_manager_new_label(gen->label_manager);

  /* Add begin label */
  tac_program_add_inst(gen->program, TAC_OP_LABEL, begin_label, NULL, NULL, 0);

  /* Generate code for condition */
  CodeAttributes cond_attrs;
  memset(&cond_attrs, 0, sizeof(CodeAttributes));
  cond_attrs.true_label = true_label;
  cond_attrs.false_label = next_label;

  if (!codegen_condition_ast(gen, node->while_stmt.condition, &cond_attrs)) {
    if (begin_label)
      free(begin_label);
    if (true_label)
      free(true_label);
    if (next_label && !attrs->next_label)
      free(next_label);
    return false;
  }

  /* Add true label */
  tac_program_add_inst(gen->program, TAC_OP_LABEL, true_label, NULL, NULL, 0);

  /* Generate code for body */
  CodeAttributes body_attrs;
  memset(&body_attrs, 0, sizeof(CodeAttributes));
  body_attrs.next_label = begin_label;

  if (!codegen_stmt_ast(gen, node->while_stmt.body, &body_attrs)) {
    if (begin_label)
      free(begin_label);
    if (true_label)
      free(true_label);
    if (next_label && !attrs->next_label)
      free(next_label);
    return false;
  }

  /* Add goto begin after body */
  tac_program_add_inst(gen->program, TAC_OP_GOTO, begin_label, NULL, NULL, 0);

  /* Add next label if it wasn't passed in */
  if (!attrs->next_label) {
    tac_program_add_inst(gen->program, TAC_OP_LABEL, next_label, NULL, NULL, 0);
  }

  /* Clean up */
  if (begin_label)
    free(begin_label);
  if (true_label)
    free(true_label);
  if (next_label && !attrs->next_label)
    free(next_label);

  DEBUG_PRINT("Generated code for while statement from AST");
  return true;
}

/**
 * @brief Generate code for a condition from AST
 */
bool codegen_condition_ast(CodeGenerator *gen, ASTNode *node,
                           CodeAttributes *attrs) {
  if (!gen || !node || !attrs) {
    return false;
  }

  /* Check for binary expression with comparison operator */
  if (node->type != AST_BINARY_EXPR) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Expected AST binary expression for condition, got %d",
             node->type);
    return false;
  }

  /* Determine comparison operator type */
  TACOpType op_type;
  switch (node->binary_expr.op) {
  case OP_EQ:
    op_type = TAC_OP_EQ;
    break;
  case OP_LT:
    op_type = TAC_OP_LT;
    break;
  case OP_GT:
    op_type = TAC_OP_GT;
    break;
  default:
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Unsupported operator type in condition: %d",
             node->binary_expr.op);
    return false;
  }

  /* Generate code for left operand */
  CodeAttributes left_attrs;
  memset(&left_attrs, 0, sizeof(CodeAttributes));

  if (!codegen_expression_ast(gen, node->binary_expr.left, &left_attrs)) {
    return false;
  }

  /* Generate code for right operand */
  CodeAttributes right_attrs;
  memset(&right_attrs, 0, sizeof(CodeAttributes));

  if (!codegen_expression_ast(gen, node->binary_expr.right, &right_attrs)) {
    if (left_attrs.place)
      free(left_attrs.place);
    return false;
  }

  /* Add conditional jump instruction */
  tac_program_add_inst(gen->program, op_type, attrs->true_label,
                       left_attrs.place, right_attrs.place, 0);

  /* Add jump to false label */
  tac_program_add_inst(gen->program, TAC_OP_GOTO, attrs->false_label, NULL,
                       NULL, 0);

  /* Clean up */
  if (left_attrs.place)
    free(left_attrs.place);
  if (right_attrs.place)
    free(right_attrs.place);

  DEBUG_PRINT("Generated code for condition from AST");
  return true;
}

/**
 * @brief Generate code for an expression from AST
 */
bool codegen_expression_ast(CodeGenerator *gen, ASTNode *node,
                            CodeAttributes *attrs) {
  if (!gen || !node || !attrs) {
    return false;
  }

  /* Handle different expression types */
  switch (node->type) {
  case AST_BINARY_EXPR: {
    /* Binary operation */
    BinaryOpType op = node->binary_expr.op;
    TACOpType tac_op;

    /* Map binary operator types to TAC operator types */
    switch (op) {
    case OP_ADD:
      tac_op = TAC_OP_ADD;
      break;
    case OP_SUB:
      tac_op = TAC_OP_SUB;
      break;
    case OP_MUL:
      tac_op = TAC_OP_MUL;
      break;
    case OP_DIV:
      tac_op = TAC_OP_DIV;
      break;
    default:
      gen->has_error = true;
      snprintf(gen->error_message, sizeof(gen->error_message),
               "Unsupported binary operator in expression: %d", op);
      return false;
    }

    /* Generate code for left operand */
    CodeAttributes left_attrs;
    memset(&left_attrs, 0, sizeof(CodeAttributes));

    if (!codegen_expression_ast(gen, node->binary_expr.left, &left_attrs)) {
      return false;
    }

    /* Generate code for right operand */
    CodeAttributes right_attrs;
    memset(&right_attrs, 0, sizeof(CodeAttributes));

    if (!codegen_expression_ast(gen, node->binary_expr.right, &right_attrs)) {
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
    tac_program_add_inst(gen->program, tac_op, temp, left_attrs.place,
                         right_attrs.place, 0);

    /* Set return attributes */
    attrs->place = temp;

    /* Clean up */
    if (left_attrs.place)
      free(left_attrs.place);
    if (right_attrs.place)
      free(right_attrs.place);

    DEBUG_PRINT("Generated code for binary expression from AST");
    return true;
  }

  case AST_VARIABLE: {
    /* Variable reference */
    const char *var_name = node->variable.name;

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

    DEBUG_PRINT("Generated code for variable reference: %s", var_name);
    return true;
  }

  case AST_CONSTANT: {
    /* Integer literal */
    char value_str[32];
    int value = node->constant.value;

    /* Convert value to string */
    snprintf(value_str, sizeof(value_str), "%d", value);

    /* Set return attributes */
    attrs->place = safe_strdup(value_str);

    DEBUG_PRINT("Generated code for constant: %d", value);
    return true;
  }

  default:
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Unknown AST expression type: %d", node->type);
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
