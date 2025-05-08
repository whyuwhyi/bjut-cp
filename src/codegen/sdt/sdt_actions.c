/**
 * @file codegen/sdt/sdt_actions.c
 * @brief Implementation of semantic actions for syntax-directed translation
 */

#include "sdt_actions.h"
#include "codegen/sdt_codegen.h"
#include "codegen/tac.h"
#include "label_manager/label_manager.h"
#include "sdt_attributes.h"
#include "symbol_table/symbol_table.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Production IDs from grammar
 */
enum {
  PROD_P_LT = 0,
  PROD_T_PT = 1,
  PROD_T_EPSILON = 2,
  PROD_L_S_SEMI = 3,
  PROD_S_ASSIGN = 4,
  PROD_S_IF_C_THEN_S_N = 5,
  PROD_S_WHILE_C_DO_S = 6,
  PROD_S_BEGIN_L_END = 7,
  PROD_N_ELSE_S = 8,
  PROD_N_EPSILON = 9,
  PROD_C_GT = 10,
  PROD_C_LT = 11,
  PROD_C_EQ = 12,
  PROD_C_GE = 13,
  PROD_C_LE = 14,
  PROD_C_NE = 15,
  PROD_C_PAREN = 16,
  PROD_E_R_X = 17,
  PROD_X_PLUS_R_X = 18,
  PROD_X_MINUS_R_X = 19,
  PROD_X_EPSILON = 20,
  PROD_R_F_Y = 21,
  PROD_Y_MUL_F_Y = 22,
  PROD_Y_DIV_F_Y = 23,
  PROD_Y_EPSILON = 24,
  PROD_F_PAREN = 25,
  PROD_F_ID = 26,
  PROD_F_INT8 = 27,
  PROD_F_INT10 = 28,
  PROD_F_INT16 = 29
};

/* Forward declarations for all semantic actions */
static bool action_P_LT(SDTCodeGen *, SyntaxTreeNode *);
static bool action_T_PT(SDTCodeGen *, SyntaxTreeNode *);
static bool action_T_EPS(SDTCodeGen *, SyntaxTreeNode *);
static bool action_L_S_SEMI(SDTCodeGen *, SyntaxTreeNode *);
static bool action_S_ASSIGN(SDTCodeGen *, SyntaxTreeNode *);
static bool action_S_IF(SDTCodeGen *, SyntaxTreeNode *);
static bool action_S_WHILE(SDTCodeGen *, SyntaxTreeNode *);
static bool action_S_BEGIN(SDTCodeGen *, SyntaxTreeNode *);
static bool action_N_ELSE(SDTCodeGen *, SyntaxTreeNode *);
static bool action_N_EPS(SDTCodeGen *, SyntaxTreeNode *);
static bool action_C_RELOP(SDTCodeGen *, SyntaxTreeNode *, TACOpType);
static bool action_C_PAREN(SDTCodeGen *, SyntaxTreeNode *);
static bool action_E_R_X(SDTCodeGen *, SyntaxTreeNode *);
static bool action_X_PLUS(SDTCodeGen *, SyntaxTreeNode *);
static bool action_X_MINUS(SDTCodeGen *, SyntaxTreeNode *);
static bool action_X_EPS(SDTCodeGen *, SyntaxTreeNode *);
static bool action_R_F_Y(SDTCodeGen *, SyntaxTreeNode *);
static bool action_Y_MUL(SDTCodeGen *, SyntaxTreeNode *);
static bool action_Y_DIV(SDTCodeGen *, SyntaxTreeNode *);
static bool action_Y_EPS(SDTCodeGen *, SyntaxTreeNode *);
static bool action_F_PAREN(SDTCodeGen *, SyntaxTreeNode *);
static bool action_F_ID(SDTCodeGen *, SyntaxTreeNode *);
static bool action_F_INT(SDTCodeGen *, SyntaxTreeNode *);

/**
 * @brief Execute the semantic actions for a node based on its production ID
 */
