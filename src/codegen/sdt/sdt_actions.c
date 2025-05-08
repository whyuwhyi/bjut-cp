/**
 * @file sdt_actions.c
 * @brief Implementation of semantic actions for syntax-directed translation
 */

#include "sdt_actions.h"
#include "common.h"
#include "sdt_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Execute semantic action for a production
 */
bool sdt_execute_action(SDTCodeGen *gen, int production_id,
                        SyntaxTreeNode *node) {
  if (!gen || !node) {
    return false;
  }

  /* Execute the appropriate semantic action for this production */
  return sdt_execute_production_action(gen, production_id, node);
}

/**
 * @brief Helper function to execute semantic action for a production
 */
bool sdt_execute_production_action(SDTCodeGen *gen, int production_id,
                                   SyntaxTreeNode *node) {
  /* Map production ID to semantic action function */
  switch (production_id) {
  case 0: /* P → L */
    return sdt_action_program_1(gen, node);

  case 1: /* P → L P */
    return sdt_action_program_2(gen, node);

  case 2: /* L → S ; */
    return sdt_action_stmt_list(gen, node);

  case 3: /* S → id = E */
    return sdt_action_assign(gen, node);

  case 4: /* S → if C then S */
    return sdt_action_if(gen, node);

  case 5: /* S → if C then S else S */
    return sdt_action_if_else(gen, node);

  case 6: /* S → while C do S */
    return sdt_action_while(gen, node);

  case 7: /* C → E > E */
    return sdt_action_condition_gt(gen, node);

  case 8: /* C → E < E */
    return sdt_action_condition_lt(gen, node);

  case 9: /* C → E = E */
    return sdt_action_condition_eq(gen, node);

  case 10: /* E → E + T */
    return sdt_action_expr_plus(gen, node);

  case 11: /* E → E - T */
    return sdt_action_expr_minus(gen, node);

  case 12: /* E → T */
    return sdt_action_expr_term(gen, node);

  case 13: /* T → F */
    return sdt_action_term_factor(gen, node);

  case 14: /* T → T * F */
    return sdt_action_term_mul(gen, node);

  case 15: /* T → T / F */
    return sdt_action_term_div(gen, node);

  case 16: /* F → ( E ) */
    return sdt_action_factor_paren(gen, node);

  case 17: /* F → id */
    return sdt_action_factor_id(gen, node);

  case 18: /* F → int8 */
    return sdt_action_factor_int8(gen, node);

  case 19: /* F → int10 */
    return sdt_action_factor_int10(gen, node);

  case 20: /* F → int16 */
    return sdt_action_factor_int16(gen, node);

  default:
    /* No semantic action for this production */
    return true;
  }
}

/**
 * @brief Semantic action for program with single statement list
 *
 * P → L { P.code = L.code }
 */
bool sdt_action_program_1(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 1) {
    return false;
  }

  /* Program inherits code from statement list - no specific action needed */
  DEBUG_PRINT("Executed P → L action");
  return true;
}

/**
 * @brief Semantic action for program with multiple statement lists
 *
 * P → L P' { P.code = L.code || P'.code }
 */
bool sdt_action_program_2(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 2) {
    return false;
  }

  /* Program concatenates code from both parts - no specific action needed */
  DEBUG_PRINT("Executed P → L P action");
  return true;
}

/**
 * @brief Semantic action for statement list
 *
 * L → S ; { L.code = S.code }
 */
bool sdt_action_stmt_list(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 2) {
    return false;
  }

  /* Statement list inherits code from statement - no specific action needed */
  DEBUG_PRINT("Executed L → S ; action");
  return true;
}

/**
 * @brief Generic semantic action for statement
 */
bool sdt_action_stmt(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node) {
    return false;
  }

  /* Dispatch to specific statement handlers based on production ID */
  switch (node->production_id) {
  case 3: /* S → id = E */
    return sdt_action_assign(gen, node);
  case 4: /* S → if C then S */
    return sdt_action_if(gen, node);
  case 5: /* S → if C then S else S */
    return sdt_action_if_else(gen, node);
  case 6: /* S → while C do S */
    return sdt_action_while(gen, node);
  default:
    /* Unrecognized statement type */
    return false;
  }
}

/**
 * @brief Semantic action for assignment statement
 *
 * S → id = E { S.code = E.code || gen(id.place ':=' E.place) }
 */
