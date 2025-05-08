/**
 * @file sdt_actions.c
 * @brief Implementation of semantic actions for syntax-directed translation
 */

#include "sdt_actions.h"
#include "../sdt_internal.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Helper function to execute semantic action for a production
 */
bool sdt_execute_production_action(SDTCodeGen *gen, int production_id,
                                   SyntaxTreeNode *node) {
  /* Map production ID to semantic action function */
  switch (production_id) {
  case 0: /* P → L T */
    return sdt_action_program_1(gen, node);

  case 1: /* T → P T */
    return sdt_action_program_2(gen, node);

  case 2:        /* T → ε */
    return true; /* No action needed for epsilon */

  case 3: /* L → S ; */
    return sdt_action_stmt_list(gen, node);

  case 4: /* S → id = E */
    return sdt_action_assign(gen, node);

  case 5: /* S → if C then S N */
    return sdt_action_if_else(gen, node);

  case 6: /* S → while C do S */
    return sdt_action_while(gen, node);

  case 7: /* S → begin L end */
    return sdt_action_block(gen, node);
  case 8: /* N → else S */
    return sdt_action_else_part(gen, node);

  case 9:        /* N → ε */
    return true; /* No action needed for epsilon */

  case 10: /* C → E > E */
    return sdt_action_condition_gt(gen, node);

  case 11: /* C → E < E */
    return sdt_action_condition_lt(gen, node);

  case 12: /* C → E = E */
    return sdt_action_condition_eq(gen, node);

  case 13: /* C → E >= E */
    return sdt_action_condition_ge(gen, node);

  case 14: /* C → E <= E */
    return sdt_action_condition_le(gen, node);

  case 15: /* C → E <> E */
    return sdt_action_condition_ne(gen, node);

  case 16: /* C → ( C ) */
    return sdt_action_condition_paren(gen, node);

  case 17: /* E → R X */
    return sdt_action_expr_term(gen, node);

  case 18: /* X → + R X */
    return sdt_action_expr_plus(gen, node);

  case 19: /* X → - R X */
    return sdt_action_expr_minus(gen, node);

  case 20:       /* X → ε */
    return true; /* No action needed for epsilon */

  case 21: /* R → F Y */
    return sdt_action_term_factor(gen, node);

  case 22: /* Y → * F Y */
    return sdt_action_term_mul(gen, node);

  case 23: /* Y → / F Y */
    return sdt_action_term_div(gen, node);

  case 24:       /* Y → ε */
    return true; /* No action needed for epsilon */

  case 25: /* F → ( E ) */
    return sdt_action_factor_paren(gen, node);

  case 26: /* F → id */
    return sdt_action_factor_id(gen, node);

  case 27: /* F → int8 */
    return sdt_action_factor_int8(gen, node);

  case 28: /* F → int10 */
    return sdt_action_factor_int10(gen, node);

  case 29: /* F → int16 */
    return sdt_action_factor_int16(gen, node);

  default:
    /* No semantic action for this production */
    return true;
  }
}

/**
 * @brief Semantic action for program with single statement list
 *
 * P → L T { P.code = L.code || T.code }
 */
bool sdt_action_program_1(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 2) {
    return false;
  }

  /* Program inherits code from statement list and tail - no specific action
   * needed */
  DEBUG_PRINT("Executed P → L T action");
  return true;
}

/**
 * @brief Semantic action for program with multiple statement lists
 *
 * T → P T { T.code = P.code || T.code }
 */
bool sdt_action_program_2(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 2) {
    return false;
  }

  /* Program concatenates code from both parts - no specific action needed */
  DEBUG_PRINT("Executed T → P T action");
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
 * @brief Semantic action for block statement
 *
 * S → begin L end { S.code = L.code }
 */
bool sdt_action_block(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 3) {
    return false;
  }

  /* Block statement inherits code from statement list - no specific action
   * needed */
  DEBUG_PRINT("Executed S → begin L end action");
  return true;
}

/**
 * @brief Semantic action for else part
 *
 * N → else S { N.code = S.code }
 */