bool sdt_execute_action(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node)
    return false;

  /* Ensure node has attributes */
  if (!node->attributes) {
    node->attributes = sdt_attributes_create();
    if (!node->attributes) {
      return false;
    }
  }

  /* Dispatch to appropriate action based on production ID */
  switch (node->production_id) {
  case PROD_P_LT:
    return action_P_LT(gen, node);
  case PROD_T_PT:
    return action_T_PT(gen, node);
  case PROD_T_EPSILON:
    return action_T_EPS(gen, node);
  case PROD_L_S_SEMI:
    return action_L_S_SEMI(gen, node);
  case PROD_S_ASSIGN:
    return action_S_ASSIGN(gen, node);
  case PROD_S_IF_C_THEN_S_N:
    return action_S_IF(gen, node);
  case PROD_S_WHILE_C_DO_S:
    return action_S_WHILE(gen, node);
  case PROD_S_BEGIN_L_END:
    return action_S_BEGIN(gen, node);
  case PROD_N_ELSE_S:
    return action_N_ELSE(gen, node);
  case PROD_N_EPSILON:
    return action_N_EPS(gen, node);
  case PROD_C_GT:
    return action_C_RELOP(gen, node, TAC_OP_GT);
  case PROD_C_LT:
    return action_C_RELOP(gen, node, TAC_OP_LT);
  case PROD_C_EQ:
    return action_C_RELOP(gen, node, TAC_OP_EQ);
  case PROD_C_GE:
    return action_C_RELOP(gen, node, TAC_OP_GE);
  case PROD_C_LE:
    return action_C_RELOP(gen, node, TAC_OP_LE);
  case PROD_C_NE:
    return action_C_RELOP(gen, node, TAC_OP_NE);
  case PROD_C_PAREN:
    return action_C_PAREN(gen, node);
  case PROD_E_R_X:
    return action_E_R_X(gen, node);
  case PROD_X_PLUS_R_X:
    return action_X_PLUS(gen, node);
  case PROD_X_MINUS_R_X:
    return action_X_MINUS(gen, node);
  case PROD_X_EPSILON:
    return action_X_EPS(gen, node);
  case PROD_R_F_Y:
    return action_R_F_Y(gen, node);
  case PROD_Y_MUL_F_Y:
    return action_Y_MUL(gen, node);
  case PROD_Y_DIV_F_Y:
    return action_Y_DIV(gen, node);
  case PROD_Y_EPSILON:
    return action_Y_EPS(gen, node);
  case PROD_F_PAREN:
    return action_F_PAREN(gen, node);
  case PROD_F_ID:
    return action_F_ID(gen, node);
  case PROD_F_INT8:
  case PROD_F_INT10:
  case PROD_F_INT16:
    return action_F_INT(gen, node);
  default:
    return true;
  }
}

/**
 * @brief Semantic action for P → L T
 *
 * P.code = L.code || T.code
 */
static bool action_P_LT(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* Process children: [0]=L, [1]=T */
  sdt_codegen_generate(gen, node->children[0]);
  sdt_codegen_generate(gen, node->children[1]);

  DEBUG_PRINT("Executed P → L T action");
  return true;
}

/**
 * @brief Semantic action for T → P T
 *
 * T.code = P.code || T.code
 */
static bool action_T_PT(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* Process children: [0]=P, [1]=T */
  sdt_codegen_generate(gen, node->children[0]);
  sdt_codegen_generate(gen, node->children[1]);

  DEBUG_PRINT("Executed T → P T action");
  return true;
}

/**
 * @brief Semantic action for T → ε
 *
 * T.code = ''
 */
static bool action_T_EPS(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* No code to generate for epsilon */
  DEBUG_PRINT("Executed T → ε action");
  return true;
}

/**
 * @brief Semantic action for L → S ;
 *
 * L.code = S.code
 */
static bool action_L_S_SEMI(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* Process statement: [0]=S */
  sdt_codegen_generate(gen, node->children[0]);

  DEBUG_PRINT("Executed L → S ; action");
  return true;
}

/**
 * @brief Semantic action for S → id = E
 *
 * S.code = E.code || gen(id.place ':=' E.place)
 */
static bool action_S_ASSIGN(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* Get children: [0]=id, [1]='=', [2]=E */
  SyntaxTreeNode *id_node = node->children[0];
  SyntaxTreeNode *E_node = node->children[2];

  /* Generate code for the expression */
  sdt_codegen_generate(gen, E_node);

  /* Generate assignment instruction */
  tac_program_add_inst(gen->program, TAC_OP_ASSIGN,
                       id_node->token.str_val,    /* destination */
                       E_node->attributes->place, /* source */
                       NULL, 0);

  DEBUG_PRINT("Generated assignment: %s := %s", id_node->token.str_val,
              E_node->attributes->place);
  return true;
}