bool sdt_action_assign(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 3) {
    return false;
  }

  /* Get children */
  SyntaxTreeNode *id_node = node->children[0];   /* id */
  SyntaxTreeNode *expr_node = node->children[2]; /* E */

  if (!id_node || !expr_node) {
    return false;
  }

  /* Get variable name from id token */
  const char *var_name = id_node->token.lexeme;

  /* Get expression place from E attributes */
  if (!expr_node->attributes || !expr_node->attributes->place) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Expression does not have a valid place attribute");
    return false;
  }

  const char *expr_place = expr_node->attributes->place;

  /* Generate assignment instruction */
  tac_program_add_inst(gen->program, TAC_OP_ASSIGN, var_name, /* result */
                       expr_place,                            /* arg1 */
                       NULL,                                  /* arg2 */
                       id_node->token.line_no                 /* line number */
  );

  DEBUG_PRINT("Generated assignment: %s := %s", var_name, expr_place);
  return true;
}

/**
 * @brief Semantic action for if statement
 *
 * S → if C then S1
 * {
 *   C.true = newlabel;
 *   C.false = S.next = newlabel;
 *   S.code = C.code || gen(C.true ':') || S1.code || gen(S.next ':')
 * }
 */
bool sdt_action_if(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 3) {
    return false;
  }

  /* Get children */
  SyntaxTreeNode *cond_node = node->children[1]; /* C */
  SyntaxTreeNode *then_node = node->children[3]; /* S1 */

  if (!cond_node || !then_node) {
    return false;
  }

  /* Create required labels if they don't exist */
  if (!cond_node->attributes) {
    cond_node->attributes = sdt_attributes_create();
    if (!cond_node->attributes) {
      return false;
    }
  }

  /* Create labels for condition */
  if (!cond_node->attributes->true_label) {
    cond_node->attributes->true_label =
        label_manager_new_label(gen->label_manager);
  }

  if (!cond_node->attributes->false_label) {
    cond_node->attributes->false_label =
        label_manager_new_label(gen->label_manager);
  }

  /* Generate true label after condition */
  tac_program_add_inst(gen->program, TAC_OP_LABEL,
                       cond_node->attributes->true_label, NULL, NULL,
                       0 /* line number not available */
  );

  /* Generate false/exit label after then part */
  tac_program_add_inst(gen->program, TAC_OP_LABEL,
                       cond_node->attributes->false_label, NULL, NULL,
                       0 /* line number not available */
  );

  DEBUG_PRINT("Generated if statement with labels: true=%s, false=%s",
              cond_node->attributes->true_label,
              cond_node->attributes->false_label);
  return true;
}

/**
 * @brief Semantic action for if-else statement
 *
 * S → if C then S1 else S2
 * {
 *   C.true = newlabel;
 *   C.false = newlabel;
 *   S.next = newlabel;
 *   S.code = C.code || gen(C.true ':') || S1.code
 *          || gen('goto' S.next) || gen(C.false ':')
 *          || S2.code || gen(S.next ':');
 * }
 */
bool sdt_action_if_else(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 5) {
    return false;
  }

  /* Get children */
  SyntaxTreeNode *cond_node = node->children[1]; /* C */
  SyntaxTreeNode *then_node = node->children[3]; /* S1 */
  SyntaxTreeNode *else_node = node->children[5]; /* S2 */

  if (!cond_node || !then_node || !else_node) {
    return false;
  }

  /* Create required labels if they don't exist */
  if (!cond_node->attributes) {
    cond_node->attributes = sdt_attributes_create();
    if (!cond_node->attributes) {
      return false;
    }
  }

  /* Create labels for condition */
  if (!cond_node->attributes->true_label) {
    cond_node->attributes->true_label =
        label_manager_new_label(gen->label_manager);
  }

  if (!cond_node->attributes->false_label) {
    cond_node->attributes->false_label =
        label_manager_new_label(gen->label_manager);
  }

  /* Create next label for end of if-else */
  char *next_label = label_manager_new_label(gen->label_manager);

  /* Generate true label after condition */
  tac_program_add_inst(gen->program, TAC_OP_LABEL,
                       cond_node->attributes->true_label, NULL, NULL,
                       0 /* line number not available */
  );

  /* Generate jump to next label after then part */
  tac_program_add_inst(gen->program, TAC_OP_GOTO, next_label, NULL, NULL,
                       0 /* line number not available */
  );

  /* Generate false label for else part */
  tac_program_add_inst(gen->program, TAC_OP_LABEL,
                       cond_node->attributes->false_label, NULL, NULL,
                       0 /* line number not available */
  );

  /* Generate next label after else part */
  tac_program_add_inst(gen->program, TAC_OP_LABEL, next_label, NULL, NULL,
                       0 /* line number not available */
  );

  /* Free next label (TAC program has its own copy) */
  free(next_label);

  DEBUG_PRINT(
      "Generated if-else statement with labels: true=%s, false=%s, next=%s",
      cond_node->attributes->true_label, cond_node->attributes->false_label,
      next_label);
  return true;
}

