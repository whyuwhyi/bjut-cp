/**
 * @file codegen/sdt/sdt_actions.c
 * @brief Implementation of semantic actions for syntax-directed translation
 */
#include "sdt_actions.h"
#include "codegen/sdt_codegen.h"
#include "codegen/tac.h"
#include "label_manager/label_manager.h"
#include "parser/grammar.h"
#include "sdt_attributes.h"
#include "symbol_table/symbol_table.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* Helper functions for common operations */
static bool ensure_attributes(SyntaxTreeNode *node);
static char *get_or_create_next_label(SDTCodeGen *gen, SyntaxTreeNode *node);
static bool is_control_structure(SyntaxTreeNode *node);
static SyntaxTreeNode *find_child_by_name(SyntaxTreeNode *node,
                                          const char *name, int node_type);

/**
 * @brief Execute the semantic actions for a node based on its production ID
 */
bool sdt_execute_action(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!gen || !node)
    return false;

  /* Ensure node has attributes */
  if (!ensure_attributes(node)) {
    return false;
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
 * @brief Ensures a node has attributes, creates them if needed
 * @return true if successful, false on allocation failure
 */
static bool ensure_attributes(SyntaxTreeNode *node) {
  if (!node->attributes) {
    node->attributes = sdt_attributes_create();
    if (!node->attributes) {
      return false;
    }
  }
  return true;
}

/**
 * @brief Get existing or create new next_label
 * @return The next_label string (caller must free if not storing in attributes)
 */
static char *get_or_create_next_label(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (node->attributes && node->attributes->next_label) {
    return safe_strdup(node->attributes->next_label);
  } else {
    return label_manager_new_label(gen->label_manager);
  }
}

/**
 * @brief Check if a node represents a control structure (if, while, block)
 * @return true if node is a control structure, false otherwise
 */
static bool is_control_structure(SyntaxTreeNode *node) {
  return (node->production_id == PROD_S_WHILE_C_DO_S ||
          node->production_id == PROD_S_IF_C_THEN_S_N ||
          node->production_id == PROD_S_BEGIN_L_END);
}

/**
 * @brief Find a child node by its symbol name and type
 * @return The matching child node, or NULL if not found
 */
static SyntaxTreeNode *find_child_by_name(SyntaxTreeNode *node,
                                          const char *name, int node_type) {
  if (!node || !name)
    return NULL;

  for (int i = 0; i < node->children_count; i++) {
    if (node->children[i]->type == node_type &&
        strcmp(node->children[i]->symbol_name, name) == 0) {
      return node->children[i];
    }
  }
  return NULL;
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
  /* Find id and E nodes */
  SyntaxTreeNode *id_node = find_child_by_name(node, "id", NODE_TERMINAL);
  SyntaxTreeNode *E_node = find_child_by_name(node, "E", NODE_NONTERMINAL);

  if (!id_node || !E_node) {
    DEBUG_PRINT("ERROR: Missing identifier or expression node");
    return false;
  }

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
 */
static bool action_S_IF(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!node || node->children_count < 5)
    return false;
  SyntaxTreeNode *C_node = node->children[1];  /* Condition */
  SyntaxTreeNode *S1_node = node->children[3]; /* Then branch */
  SyntaxTreeNode *N_node = node->children[4];  /* Else branch */

  /* Ensure attribute structures exist */
  if (!ensure_attributes(node) || !ensure_attributes(C_node)) {
    return false;
  }

  /* Check if next_label is inherited from parent node */
  bool inherited = (node->attributes->next_label != NULL);

  /* 1. Generate or reuse next_label */
  char *next_label = inherited ? safe_strdup(node->attributes->next_label)
                               : label_manager_new_label(gen->label_manager);
  node->attributes->next_label = safe_strdup(next_label);

  /* 2. Generate true_label and false_label */
  char *true_label = label_manager_new_label(gen->label_manager);
  bool has_else = (N_node->production_id == PROD_N_ELSE_S);
  char *false_label = has_else ? label_manager_new_label(gen->label_manager)
                               : safe_strdup(next_label);

  /* 3. Pass labels to condition node */
  C_node->attributes->true_label = safe_strdup(true_label);
  C_node->attributes->false_label = safe_strdup(false_label);

  /* 4. Generate condition code */
  sdt_codegen_generate(gen, C_node);

  /* 5. If 'then' is not a control structure, add true_label */
  bool is_ctrl = is_control_structure(S1_node);
  if (!is_ctrl) {
    tac_program_add_inst(gen->program, TAC_OP_LABEL, true_label, NULL, NULL, 0);
  }

  /* 6. If it's a control structure, pass true_label to reuse it */
  if (is_ctrl) {
    if (!ensure_attributes(S1_node))
      return false;
    S1_node->attributes->true_label = safe_strdup(true_label);
  }

  /* 7. Generate code for 'then' branch */
  if (!ensure_attributes(S1_node))
    return false;
  S1_node->attributes->next_label = safe_strdup(next_label);
  sdt_codegen_generate(gen, S1_node);

  /* 8. Handle 'else' branch (if exists) */
  if (has_else) {
    if (!is_ctrl) {
      tac_program_add_inst(gen->program, TAC_OP_GOTO, next_label, NULL, NULL,
                           0);
    }
    /* Output else label */
    tac_program_add_inst(gen->program, TAC_OP_LABEL, false_label, NULL, NULL,
                         0);
    /* Generate code for else branch */
    if (N_node->children_count > 1) {
      SyntaxTreeNode *else_stmt = N_node->children[1];
      if (!ensure_attributes(else_stmt))
        return false;
      else_stmt->attributes->next_label = safe_strdup(next_label);
      sdt_codegen_generate(gen, else_stmt);
    }
  }

  /* 9. Output next_label (only if newly generated by this node) */
  if (!inherited) {
    tac_program_add_inst(gen->program, TAC_OP_LABEL, next_label, NULL, NULL, 0);
  }

  DEBUG_PRINT("Generated if: true=%s false=%s next=%s", true_label, false_label,
              next_label);
  return true;
}

/**
 * @brief Semantic action for S → while C do S1
 */
static bool action_S_WHILE(SDTCodeGen *gen, SyntaxTreeNode *node) {
  if (!node || node->children_count < 4)
    return false;
  SyntaxTreeNode *C_node = node->children[1];  /* Condition */
  SyntaxTreeNode *S1_node = node->children[3]; /* Loop body */

  /* Ensure attribute structures */
  if (!ensure_attributes(node) || !ensure_attributes(C_node)) {
    return false;
  }

  /* Check if next_label is inherited */
  bool inherited = (node->attributes->next_label != NULL);

  /* 1. Generate or reuse next_label */
  char *next_label = inherited ? safe_strdup(node->attributes->next_label)
                               : label_manager_new_label(gen->label_manager);
  node->attributes->next_label = safe_strdup(next_label);

  /* 2. Generate loop entry begin_label */
  char *begin_label;
  if (node->attributes->true_label) {
    /* If outer if already passed a true_label, reuse it */
    begin_label = safe_strdup(node->attributes->true_label);
  } else {
    /* Otherwise create a new one */
    begin_label = label_manager_new_label(gen->label_manager);
  }

  /* 3. Generate true/false labels for condition */
  char *true_label = label_manager_new_label(gen->label_manager);
  C_node->attributes->true_label = safe_strdup(true_label);
  C_node->attributes->false_label = safe_strdup(next_label);

  /* 4. Output loop entry point */
  tac_program_add_inst(gen->program, TAC_OP_LABEL, begin_label, NULL, NULL, 0);

  /* 5. Generate condition code */
  sdt_codegen_generate(gen, C_node);

  /* 6. When condition is true, add true_label */
  tac_program_add_inst(gen->program, TAC_OP_LABEL, true_label, NULL, NULL, 0);

  /* 7. Generate loop body code */
  if (!ensure_attributes(S1_node))
    return false;
  S1_node->attributes->next_label = safe_strdup(begin_label);
  sdt_codegen_generate(gen, S1_node);

  /* 8. Jump back to loop entry */
  tac_program_add_inst(gen->program, TAC_OP_GOTO, begin_label, NULL, NULL, 0);

  /* 9. Output exit label next_label (only if newly generated by this node) */
  if (!inherited) {
    tac_program_add_inst(gen->program, TAC_OP_LABEL, next_label, NULL, NULL, 0);
  }

  DEBUG_PRINT("Generated while: begin=%s true=%s next=%s", begin_label,
              true_label, next_label);
  return true;
}

/**
 * @brief Semantic action for S → begin L end
 *
 * S.code = L.code
 */
static bool action_S_BEGIN(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* Get list of statements: [1]=L */
  SyntaxTreeNode *L_node = node->children[1];

  /* Pass next_label down to statement list */
  if (!ensure_attributes(L_node)) {
    return false;
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
  if (!node || node->children_count < 3) {
    DEBUG_PRINT("ERROR: Missing expression node(s) in relational operation");
    return false;
  }

  SyntaxTreeNode *E1_node = node->children[0];
  SyntaxTreeNode *E2_node = node->children[2];

  /* 1. Generate code for both expressions */
  sdt_codegen_generate(gen, E1_node);
  sdt_codegen_generate(gen, E2_node);

  /* 2. Ensure attributes exist */
  if (!ensure_attributes(node)) {
    return false;
  }

  /* 3. Use existing or create new true/false labels */
  if (!node->attributes->true_label) {
    node->attributes->true_label = label_manager_new_label(gen->label_manager);
  }

  if (!node->attributes->false_label) {
    node->attributes->false_label = label_manager_new_label(gen->label_manager);
  }

  char *t = node->attributes->true_label;
  char *f = node->attributes->false_label;

  /* 4. Generate conditional jump and default jump */
  tac_program_add_inst(gen->program, op, t, E1_node->attributes->place,
                       E2_node->attributes->place, 0);
  tac_program_add_inst(gen->program, TAC_OP_GOTO, f, NULL, NULL, 0);

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
  if (!node || node->children_count < 3) {
    DEBUG_PRINT("ERROR: Invalid parenthesized condition");
    return false;
  }

  /* Find inner condition node */
  SyntaxTreeNode *C1_node = find_child_by_name(node, "C", NODE_NONTERMINAL);

  if (!C1_node) {
    DEBUG_PRINT("ERROR: Missing inner condition in parentheses");
    return false;
  }

  /* Ensure inner condition has attributes */
  if (!ensure_attributes(C1_node)) {
    return false;
  }

  /* Pass down true/false labels if present */
  if (node->attributes && node->attributes->true_label) {
    C1_node->attributes->true_label = safe_strdup(node->attributes->true_label);
  }

  if (node->attributes && node->attributes->false_label) {
    C1_node->attributes->false_label =
        safe_strdup(node->attributes->false_label);
  }

  /* Generate inner condition code */
  sdt_codegen_generate(gen, C1_node);

  /* Inherit labels back if needed */
  if (!ensure_attributes(node)) {
    return false;
  }

  if (!node->attributes->true_label && C1_node->attributes->true_label) {
    node->attributes->true_label = safe_strdup(C1_node->attributes->true_label);
  }

  if (!node->attributes->false_label && C1_node->attributes->false_label) {
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
  if (!ensure_attributes(X_node)) {
    return false;
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

  /* 1. Generate code for right operand */
  sdt_codegen_generate(gen, R_node);

  /* 2. Allocate temporary variable */
  char *temp = symbol_table_new_temp(gen->symbol_table);
  if (!temp)
    return false;

  /* 3. Check operands */
  if (!node->attributes->place || !R_node->attributes->place) {
    DEBUG_PRINT("ERROR: Missing operands for addition");
    free(temp);
    return false;
  }

  /* 4. Generate addition instruction */
  tac_program_add_inst(gen->program, TAC_OP_ADD, temp, /* result */
                       node->attributes->place,   /* left operand (inherited) */
                       R_node->attributes->place, /* right operand */
                       0);

  /* 5. Pass inherited attribute to X1 */
  if (!ensure_attributes(X1_node)) {
    free(temp);
    return false;
  }

  X1_node->attributes->place = safe_strdup(temp);

  /* 6. Generate X1 code */
  sdt_codegen_generate(gen, X1_node);

  /* 7. Inherit synthesized place */
  if (X1_node->attributes->place)
    node->attributes->place = safe_strdup(X1_node->attributes->place);

  DEBUG_PRINT("Generated addition: %s := %s + %s", temp,
              node->attributes->place, R_node->attributes->place);

  /* 8. Clean up temporary string */
  free(temp);
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

  /* Generate code for right operand */
  sdt_codegen_generate(gen, R_node);

  /* Allocate temporary variable */
  char *temp = symbol_table_new_temp(gen->symbol_table);
  if (!temp)
    return false;

  /* Check operands */
  if (!node->attributes->place || !R_node->attributes->place) {
    DEBUG_PRINT("ERROR: Missing operands for subtraction");
    free(temp);
    return false;
  }

  /* Generate subtraction instruction */
  tac_program_add_inst(gen->program, TAC_OP_SUB, temp, node->attributes->place,
                       R_node->attributes->place, 0);

  /* Pass inherited attribute to X1 */
  if (!ensure_attributes(X1_node)) {
    free(temp);
    return false;
  }

  X1_node->attributes->place = safe_strdup(temp);

  /* Generate X1 code */
  sdt_codegen_generate(gen, X1_node);

  /* Inherit synthesized place */
  if (X1_node->attributes->place)
    node->attributes->place = safe_strdup(X1_node->attributes->place);

  DEBUG_PRINT("Generated subtraction: %s := %s - %s", temp,
              node->attributes->place, R_node->attributes->place);

  /* Clean up */
  free(temp);
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
  if (!ensure_attributes(Y_node)) {
    return false;
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

  /* Allocate temporary variable */
  char *temp = symbol_table_new_temp(gen->symbol_table);
  if (!temp)
    return false;

  /* Check operands */
  if (!node->attributes->place || !F_node->attributes->place) {
    DEBUG_PRINT("ERROR: Missing operands for multiplication");
    free(temp);
    return false;
  }

  /* Generate multiplication instruction */
  tac_program_add_inst(gen->program, TAC_OP_MUL, temp, node->attributes->place,
                       F_node->attributes->place, 0);

  /* Pass inherited attribute to Y1 */
  if (!ensure_attributes(Y1_node)) {
    free(temp);
    return false;
  }

  Y1_node->attributes->place = safe_strdup(temp);

  /* Generate Y1 code */
  sdt_codegen_generate(gen, Y1_node);

  /* Inherit synthesized place */
  if (Y1_node->attributes->place)
    node->attributes->place = safe_strdup(Y1_node->attributes->place);

  DEBUG_PRINT("Generated multiplication: %s := %s * %s", temp,
              node->attributes->place, F_node->attributes->place);

  /* Clean up */
  free(temp);
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

  /* Allocate temporary variable */
  char *temp = symbol_table_new_temp(gen->symbol_table);
  if (!temp)
    return false;

  /* Check operands */
  if (!node->attributes->place || !F_node->attributes->place) {
    DEBUG_PRINT("ERROR: Missing operands for division");
    free(temp);
    return false;
  }

  /* Generate division instruction */
  tac_program_add_inst(gen->program, TAC_OP_DIV, temp, node->attributes->place,
                       F_node->attributes->place, 0);

  /* Pass inherited attribute to Y1 */
  if (!ensure_attributes(Y1_node)) {
    free(temp);
    return false;
  }

  Y1_node->attributes->place = safe_strdup(temp);

  /* Generate Y1 code */
  sdt_codegen_generate(gen, Y1_node);

  /* Inherit synthesized place */
  if (Y1_node->attributes->place)
    node->attributes->place = safe_strdup(Y1_node->attributes->place);

  DEBUG_PRINT("Generated division: %s := %s / %s", temp,
              node->attributes->place, F_node->attributes->place);

  /* Clean up */
  free(temp);
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
  /* Find expression node (should be between parentheses) */
  SyntaxTreeNode *E_node = find_child_by_name(node, "E", NODE_NONTERMINAL);

  if (!E_node) {
    DEBUG_PRINT("ERROR: Expression node not found in parenthesized expression");
    return false;
  }

  /* Generate code for expression */
  sdt_codegen_generate(gen, E_node);

  /* Inherit place from expression */
  if (!ensure_attributes(node)) {
    return false;
  }

  if (E_node->attributes && E_node->attributes->place) {
    node->attributes->place = safe_strdup(E_node->attributes->place);
    DEBUG_PRINT("Executed F → ( E ) action: place = %s",
                node->attributes->place);
  } else {
    DEBUG_PRINT("WARNING: Expression in parentheses has no place attribute");
    node->attributes->place = safe_strdup("unknown");
  }

  return true;
}

/**
 * @brief Semantic action for F → id
 *
 * F.place = id.lexeme;
 * F.code = '';
 */
static bool action_F_ID(SDTCodeGen *gen, SyntaxTreeNode *node) {
  /* Check node validity */
  if (!node || node->type != NODE_NONTERMINAL || !ensure_attributes(node)) {
    DEBUG_PRINT("ERROR: Invalid node for F_ID");
    return false;
  }

  /* Find the identifier terminal node */
  SyntaxTreeNode *id_node = find_child_by_name(node, "id", NODE_TERMINAL);
  if (!id_node) {
    DEBUG_PRINT("ERROR: Identifier terminal node not found");
    /* Use a placeholder */
    static int id_counter = 0;
    char placeholder[32];
    snprintf(placeholder, sizeof(placeholder), "id_%d", id_counter++);
    node->attributes->place = safe_strdup(placeholder);
    return true;
  }

  /* Get token string representation (simplified with improved token_to_string)
   */
  char token_str[128];
  token_to_string(&id_node->token, token_str, sizeof(token_str));

  /* Set place to the identifier name */
  node->attributes->place = safe_strdup(token_str);
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
  /* Check node validity */
  if (!node || node->type != NODE_NONTERMINAL || !ensure_attributes(node)) {
    DEBUG_PRINT("ERROR: Invalid node for F_INT");
    return false;
  }

  /* Find the integer terminal node */
  SyntaxTreeNode *int_node = NULL;
  for (int i = 0; i < node->children_count; i++) {
    if (node->children[i]->type == NODE_TERMINAL) {
      const char *symbol = node->children[i]->symbol_name;
      if (strcmp(symbol, "int8") == 0 || strcmp(symbol, "int10") == 0 ||
          strcmp(symbol, "int16") == 0) {
        int_node = node->children[i];
        break;
      }
    }
  }

  if (!int_node) {
    DEBUG_PRINT("ERROR: Integer terminal node not found");
    return false;
  }

  /* Get token string representation (simplified with improved token_to_string)
   */
  char token_str[128];
  token_to_string(&int_node->token, token_str, sizeof(token_str));

  /* Set place to the integer value string */
  node->attributes->place = safe_strdup(token_str);
  DEBUG_PRINT("Factor place set to integer: %s", node->attributes->place);

  return true;
}