/**
 * @brief Semantic action for S → if C then S1 N
 *
 * If N is epsilon:
 *   C.true = newlabel;
 *   C.false = S.next;
 *   S1.next = S.next;
 *   S.code = C.code || gen(C.true ':') || S1.code;
 * Else:
 *   C.true = newlabel;
 *   C.false = newlabel;
 *   S1.next = S.next;
 *   N.stmt.next = S.next;
 *   S.code = C.code || gen(C.true ':') || S1.code ||
 *            gen('goto' S.next) || gen(C.false ':') || N.code;
 */
static bool action_S_IF(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* Get children: [0]='if', [1]=C, [2]='then', [3]=S1, [4]=N */
  SyntaxTreeNode *C_node = node->children[1];
  SyntaxTreeNode *S1_node = node->children[3];
  SyntaxTreeNode *N_node = node->children[4];

  /* Ensure condition node has attributes */
  if (!C_node->attributes) {
    C_node->attributes = sdt_attributes_create();
    if (!C_node->attributes) {
      return false;
    }
  }

  /* Generate true/false labels for condition */
  char *true_label = label_manager_new_label(gen->label_manager);
  C_node->attributes->true_label = true_label;

  /* Check if there's an else part */
  bool has_else = (N_node->production_id == PROD_N_ELSE_S);

  /* Get or create next label */
  char *next_label = NULL;
  if (node->attributes->next_label) {
    next_label = safe_strdup(node->attributes->next_label);
  } else {
    next_label = label_manager_new_label(gen->label_manager);
  }

  /* Set false label based on whether there's an else part */
  char *false_label = NULL;
  if (has_else) {
    false_label = label_manager_new_label(gen->label_manager);
  } else {
    false_label = safe_strdup(next_label);
  }
  C_node->attributes->false_label = false_label;

  /* Generate code for the condition */
  sdt_codegen_generate(gen, C_node);

  /* Generate true label for then branch */
  tac_program_add_inst(gen->program, TAC_OP_LABEL, true_label, NULL, NULL, 0);

  /* Pass next_label to then branch */
  if (!S1_node->attributes) {
    S1_node->attributes = sdt_attributes_create();
  }
  S1_node->attributes->next_label = safe_strdup(next_label);

  /* Generate code for then branch */
  sdt_codegen_generate(gen, S1_node);

  if (has_else) {
    /* Add jump to next after then branch */
    tac_program_add_inst(gen->program, TAC_OP_GOTO, next_label, NULL, NULL, 0);

    /* Generate false label for else branch */
    tac_program_add_inst(gen->program, TAC_OP_LABEL, false_label, NULL, NULL,
                         0);

    /* Pass next_label to else branch's statement */
    SyntaxTreeNode *else_stmt = N_node->children[1];
    if (!else_stmt->attributes) {
      else_stmt->attributes = sdt_attributes_create();
    }
    else_stmt->attributes->next_label = safe_strdup(next_label);

    /* Generate code for else branch */
    sdt_codegen_generate(gen, else_stmt);

    /* Generate next label after else branch */
    tac_program_add_inst(gen->program, TAC_OP_LABEL, next_label, NULL, NULL, 0);
  } else {
    /* Generate false label (which is the same as next_label) */
    tac_program_add_inst(gen->program, TAC_OP_LABEL, false_label, NULL, NULL,
                         0);
  }

  /* Clean up */
  if (next_label != node->attributes->next_label) {
    free(next_label);
  }

  DEBUG_PRINT(
      "Generated if-else statement with labels: true=%s, false=%s, next=%s",
      true_label, false_label, next_label);
  return true;
}

/**
 * @brief Semantic action for S → while C do S1
 *
 * S.begin = newlabel;
 * C.true = newlabel;
 * C.false = S.next;
 * S1.next = S.begin;
 * S.code = gen(S.begin ':') || C.code || gen(C.true ':') || S1.code ||
 * gen('goto' S.begin)
 */