/**
 * @brief Semantic action for while statement
 *
 * S → while C do S1
 * {
 *   S.begin = newlabel;
 *   C.true = newlabel;
 *   C.false = S.next = newlabel;
 *   S.code = gen(S.begin ':') || C.code
 *          || gen(C.true ':') || S1.code
 *          || gen('goto' S.begin) || gen(S.next ':');
 * }
 */
bool sdt_action_while(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 4) {
    return false;
  }

  /* Get children */
  SyntaxTreeNode *cond_node = node->children[1]; /* C */
  SyntaxTreeNode *body_node = node->children[3]; /* S1 */

  if (!cond_node || !body_node) {
    return false;
  }

  /* Create required labels if they don't exist */
  if (!cond_node->attributes) {
    cond_node->attributes = sdt_attributes_create();
    if (!cond_node->attributes) {
      return false;
    }
  }

  /* Create begin label for loop start */
  char *begin_label = label_manager_new_label(gen->label_manager);

  /* Create labels for condition */
  if (!cond_node->attributes->true_label) {
    cond_node->attributes->true_label =
        label_manager_new_label(gen->label_manager);
  }

  if (!cond_node->attributes->false_label) {
    cond_node->attributes->false_label =
        label_manager_new_label(gen->label_manager);
  }

  /* Generate begin label at loop start */
  tac_program_add_inst(gen->program, TAC_OP_LABEL, begin_label, NULL, NULL,
                       0 /* line number not available */
  );

  /* Generate true label after condition */
  tac_program_add_inst(gen->program, TAC_OP_LABEL,
                       cond_node->attributes->true_label, NULL, NULL,
                       0 /* line number not available */
  );

  /* Generate goto begin after body */
  tac_program_add_inst(gen->program, TAC_OP_GOTO, begin_label, NULL, NULL,
                       0 /* line number not available */
  );

  /* Generate false label after loop */
  tac_program_add_inst(gen->program, TAC_OP_LABEL,
                       cond_node->attributes->false_label, NULL, NULL,
                       0 /* line number not available */
  );

  /* Free begin label (TAC program has its own copy) */
  free(begin_label);

  DEBUG_PRINT(
      "Generated while statement with labels: begin=%s, true=%s, false=%s",
      begin_label, cond_node->attributes->true_label,
      cond_node->attributes->false_label);
  return true;
}

/**
 * @brief Semantic action for greater-than condition
 *
 * C → E1 > E2
 * {
 *   C.code = E1.code || E2.code ||
 *            gen('if' E1.place '>' E2.place 'goto' C.true) ||
 *            gen('goto' C.false)
 * }
 */
bool sdt_action_condition_gt(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 3) {
    return false;
  }

  /* Get children */
  SyntaxTreeNode *left_node = node->children[0];  /* E1 */
  SyntaxTreeNode *right_node = node->children[2]; /* E2 */

  if (!left_node || !right_node) {
    return false;
  }

  /* Get expression places */
  if (!left_node->attributes || !left_node->attributes->place ||
      !right_node->attributes || !right_node->attributes->place) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Expression does not have a valid place attribute");
    return false;
  }

  /* Create attributes for condition node if not exists */
  if (!node->attributes) {
    node->attributes = sdt_attributes_create();
    if (!node->attributes) {
      return false;
    }
  }

  /* Generate labels if needed */
  if (!node->attributes->true_label) {
    node->attributes->true_label = label_manager_new_label(gen->label_manager);
  }

  if (!node->attributes->false_label) {
    node->attributes->false_label = label_manager_new_label(gen->label_manager);
  }

  /* Generate conditional jump instruction */
  tac_program_add_inst(
      gen->program, TAC_OP_GT,
      node->attributes->true_label,  /* goto this label if condition is true */
      left_node->attributes->place,  /* left operand */
      right_node->attributes->place, /* right operand */
      0                              /* line number not available */
  );

  /* Generate jump to false label */
  tac_program_add_inst(gen->program, TAC_OP_GOTO, node->attributes->false_label,
                       NULL, NULL, 0 /* line number not available */
  );

  DEBUG_PRINT("Generated condition E > E with jumps to: true=%s, false=%s",
              node->attributes->true_label, node->attributes->false_label);
  return true;
}

