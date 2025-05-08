/**
 * @file sdt_actions.h
 * @brief Semantic actions for syntax-directed translation
 */

#ifndef SDT_ACTIONS_H
#define SDT_ACTIONS_H

#include "codegen/sdt_attributes.h"
#include "parser/syntax_tree.h"
#include <stdbool.h>

struct SDTCodeGen; /* Forward declaration */

/**
 * @brief Helper function to execute semantic action for a production
 */
bool sdt_execute_production_action(struct SDTCodeGen *gen, int production_id,
                                   SyntaxTreeNode *node);

/* Program */
bool sdt_action_program_1(struct SDTCodeGen *gen, SyntaxTreeNode *node);
bool sdt_action_program_2(struct SDTCodeGen *gen, SyntaxTreeNode *node);

/* Statement list */
bool sdt_action_stmt_list(struct SDTCodeGen *gen, SyntaxTreeNode *node);

/* Statement */
bool sdt_action_assign(struct SDTCodeGen *gen, SyntaxTreeNode *node);
bool sdt_action_if(struct SDTCodeGen *gen, SyntaxTreeNode *node);
bool sdt_action_if_else(struct SDTCodeGen *gen, SyntaxTreeNode *node);
bool sdt_action_while(struct SDTCodeGen *gen, SyntaxTreeNode *node);
bool sdt_action_block(struct SDTCodeGen *gen, SyntaxTreeNode *node);
bool sdt_action_else_part(struct SDTCodeGen *gen, SyntaxTreeNode *node);

/* Condition */
bool sdt_action_condition_gt(struct SDTCodeGen *gen, SyntaxTreeNode *node);
bool sdt_action_condition_lt(struct SDTCodeGen *gen, SyntaxTreeNode *node);
bool sdt_action_condition_eq(struct SDTCodeGen *gen, SyntaxTreeNode *node);
bool sdt_action_condition_ge(struct SDTCodeGen *gen, SyntaxTreeNode *node);
bool sdt_action_condition_le(struct SDTCodeGen *gen, SyntaxTreeNode *node);
bool sdt_action_condition_ne(struct SDTCodeGen *gen, SyntaxTreeNode *node);
bool sdt_action_condition_paren(struct SDTCodeGen *gen, SyntaxTreeNode *node);

/* Expression */
bool sdt_action_expr_plus(struct SDTCodeGen *gen, SyntaxTreeNode *node);
bool sdt_action_expr_minus(struct SDTCodeGen *gen, SyntaxTreeNode *node);
bool sdt_action_expr_term(struct SDTCodeGen *gen, SyntaxTreeNode *node);

/* Term */
bool sdt_action_term_mul(struct SDTCodeGen *gen, SyntaxTreeNode *node);
bool sdt_action_term_div(struct SDTCodeGen *gen, SyntaxTreeNode *node);
bool sdt_action_term_factor(struct SDTCodeGen *gen, SyntaxTreeNode *node);

/* Factor */
bool sdt_action_factor_paren(struct SDTCodeGen *gen, SyntaxTreeNode *node);
bool sdt_action_factor_id(struct SDTCodeGen *gen, SyntaxTreeNode *node);
bool sdt_action_factor_int8(struct SDTCodeGen *gen, SyntaxTreeNode *node);
bool sdt_action_factor_int10(struct SDTCodeGen *gen, SyntaxTreeNode *node);
bool sdt_action_factor_int16(struct SDTCodeGen *gen, SyntaxTreeNode *node);

#endif /* SDT_ACTIONS_H */