static bool action_S_WHILE(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* Get children: [0]='while', [1]=C, [2]='do', [3]=S1 */
  SyntaxTreeNode *C_node = node->children[1];
  SyntaxTreeNode *S1_node = node->children[3];

  /* Ensure condition node has attributes */
  if (!C_node->attributes) {
    C_node->attributes = sdt_attributes_create();
  }

  /* Get or create next label */
  char *next_label = NULL;
  if (node->attributes->next_label) {
    next_label = safe_strdup(node->attributes->next_label);
  } else {
    next_label = label_manager_new_label(gen->label_manager);
  }

  /* Create begin label for loop */
  char *begin_label = label_manager_new_label(gen->label_manager);
  node->attributes->begin_label = begin_label;

  /* Create true label for condition */
  char *true_label = label_manager_new_label(gen->label_manager);
  C_node->attributes->true_label = true_label;

  /* Set false label to next label */
  C_node->attributes->false_label = safe_strdup(next_label);

  /* Generate begin label at loop start */
  tac_program_add_inst(gen->program, TAC_OP_LABEL, begin_label, NULL, NULL, 0);

  /* Generate code for condition */
  sdt_codegen_generate(gen, C_node);

  /* Generate true label for loop body */
  tac_program_add_inst(gen->program, TAC_OP_LABEL, true_label, NULL, NULL, 0);

  /* Pass begin_label as next_label to loop body */
  if (!S1_node->attributes) {
    S1_node->attributes = sdt_attributes_create();
  }
  S1_node->attributes->next_label = safe_strdup(begin_label);

  /* Generate code for loop body */
  sdt_codegen_generate(gen, S1_node);

  /* Generate goto back to begin label */
  tac_program_add_inst(gen->program, TAC_OP_GOTO, begin_label, NULL, NULL, 0);

  /* Generate false label after loop */
  tac_program_add_inst(gen->program, TAC_OP_LABEL, next_label, NULL, NULL, 0);

  /* Clean up */
  if (next_label != node->attributes->next_label) {
    free(next_label);
  }

  DEBUG_PRINT(
      "Generated while statement with labels: begin=%s, true=%s, false=%s",
      begin_label, true_label, next_label);
  return true;
}

/**
 * @brief Semantic action for S → begin L end
 *
 * S.code = L.code
 */
static bool action_S_BEGIN(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* Get child: [0]='begin', [1]=L, [2]='end' */
  SyntaxTreeNode *L_node = node->children[1];

  /* Pass next_label down to L */
  if (!L_node->attributes) {
    L_node->attributes = sdt_attributes_create();
  }
  if (node->attributes && node->attributes->next_label) {
    L_node->attributes->next_label = safe_strdup(node->attributes->next_label);
  }

  /* Generate code for statement list */
  sdt_codegen_generate(gen, L_node);

  DEBUG_PRINT("Executed S → begin L end action");
  return true;
}

/**
 * @brief Semantic action for N → else S
 *
 * N.isEpsilon = false;
 * N.code = S.code;
 * N.stmt = S;
 */
static bool action_N_ELSE(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* This is handled in the if-statement action */
  DEBUG_PRINT("Executed N → else S action");
  return true;
}

/**
 * @brief Semantic action for N → ε
 *
 * N.isEpsilon = true;
 * N.code = '';
 */
static bool action_N_EPS(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* No code to generate for epsilon */
  DEBUG_PRINT("Executed N → ε action");
  return true;
}

/**
 * @brief Semantic action for C → E1 op E2
 *
 * C.code = E1.code || E2.code ||
 *          gen('if' E1.place 'op' E2.place 'goto' C.true) ||
 *          gen('goto' C.false)
 */
static bool action_C_RELOP(SDTCodeGen *gen, SyntaxTreeNode *node,
                           TACOpType op) {
  /* Get children: [0]=E1, [1]=op, [2]=E2 */
  SyntaxTreeNode *E1_node = node->children[0];
  SyntaxTreeNode *E2_node = node->children[2];

  /* Generate code for expressions */
  sdt_codegen_generate(gen, E1_node);
  sdt_codegen_generate(gen, E2_node);

  /* Get or create true/false labels */
  char *true_label = NULL;
  char *false_label = NULL;

  if (node->attributes->true_label) {
    true_label = node->attributes->true_label;
  } else {
    true_label = label_manager_new_label(gen->label_manager);
    node->attributes->true_label = true_label;
  }

  if (node->attributes->false_label) {
    false_label = node->attributes->false_label;
  } else {
    false_label = label_manager_new_label(gen->label_manager);
    node->attributes->false_label = false_label;
  }

  /* Generate conditional jump */
  tac_program_add_inst(gen->program, op, true_label, E1_node->attributes->place,
                       E2_node->attributes->place, 0);

  /* Generate jump to false label */
  tac_program_add_inst(gen->program, TAC_OP_GOTO, false_label, NULL, NULL, 0);

  DEBUG_PRINT("Generated condition with relational operator: %s",
              tac_op_type_to_string(op));
  return true;
}