/**
 * @brief Semantic action for less-than condition
 *
 * C → E1 < E2
 * {
 *   C.code = E1.code || E2.code ||
 *            gen('if' E1.place '<' E2.place 'goto' C.true) ||
 *            gen('goto' C.false)
 * }
 */
bool sdt_action_condition_lt(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 3) {
    return false;
  }

  /* Get children */
  SyntaxTreeNode *left_node = node->children[0];  /* E1 */
  SyntaxTreeNode *right_node = node->children[2]; /* E2 */

  if (!left_node || !right_node) {
    return false;
  }

  /* Get expression places */
  if (!left_node->attributes || !left_node->attributes->place ||
      !right_node->attributes || !right_node->attributes->place) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Expression does not have a valid place attribute");
    return false;
  }

  /* Create attributes for condition node if not exists */
  if (!node->attributes) {
    node->attributes = sdt_attributes_create();
    if (!node->attributes) {
      return false;
    }
  }

  /* Generate labels if needed */
  if (!node->attributes->true_label) {
    node->attributes->true_label = label_manager_new_label(gen->label_manager);
  }

  if (!node->attributes->false_label) {
    node->attributes->false_label = label_manager_new_label(gen->label_manager);
  }

  /* Generate conditional jump instruction */
  tac_program_add_inst(
      gen->program, TAC_OP_LT,
      node->attributes->true_label,  /* goto this label if condition is true */
      left_node->attributes->place,  /* left operand */
      right_node->attributes->place, /* right operand */
      0                              /* line number not available */
  );

  /* Generate jump to false label */
  tac_program_add_inst(gen->program, TAC_OP_GOTO, node->attributes->false_label,
                       NULL, NULL, 0 /* line number not available */
  );

  DEBUG_PRINT("Generated condition E < E with jumps to: true=%s, false=%s",
              node->attributes->true_label, node->attributes->false_label);
  return true;
}

/**
 * @brief Semantic action for equality condition
 *
 * C → E1 = E2
 * {
 *   C.code = E1.code || E2.code ||
 *            gen('if' E1.place '=' E2.place 'goto' C.true) ||
 *            gen('goto' C.false)
 * }
 */
bool sdt_action_condition_eq(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 3) {
    return false;
  }

  /* Get children */
  SyntaxTreeNode *left_node = node->children[0];  /* E1 */
  SyntaxTreeNode *right_node = node->children[2]; /* E2 */

  if (!left_node || !right_node) {
    return false;
  }

  /* Get expression places */
  if (!left_node->attributes || !left_node->attributes->place ||
      !right_node->attributes || !right_node->attributes->place) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Expression does not have a valid place attribute");
    return false;
  }

  /* Create attributes for condition node if not exists */
  if (!node->attributes) {
    node->attributes = sdt_attributes_create();
    if (!node->attributes) {
      return false;
    }
  }

  /* Generate labels if needed */
  if (!node->attributes->true_label) {
    node->attributes->true_label = label_manager_new_label(gen->label_manager);
  }

  if (!node->attributes->false_label) {
    node->attributes->false_label = label_manager_new_label(gen->label_manager);
  }

  /* Generate conditional jump instruction */
  tac_program_add_inst(
      gen->program, TAC_OP_EQ,
      node->attributes->true_label,  /* goto this label if condition is true */
      left_node->attributes->place,  /* left operand */
      right_node->attributes->place, /* right operand */
      0                              /* line number not available */
  );

  /* Generate jump to false label */
  tac_program_add_inst(gen->program, TAC_OP_GOTO, node->attributes->false_label,
                       NULL, NULL, 0 /* line number not available */
  );

  DEBUG_PRINT("Generated condition E = E with jumps to: true=%s, false=%s",
              node->attributes->true_label, node->attributes->false_label);
  return true;
}