bool sdt_action_else_part(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 2) {
    return false;
  }

  /* Else part inherits code from statement - no specific action needed */
  DEBUG_PRINT("Executed N → else S action");
  return true;
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
  const char *var_name = id_node->token.str_val;

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
                       0                                      /* line number */
  );

  DEBUG_PRINT("Generated assignment: %s := %s", var_name, expr_place);
  return true;
}

/**
 * @brief Semantic action for if-else statement
 *
 * S → if C then S N
 * {
 *   C.true = newlabel;
 *   C.false = newlabel;
 *   S.next = newlabel;
 *   S.code = C.code || gen(C.true ':') || S1.code
 *          || gen('goto' S.next) || gen(C.false ':')
 *          || N.code || gen(S.next ':');
 * }
 */
bool sdt_action_if_else(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 4) {
    return false;
  }

  /* Get children */
  SyntaxTreeNode *cond_node = node->children[1];      /* C */
  SyntaxTreeNode *then_node = node->children[3];      /* S1 */
  SyntaxTreeNode *else_part_node = node->children[4]; /* N */

  if (!cond_node || !then_node || !else_part_node) {
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
 * @brief Semantic action for condition with parentheses
 *
 * C → ( C )
 */
bool sdt_action_condition_paren(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 3) {
    return false;
  }

  /* Get condition node */
  SyntaxTreeNode *cond_node = node->children[1]; /* C */

  if (!cond_node) {
    return false;
  }

  /* Get condition attributes */
  if (!cond_node->attributes) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Condition does not have valid attributes");
    return false;
  }

  /* Create attributes for this node if not exists */
  if (!node->attributes) {
    node->attributes = sdt_attributes_create();
    if (!node->attributes) {
      return false;
    }
  }

  /* Copy condition attributes to this node */
  if (cond_node->attributes->true_label) {
    if (node->attributes->true_label)
      free(node->attributes->true_label);
    node->attributes->true_label =
        safe_strdup(cond_node->attributes->true_label);
  }

  if (cond_node->attributes->false_label) {
    if (node->attributes->false_label)
      free(node->attributes->false_label);
    node->attributes->false_label =
        safe_strdup(cond_node->attributes->false_label);
  }

  DEBUG_PRINT("Executed C → ( C ) action");
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
 * @brief Semantic action for greater-than-or-equal condition
 *
 * C → E1 >= E2
 */
bool sdt_action_condition_ge(SDTCodeGen *gen, SyntaxTreeNode *node) {
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
      gen->program, TAC_OP_GE,
      node->attributes->true_label,  /* goto this label if condition is true */
      left_node->attributes->place,  /* left operand */
      right_node->attributes->place, /* right operand */
      0                              /* line number not available */
  );

  /* Generate jump to false label */
  tac_program_add_inst(gen->program, TAC_OP_GOTO, node->attributes->false_label,
                       NULL, NULL, 0 /* line number not available */
  );

  DEBUG_PRINT("Generated condition E >= E with jumps to: true=%s, false=%s",
              node->attributes->true_label, node->attributes->false_label);
  return true;
}

/**
 * @brief Semantic action for less-than-or-equal condition
 *
 * C → E1 <= E2
 */
bool sdt_action_condition_le(SDTCodeGen *gen, SyntaxTreeNode *node) {
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
      gen->program, TAC_OP_LE,
      node->attributes->true_label,  /* goto this label if condition is true */
      left_node->attributes->place,  /* left operand */
      right_node->attributes->place, /* right operand */
      0                              /* line number not available */
  );

  /* Generate jump to false label */
  tac_program_add_inst(gen->program, TAC_OP_GOTO, node->attributes->false_label,
                       NULL, NULL, 0 /* line number not available */
  );

  DEBUG_PRINT("Generated condition E <= E with jumps to: true=%s, false=%s",
              node->attributes->true_label, node->attributes->false_label);
  return true;
}

/**
 * @brief Semantic action for not-equal condition
 *
 * C → E1 <> E2
 */