/**
 * @brief Semantic action for C → ( C1 )
 *
 * C.code = C1.code;
 * C1.true = C.true;
 * C1.false = C.false;
 */
static bool action_C_PAREN(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* Get child: [0]='(', [1]=C1, [2]=')' */
  SyntaxTreeNode *C1_node = node->children[1];

  /* Pass true/false labels down to inner condition */
  if (!C1_node->attributes) {
    C1_node->attributes = sdt_attributes_create();
  }

  if (node->attributes->true_label) {
    C1_node->attributes->true_label = safe_strdup(node->attributes->true_label);
  }

  if (node->attributes->false_label) {
    C1_node->attributes->false_label =
        safe_strdup(node->attributes->false_label);
  }

  /* Generate code for inner condition */
  sdt_codegen_generate(gen, C1_node);

  /* Inherit true/false labels from inner condition */
  if (C1_node->attributes->true_label && !node->attributes->true_label) {
    node->attributes->true_label = safe_strdup(C1_node->attributes->true_label);
  }

  if (C1_node->attributes->false_label && !node->attributes->false_label) {
    node->attributes->false_label =
        safe_strdup(C1_node->attributes->false_label);
  }

  DEBUG_PRINT("Executed C → ( C1 ) action");
  return true;
}

/**
 * @brief Semantic action for E → R X
 *
 * E.place = X.synthesized;
 * E.code = R.code || X.code;
 * X.inherited = R.place;
 */
static bool action_E_R_X(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* Get children: [0]=R, [1]=X */
  SyntaxTreeNode *R_node = node->children[0];
  SyntaxTreeNode *X_node = node->children[1];

  /* Generate code for term */
  sdt_codegen_generate(gen, R_node);

  /* Pass R.place to X as inherited attribute */
  if (!X_node->attributes) {
    X_node->attributes = sdt_attributes_create();
  }
  X_node->attributes->place = safe_strdup(R_node->attributes->place);

  /* Generate code for expression tail */
  sdt_codegen_generate(gen, X_node);

  /* Inherit synthesized place from X */
  node->attributes->place = safe_strdup(X_node->attributes->place);

  DEBUG_PRINT("Executed E → R X action: place = %s", node->attributes->place);
  return true;
}

/**
 * @brief Semantic action for X → + R X1
 *
 * X.synthesized = newtemp;
 * X.code = R.code || gen(X.synthesized ':=' X.inherited '+' R.place) ||
 * X1.code; X1.inherited = X.synthesized;
 */
static bool action_X_PLUS(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* Get children: [0]='+', [1]=R, [2]=X1 */
  SyntaxTreeNode *R_node = node->children[1];
  SyntaxTreeNode *X1_node = node->children[2];

  /* Generate code for term */
  sdt_codegen_generate(gen, R_node);

  /* Create temporary for result */
  char *temp = symbol_table_new_temp(gen->symbol_table);
  if (!temp) {
    return false;
  }

  /* Generate addition instruction */
  tac_program_add_inst(gen->program, TAC_OP_ADD, temp, /* result */
                       node->attributes->place,   /* inherited from parent */
                       R_node->attributes->place, /* right operand */
                       0);

  /* Pass temp to X1 as inherited attribute */
  if (!X1_node->attributes) {
    X1_node->attributes = sdt_attributes_create();
  }
  X1_node->attributes->place = safe_strdup(temp);

  /* Generate code for expression tail */
  sdt_codegen_generate(gen, X1_node);

  /* Inherit synthesized place from X1 */
  node->attributes->place = safe_strdup(X1_node->attributes->place);

  /* Clean up */
  free(temp);

  DEBUG_PRINT("Generated addition: %s := %s + %s", temp,
              node->attributes->place, R_node->attributes->place);
  return true;
}

/**
 * @brief Semantic action for X → - R X1
 *
 * X.synthesized = newtemp;
 * X.code = R.code || gen(X.synthesized ':=' X.inherited '-' R.place) ||
 * X1.code; X1.inherited = X.synthesized;
 */