/**
 * @brief Semantic action for addition expression
 *
 * E → E1 + T
 * {
 *   E.place = newtemp;
 *   E.code = E1.code || T.code ||
 *            gen(E.place ':=' E1.place '+' T.place)
 * }
 */
bool sdt_action_expr_plus(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 3) {
    return false;
  }

  /* Get children */
  SyntaxTreeNode *left_node = node->children[0];  /* E1 */
  SyntaxTreeNode *right_node = node->children[2]; /* T */

  if (!left_node || !right_node) {
    return false;
  }

  /* Get operand places */
  if (!left_node->attributes || !left_node->attributes->place ||
      !right_node->attributes || !right_node->attributes->place) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Expression does not have a valid place attribute");
    return false;
  }

  /* Create attributes for result if not exists */
  if (!node->attributes) {
    node->attributes = sdt_attributes_create();
    if (!node->attributes) {
      return false;
    }
  }

  /* Create a new temporary for the result */
  char *temp = symbol_table_new_temp(gen->symbol_table);
  if (!temp) {
    return false;
  }

  /* Store result place */
  if (node->attributes->place) {
    free(node->attributes->place);
  }
  node->attributes->place = temp;

  /* Generate addition instruction */
  tac_program_add_inst(gen->program, TAC_OP_ADD,
                       node->attributes->place,       /* result */
                       left_node->attributes->place,  /* left operand */
                       right_node->attributes->place, /* right operand */
                       0 /* line number not available */
  );

  DEBUG_PRINT("Generated addition: %s := %s + %s", node->attributes->place,
              left_node->attributes->place, right_node->attributes->place);
  return true;
}

/**
 * @brief Semantic action for subtraction expression
 *
 * E → E1 - T
 * {
 *   E.place = newtemp;
 *   E.code = E1.code || T.code ||
 *            gen(E.place ':=' E1.place '-' T.place)
 * }
 */
bool sdt_action_expr_minus(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 3) {
    return false;
  }

  /* Get children */
  SyntaxTreeNode *left_node = node->children[0];  /* E1 */
  SyntaxTreeNode *right_node = node->children[2]; /* T */

  if (!left_node || !right_node) {
    return false;
  }

  /* Get operand places */
  if (!left_node->attributes || !left_node->attributes->place ||
      !right_node->attributes || !right_node->attributes->place) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Expression does not have a valid place attribute");
    return false;
  }

  /* Create attributes for result if not exists */
  if (!node->attributes) {
    node->attributes = sdt_attributes_create();
    if (!node->attributes) {
      return false;
    }
  }

  /* Create a new temporary for the result */
  char *temp = symbol_table_new_temp(gen->symbol_table);
  if (!temp) {
    return false;
  }

  /* Store result place */
  if (node->attributes->place) {
    free(node->attributes->place);
  }
  node->attributes->place = temp;

  /* Generate subtraction instruction */
  tac_program_add_inst(gen->program, TAC_OP_SUB,
                       node->attributes->place,       /* result */
                       left_node->attributes->place,  /* left operand */
                       right_node->attributes->place, /* right operand */
                       0 /* line number not available */
  );

  DEBUG_PRINT("Generated subtraction: %s := %s - %s", node->attributes->place,
              left_node->attributes->place, right_node->attributes->place);
  return true;
}

/**
 * @brief Semantic action for expression from term
 *
 * E → T { E.place = T.place; E.code = T.code }
 */
bool sdt_action_expr_term(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 1) {
    return false;
  }

  /* Get term node */
  SyntaxTreeNode *term_node = node->children[0]; /* T */

  if (!term_node) {
    return false;
  }

  /* Get term place */
  if (!term_node->attributes || !term_node->attributes->place) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Term does not have a valid place attribute");
    return false;
  }

  /* Create attributes for expression if not exists */
  if (!node->attributes) {
    node->attributes = sdt_attributes_create();
    if (!node->attributes) {
      return false;
    }
  }

  /* Copy term place to expression place */
  if (node->attributes->place) {
    free(node->attributes->place);
  }
  node->attributes->place = safe_strdup(term_node->attributes->place);

  DEBUG_PRINT("Expression inherited place from term: %s",
              node->attributes->place);
  return true;
}