bool sdt_action_condition_ne(SDTCodeGen *gen, SyntaxTreeNode *node) {
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
      gen->program, TAC_OP_NE,
      node->attributes->true_label,  /* goto this label if condition is true */
      left_node->attributes->place,  /* left operand */
      right_node->attributes->place, /* right operand */
      0                              /* line number not available */
  );

  /* Generate jump to false label */
  tac_program_add_inst(gen->program, TAC_OP_GOTO, node->attributes->false_label,
                       NULL, NULL, 0 /* line number not available */
  );

  DEBUG_PRINT("Generated condition E <> E with jumps to: true=%s, false=%s",
              node->attributes->true_label, node->attributes->false_label);
  return true;
}

/**
 * @brief Semantic action for addition expression
 *
 * X → + R X
 * {
 *   X.place = newtemp;
 *   X.code = R.code || gen(X.place ':=' X.prev_place '+' R.place) || X'.code
 * }
 */
bool sdt_action_expr_plus(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 3) {
    return false;
  }

  /* Get children */
  SyntaxTreeNode *plus_node = node->children[0]; /* + */
  SyntaxTreeNode *term_node = node->children[1]; /* R */
  SyntaxTreeNode *x_node = node->children[2];    /* X */

  if (!plus_node || !term_node) {
    return false;
  }

  /* Get parent node which should be E → R X */
  SyntaxTreeNode *parent = node->parent;
  if (!parent || parent->children_count < 2) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Parent expression node not found or invalid");
    return false;
  }

  /* Get R node from parent which has the left operand */
  SyntaxTreeNode *left_node = parent->children[0];
  if (!left_node || !left_node->attributes || !left_node->attributes->place) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Left operand does not have a valid place attribute");
    return false;
  }

  /* Get operand places */
  if (!term_node->attributes || !term_node->attributes->place) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Right operand does not have a valid place attribute");
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
                       node->attributes->place,      /* result */
                       left_node->attributes->place, /* left operand */
                       term_node->attributes->place, /* right operand */
                       0 /* line number not available */
  );

  DEBUG_PRINT("Generated addition: %s := %s + %s", node->attributes->place,
              left_node->attributes->place, term_node->attributes->place);

  /* Pass the result to parent expression node */
  if (parent->attributes) {
    if (parent->attributes->place) {
      free(parent->attributes->place);
    }
    parent->attributes->place = safe_strdup(node->attributes->place);
  } else {
    parent->attributes = sdt_attributes_create();
    if (parent->attributes) {
      parent->attributes->place = safe_strdup(node->attributes->place);
    }
  }

  return true;
}

/**
 * @brief Semantic action for subtraction expression
 *
 * X → - R X
 * {
 *   X.place = newtemp;
 *   X.code = R.code || gen(X.place ':=' X.prev_place '-' R.place) || X'.code
 * }
 */
bool sdt_action_expr_minus(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 3) {
    return false;
  }

  /* Get children */
  SyntaxTreeNode *minus_node = node->children[0]; /* - */
  SyntaxTreeNode *term_node = node->children[1];  /* R */
  SyntaxTreeNode *x_node = node->children[2];     /* X */

  if (!minus_node || !term_node) {
    return false;
  }

  /* Get parent node which should be E → R X */
  SyntaxTreeNode *parent = node->parent;
  if (!parent || parent->children_count < 2) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Parent expression node not found or invalid");
    return false;
  }

  /* Get R node from parent which has the left operand */
  SyntaxTreeNode *left_node = parent->children[0];
  if (!left_node || !left_node->attributes || !left_node->attributes->place) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Left operand does not have a valid place attribute");
    return false;
  }

  /* Get operand places */
  if (!term_node->attributes || !term_node->attributes->place) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Right operand does not have a valid place attribute");
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
                       node->attributes->place,      /* result */
                       left_node->attributes->place, /* left operand */
                       term_node->attributes->place, /* right operand */
                       0 /* line number not available */
  );

  DEBUG_PRINT("Generated subtraction: %s := %s - %s", node->attributes->place,
              left_node->attributes->place, term_node->attributes->place);

  /* Pass the result to parent expression node */
  if (parent->attributes) {
    if (parent->attributes->place) {
      free(parent->attributes->place);
    }
    parent->attributes->place = safe_strdup(node->attributes->place);
  } else {
    parent->attributes = sdt_attributes_create();
    if (parent->attributes) {
      parent->attributes->place = safe_strdup(node->attributes->place);
    }
  }

  return true;
}