static bool action_X_MINUS(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* Get children: [0]='-', [1]=R, [2]=X1 */
  SyntaxTreeNode *R_node = node->children[1];
  SyntaxTreeNode *X1_node = node->children[2];

  /* Generate code for term */
  sdt_codegen_generate(gen, R_node);

  /* Create temporary for result */
  char *temp = symbol_table_new_temp(gen->symbol_table);
  if (!temp) {
    return false;
  }

  /* Generate subtraction instruction */
  tac_program_add_inst(gen->program, TAC_OP_SUB, temp, /* result */
                       node->attributes->place,   /* inherited from parent */
                       R_node->attributes->place, /* right operand */
                       0);

  /* Pass temp to X1 as inherited attribute */
  if (!X1_node->attributes) {
    X1_node->attributes = sdt_attributes_create();
  }
  X1_node->attributes->place = safe_strdup(temp);

  /* Generate code for expression tail */
  sdt_codegen_generate(gen, X1_node);

  /* Inherit synthesized place from X1 */
  node->attributes->place = safe_strdup(X1_node->attributes->place);

  /* Clean up */
  free(temp);

  DEBUG_PRINT("Generated subtraction: %s := %s - %s", temp,
              node->attributes->place, R_node->attributes->place);
  return true;
}

/**
 * @brief Semantic action for X → ε
 *
 * X.synthesized = X.inherited;
 * X.code = '';
 */
static bool action_X_EPS(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* X.synthesized = X.inherited (place already inherited) */
  DEBUG_PRINT("Executed X → ε action: place = %s", node->attributes->place);
  return true;
}

/**
 * @brief Semantic action for R → F Y
 *
 * R.place = Y.synthesized;
 * R.code = F.code || Y.code;
 * Y.inherited = F.place;
 */
static bool action_R_F_Y(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* Get children: [0]=F, [1]=Y */
  SyntaxTreeNode *F_node = node->children[0];
  SyntaxTreeNode *Y_node = node->children[1];

  /* Generate code for factor */
  sdt_codegen_generate(gen, F_node);

  /* Pass F.place to Y as inherited attribute */
  if (!Y_node->attributes) {
    Y_node->attributes = sdt_attributes_create();
  }
  Y_node->attributes->place = safe_strdup(F_node->attributes->place);

  /* Generate code for term tail */
  sdt_codegen_generate(gen, Y_node);

  /* Inherit synthesized place from Y */
  node->attributes->place = safe_strdup(Y_node->attributes->place);

  DEBUG_PRINT("Executed R → F Y action: place = %s", node->attributes->place);
  return true;
}

/**
 * @brief Semantic action for Y → * F Y1
 *
 * Y.synthesized = newtemp;
 * Y.code = F.code || gen(Y.synthesized ':=' Y.inherited '*' F.place) ||
 * Y1.code; Y1.inherited = Y.synthesized;
 */
static bool action_Y_MUL(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* Get children: [0]='*', [1]=F, [2]=Y1 */
  SyntaxTreeNode *F_node = node->children[1];
  SyntaxTreeNode *Y1_node = node->children[2];

  /* Generate code for factor */
  sdt_codegen_generate(gen, F_node);

  /* Create temporary for result */
  char *temp = symbol_table_new_temp(gen->symbol_table);
  if (!temp) {
    return false;
  }

  /* Generate multiplication instruction */
  tac_program_add_inst(gen->program, TAC_OP_MUL, temp, /* result */
                       node->attributes->place,   /* inherited from parent */
                       F_node->attributes->place, /* right operand */
                       0);

  /* Pass temp to Y1 as inherited attribute */
  if (!Y1_node->attributes) {
    Y1_node->attributes = sdt_attributes_create();
  }
  Y1_node->attributes->place = safe_strdup(temp);

  /* Generate code for term tail */
  sdt_codegen_generate(gen, Y1_node);

  /* Inherit synthesized place from Y1 */
  node->attributes->place = safe_strdup(Y1_node->attributes->place);

  /* Clean up */
  free(temp);

  DEBUG_PRINT("Generated multiplication: %s := %s * %s", temp,
              node->attributes->place, F_node->attributes->place);
  return true;
}

/**
 * @brief Semantic action for Y → / F Y1
 *
 * Y.synthesized = newtemp;
 * Y.code = F.code || gen(Y.synthesized ':=' Y.inherited '/' F.place) ||
 * Y1.code; Y1.inherited = Y.synthesized;
 */