/**
 * @brief Semantic action for multiplication term
 *
 * T → T1 * F
 * {
 *   T.place = newtemp;
 *   T.code = T1.code || F.code ||
 *            gen(T.place ':=' T1.place '*' F.place)
 * }
 */
bool sdt_action_term_mul(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 3) {
    return false;
  }

  /* Get children */
  SyntaxTreeNode *left_node = node->children[0];  /* T1 */
  SyntaxTreeNode *right_node = node->children[2]; /* F */

  if (!left_node || !right_node) {
    return false;
  }

  /* Get operand places */
  if (!left_node->attributes || !left_node->attributes->place ||
      !right_node->attributes || !right_node->attributes->place) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Term or factor does not have a valid place attribute");
    return false;
  }

  /* Create attributes for result if not exists */
  if (!node->attributes) {
    node->attributes = sdt_attributes_create();
    if (!node->attributes) {
      return false;
    }
  }

  /* Create a new temporary for the result */
  char *temp = symbol_table_new_temp(gen->symbol_table);
  if (!temp) {
    return false;
  }

  /* Store result place */
  if (node->attributes->place) {
    free(node->attributes->place);
  }
  node->attributes->place = temp;

  /* Generate multiplication instruction */
  tac_program_add_inst(gen->program, TAC_OP_MUL,
                       node->attributes->place,       /* result */
                       left_node->attributes->place,  /* left operand */
                       right_node->attributes->place, /* right operand */
                       0 /* line number not available */
  );

  DEBUG_PRINT("Generated multiplication: %s := %s * %s",
              node->attributes->place, left_node->attributes->place,
              right_node->attributes->place);
  return true;
}

/**
 * @brief Semantic action for division term
 *
 * T → T1 / F
 * {
 *   T.place = newtemp;
 *   T.code = T1.code || F.code ||
 *            gen(T.place ':=' T1.place '/' F.place)
 * }
 */
bool sdt_action_term_div(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 3) {
    return false;
  }

  /* Get children */
  SyntaxTreeNode *left_node = node->children[0];  /* T1 */
  SyntaxTreeNode *right_node = node->children[2]; /* F */

  if (!left_node || !right_node) {
    return false;
  }

  /* Get operand places */
  if (!left_node->attributes || !left_node->attributes->place ||
      !right_node->attributes || !right_node->attributes->place) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Term or factor does not have a valid place attribute");
    return false;
  }

  /* Create attributes for result if not exists */
  if (!node->attributes) {
    node->attributes = sdt_attributes_create();
    if (!node->attributes) {
      return false;
    }
  }

  /* Create a new temporary for the result */
  char *temp = symbol_table_new_temp(gen->symbol_table);
  if (!temp) {
    return false;
  }

  /* Store result place */
  if (node->attributes->place) {
    free(node->attributes->place);
  }
  node->attributes->place = temp;

  /* Generate division instruction */
  tac_program_add_inst(gen->program, TAC_OP_DIV,
                       node->attributes->place,       /* result */
                       left_node->attributes->place,  /* left operand */
                       right_node->attributes->place, /* right operand */
                       0 /* line number not available */
  );

  DEBUG_PRINT("Generated division: %s := %s / %s", node->attributes->place,
              left_node->attributes->place, right_node->attributes->place);
  return true;
}

/**
 * @brief Semantic action for term from factor
 *
 * T → F { T.place = F.place; T.code = F.code }
 */
bool sdt_action_term_factor(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 1) {
    return false;
  }

  /* Get factor node */
  SyntaxTreeNode *factor_node = node->children[0]; /* F */

  if (!factor_node) {
    return false;
  }

  /* Get factor place */
  if (!factor_node->attributes || !factor_node->attributes->place) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Factor does not have a valid place attribute");
    return false;
  }

  /* Create attributes for term if not exists */
  if (!node->attributes) {
    node->attributes = sdt_attributes_create();
    if (!node->attributes) {
      return false;
    }
  }

  /* Copy factor place to term place */
  if (node->attributes->place) {
    free(node->attributes->place);
  }
  node->attributes->place = safe_strdup(factor_node->attributes->place);

  DEBUG_PRINT("Term inherited place from factor: %s", node->attributes->place);
  return true;
}

/**
 * @brief Semantic action for parenthesized expression
 *
 * F → ( E ) { F.place = E.place; F.code = E.code }
 */