/**
 * @brief Semantic action for expression from term
 *
 * E → R X { E.place = R.place if X.place is null, else E.place = X.place }
 */
bool sdt_action_expr_term(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 2) {
    return false;
  }

  /* Get term node */
  SyntaxTreeNode *term_node = node->children[0]; /* R */
  SyntaxTreeNode *x_node = node->children[1];    /* X */

  if (!term_node) {
    return false;
  }

  /* Create attributes for expression if not exists */
  if (!node->attributes) {
    node->attributes = sdt_attributes_create();
    if (!node->attributes) {
      return false;
    }
  }

  /* Check if X node has a place (indicating an operation was performed) */
  if (x_node && x_node->attributes && x_node->attributes->place) {
    /* Copy X place to expression place */
    if (node->attributes->place) {
      free(node->attributes->place);
    }
    node->attributes->place = safe_strdup(x_node->attributes->place);
    DEBUG_PRINT("Expression inherited place from X: %s",
                node->attributes->place);
  } else if (term_node->attributes && term_node->attributes->place) {
    /* Copy term place to expression place */
    if (node->attributes->place) {
      free(node->attributes->place);
    }
    node->attributes->place = safe_strdup(term_node->attributes->place);
    DEBUG_PRINT("Expression inherited place from term: %s",
                node->attributes->place);
  } else {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Neither term nor X has a valid place attribute");
    return false;
  }

  return true;
}

/**
 * @brief Semantic action for term from factor and term tail
 *
 * R → F Y { R.place = F.place if Y.place is null, else R.place = Y.place }
 */
bool sdt_action_term_factor(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 2) {
    return false;
  }

  /* Get factor node */
  SyntaxTreeNode *factor_node = node->children[0]; /* F */
  SyntaxTreeNode *y_node = node->children[1];      /* Y */

  if (!factor_node) {
    return false;
  }

  /* Create attributes for term if not exists */
  if (!node->attributes) {
    node->attributes = sdt_attributes_create();
    if (!node->attributes) {
      return false;
    }
  }

  /* Check if Y node has a place (indicating an operation was performed) */
  if (y_node && y_node->attributes && y_node->attributes->place) {
    /* Copy Y place to term place */
    if (node->attributes->place) {
      free(node->attributes->place);
    }
    node->attributes->place = safe_strdup(y_node->attributes->place);
    DEBUG_PRINT("Term inherited place from Y: %s", node->attributes->place);
  } else if (factor_node->attributes && factor_node->attributes->place) {
    /* Copy factor place to term place */
    if (node->attributes->place) {
      free(node->attributes->place);
    }
    node->attributes->place = safe_strdup(factor_node->attributes->place);
    DEBUG_PRINT("Term inherited place from factor: %s",
                node->attributes->place);
  } else {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Neither factor nor Y has a valid place attribute");
    return false;
  }

  return true;
}

/**
 * @brief Semantic action for multiplication term
 *
 * Y → * F Y
 * {
 *   Y.place = newtemp;
 *   Y.code = F.code || gen(Y.place ':=' Y.prev_place '*' F.place) || Y'.code
 * }
 */