static bool action_Y_DIV(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* Get children: [0]='/', [1]=F, [2]=Y1 */
  SyntaxTreeNode *F_node = node->children[1];
  SyntaxTreeNode *Y1_node = node->children[2];

  /* Generate code for factor */
  sdt_codegen_generate(gen, F_node);

  /* Create temporary for result */
  char *temp = symbol_table_new_temp(gen->symbol_table);
  if (!temp) {
    return false;
  }

  /* Generate division instruction */
  tac_program_add_inst(gen->program, TAC_OP_DIV, temp, /* result */
                       node->attributes->place,   /* inherited from parent */
                       F_node->attributes->place, /* right operand */
                       0);

  /* Pass temp to Y1 as inherited attribute */
  if (!Y1_node->attributes) {
    Y1_node->attributes = sdt_attributes_create();
  }
  Y1_node->attributes->place = safe_strdup(temp);

  /* Generate code for term tail */
  sdt_codegen_generate(gen, Y1_node);

  /* Inherit synthesized place from Y1 */
  node->attributes->place = safe_strdup(Y1_node->attributes->place);

  /* Clean up */
  free(temp);

  DEBUG_PRINT("Generated division: %s := %s / %s", temp,
              node->attributes->place, F_node->attributes->place);
  return true;
}

/**
 * @brief Semantic action for Y → ε
 *
 * Y.synthesized = Y.inherited;
 * Y.code = '';
 */
static bool action_Y_EPS(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* Y.synthesized = Y.inherited (place already inherited) */
  DEBUG_PRINT("Executed Y → ε action: place = %s", node->attributes->place);
  return true;
}

/**
 * @brief Semantic action for F → ( E )
 *
 * F.place = E.place;
 * F.code = E.code;
 */
static bool action_F_PAREN(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* Get child: [0]='(', [1]=E, [2]=')' */
  SyntaxTreeNode *E_node = node->children[1];

  /* Generate code for expression */
  sdt_codegen_generate(gen, E_node);

  /* Inherit place from expression */
  node->attributes->place = safe_strdup(E_node->attributes->place);

  DEBUG_PRINT("Executed F → ( E ) action: place = %s", node->attributes->place);
  return true;
}

// /**
//  * @brief Semantic action for F → id
//  *
//  * F.place = id.name;
//  * F.code = '';
//  */
// static bool action_F_ID(SDTCodeGen *gen, SyntaxTreeNode *node) {
//   /* Get token from node */
//   const char *id_name = node->token.str_val;
//
//   /* Set place to identifier name */
//   node->attributes->place = safe_strdup(id_name);
//
//   DEBUG_PRINT("Factor place set to identifier: %s", node->attributes->place);
//   return true;
// }
//
static bool action_F_ID(SDTCodeGen *gen, SyntaxTreeNode *node) {
  DEBUG_PRINT("Processing F_ID node, token address: %p", &node->token);
  DEBUG_PRINT("Token type: %d, line: %d", node->token.type, 0);
  DEBUG_PRINT("Token str_val address: %p", node->token.str_val);

  const char *id_name = node->token.str_val;
  if (!id_name) {
    DEBUG_PRINT("CRITICAL: id_name is NULL");
    sdt_set_error(gen, "Null identifier at line %d", 0);
    return false;
  }

  if (id_name[0] == '\0') {
    DEBUG_PRINT("CRITICAL: id_name is empty string");
    sdt_set_error(gen, "Empty identifier at line %d", 0);
    return false;
  }

  node->attributes->place = safe_strdup(id_name);
  DEBUG_PRINT("Factor place set to identifier: %s", node->attributes->place);
  return true;
}

/**
 * @brief Semantic action for F → int8 | int10 | int16
 *
 * F.place = int.value;
 * F.code = '';
 */
static bool action_F_INT(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* Convert integer value to string */
  char value_str[64];
  int value = node->token.num_val;
  if (safe_itoa(value, value_str, 64, 10) == NULL) {
    fprintf(stderr, "Error converting integer to string\n");
    return false;
  }

  /* Set place to integer value */
  node->attributes->place = safe_strdup(value_str);

  DEBUG_PRINT("Factor place set to integer: %s", node->attributes->place);
  return true;
}