bool sdt_action_factor_paren(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 3) {
    return false;
  }

  /* Get expression node */
  SyntaxTreeNode *expr_node = node->children[1]; /* E */

  if (!expr_node) {
    return false;
  }

  /* Get expression place */
  if (!expr_node->attributes || !expr_node->attributes->place) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Expression does not have a valid place attribute");
    return false;
  }

  /* Create attributes for factor if not exists */
  if (!node->attributes) {
    node->attributes = sdt_attributes_create();
    if (!node->attributes) {
      return false;
    }
  }

  /* Copy expression place to factor place */
  if (node->attributes->place) {
    free(node->attributes->place);
  }
  node->attributes->place = safe_strdup(expr_node->attributes->place);

  DEBUG_PRINT("Factor inherited place from parenthesized expression: %s",
              node->attributes->place);
  return true;
}

/**
 * @brief Semantic action for identifier factor
 *
 * F → id { F.place = id.name; F.code = '' }
 */
bool sdt_action_factor_id(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 1) {
    return false;
  }

  /* Get identifier node */
  SyntaxTreeNode *id_node = node->children[0]; /* id */

  if (!id_node) {
    return false;
  }

  /* Get identifier lexeme */
  const char *var_name = id_node->token.lexeme;

  /* Create attributes for factor if not exists */
  if (!node->attributes) {
    node->attributes = sdt_attributes_create();
    if (!node->attributes) {
      return false;
    }
  }

  /* Set factor place to identifier name */
  if (node->attributes->place) {
    free(node->attributes->place);
  }
  node->attributes->place = safe_strdup(var_name);

  DEBUG_PRINT("Factor place set to identifier: %s", node->attributes->place);
  return true;
}

/**
 * @brief Semantic action for octal integer factor
 *
 * F → int8 { F.place = int8.value; F.code = '' }
 */
bool sdt_action_factor_int8(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 1) {
    return false;
  }

  /* Get integer node */
  SyntaxTreeNode *int_node = node->children[0]; /* int8 */

  if (!int_node) {
    return false;
  }

  /* Get integer lexeme */
  const char *int_value = int_node->token.lexeme;

  /* Create attributes for factor if not exists */
  if (!node->attributes) {
    node->attributes = sdt_attributes_create();
    if (!node->attributes) {
      return false;
    }
  }

  /* Set factor place to integer value */
  if (node->attributes->place) {
    free(node->attributes->place);
  }
  node->attributes->place = safe_strdup(int_value);

  DEBUG_PRINT("Factor place set to octal integer: %s", node->attributes->place);
  return true;
}

/**
 * @brief Semantic action for decimal integer factor
 *
 * F → int10 { F.place = int10.value; F.code = '' }
 */
bool sdt_action_factor_int10(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 1) {
    return false;
  }

  /* Get integer node */
  SyntaxTreeNode *int_node = node->children[0]; /* int10 */

  if (!int_node) {
    return false;
  }

  /* Get integer lexeme */
  const char *int_value = int_node->token.lexeme;

  /* Create attributes for factor if not exists */
  if (!node->attributes) {
    node->attributes = sdt_attributes_create();
    if (!node->attributes) {
      return false;
    }
  }

  /* Set factor place to integer value */
  if (node->attributes->place) {
    free(node->attributes->place);
  }
  node->attributes->place = safe_strdup(int_value);

  DEBUG_PRINT("Factor place set to decimal integer: %s",
              node->attributes->place);
  return true;
}

/**
 * @brief Semantic action for hexadecimal integer factor
 *
 * F → int16 { F.place = int16.value; F.code = '' }
 */
bool sdt_action_factor_int16(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 1) {
    return false;
  }

  /* Get integer node */
  SyntaxTreeNode *int_node = node->children[0]; /* int16 */

  if (!int_node) {
    return false;
  }

  /* Get integer lexeme */
  const char *int_value = int_node->token.lexeme;

  /* Create attributes for factor if not exists */
  if (!node->attributes) {
    node->attributes = sdt_attributes_create();
    if (!node->attributes) {
      return false;
    }
  }

  /* Set factor place to integer value */
  if (node->attributes->place) {
    free(node->attributes->place);
  }
  node->attributes->place = safe_strdup(int_value);

  DEBUG_PRINT("Factor place set to hexadecimal integer: %s",
              node->attributes->place);
  return true;
}