bool sdt_action_term_mul(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 3) {
    return false;
  }

  /* Get children */
  SyntaxTreeNode *mul_node = node->children[0];    /* * */
  SyntaxTreeNode *factor_node = node->children[1]; /* F */
  SyntaxTreeNode *y_node = node->children[2];      /* Y */

  if (!mul_node || !factor_node) {
    return false;
  }

  /* Get parent node which should be R → F Y */
  SyntaxTreeNode *parent = node->parent;
  if (!parent || parent->children_count < 2) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Parent term node not found or invalid");
    return false;
  }

  /* Get F node from parent which has the left operand */
  SyntaxTreeNode *left_node = parent->children[0];
  if (!left_node || !left_node->attributes || !left_node->attributes->place) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Left operand does not have a valid place attribute");
    return false;
  }

  /* Get operand places */
  if (!factor_node->attributes || !factor_node->attributes->place) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Right operand does not have a valid place attribute");
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
                       node->attributes->place,        /* result */
                       left_node->attributes->place,   /* left operand */
                       factor_node->attributes->place, /* right operand */
                       0 /* line number not available */
  );

  DEBUG_PRINT("Generated multiplication: %s := %s * %s",
              node->attributes->place, left_node->attributes->place,
              factor_node->attributes->place);

  /* Pass the result to parent term node */
  if (parent->attributes) {
    if (parent->attributes->place) {
      free(parent->attributes->place);
    }
    parent->attributes->place = safe_strdup(node->attributes->place);
  } else {
    parent->attributes = sdt_attributes_create();
    if (parent->attributes) {
      parent->attributes->place = safe_strdup(node->attributes->place);
    }
  }

  return true;
}

/**
 * @brief Semantic action for division term
 *
 * Y → / F Y
 * {
 *   Y.place = newtemp;
 *   Y.code = F.code || gen(Y.place ':=' Y.prev_place '/' F.place) || Y'.code
 * }
 */
bool sdt_action_term_div(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node || node->children_count < 3) {
    return false;
  }

  /* Get children */
  SyntaxTreeNode *div_node = node->children[0];    /* / */
  SyntaxTreeNode *factor_node = node->children[1]; /* F */
  SyntaxTreeNode *y_node = node->children[2];      /* Y */

  if (!div_node || !factor_node) {
    return false;
  }

  /* Get parent node which should be R → F Y */
  SyntaxTreeNode *parent = node->parent;
  if (!parent || parent->children_count < 2) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Parent term node not found or invalid");
    return false;
  }

  /* Get F node from parent which has the left operand */
  SyntaxTreeNode *left_node = parent->children[0];
  if (!left_node || !left_node->attributes || !left_node->attributes->place) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Left operand does not have a valid place attribute");
    return false;
  }

  /* Get operand places */
  if (!factor_node->attributes || !factor_node->attributes->place) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Right operand does not have a valid place attribute");
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
                       node->attributes->place,        /* result */
                       left_node->attributes->place,   /* left operand */
                       factor_node->attributes->place, /* right operand */
                       0 /* line number not available */
  );

  DEBUG_PRINT("Generated division: %s := %s / %s", node->attributes->place,
              left_node->attributes->place, factor_node->attributes->place);

  /* Pass the result to parent term node */
  if (parent->attributes) {
    if (parent->attributes->place) {
      free(parent->attributes->place);
    }
    parent->attributes->place = safe_strdup(node->attributes->place);
  } else {
    parent->attributes = sdt_attributes_create();
    if (parent->attributes) {
      parent->attributes->place = safe_strdup(node->attributes->place);
    }
  }

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
  const char *var_name = id_node->token.str_val;

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
  char int_value_str[64];
  if (!safe_itoa(int_node->token.num_val, int_value_str, sizeof(int_value_str),
                 10)) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Failed to convert integer value to string");
    return false;
  }
  const char *int_value = int_value_str;

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
  char int_value_str[64];
  if (!safe_itoa(int_node->token.num_val, int_value_str, sizeof(int_value_str),
                 10)) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Failed to convert integer value to string");
    return false;
  }
  const char *int_value = int_value_str;

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
  char int_value_str[64];
  if (!safe_itoa(int_node->token.num_val, int_value_str, sizeof(int_value_str),
                 10)) {
    gen->has_error = true;
    snprintf(gen->error_message, sizeof(gen->error_message),
             "Failed to convert integer value to string");
    return false;
  }
  const char *int_value = int_value_str;

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
