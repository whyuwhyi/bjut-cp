/**
 * @file action_table.h
 * @brief LR action/goto table representation (internal)
 */

#ifndef ACTION_TABLE_H
#define ACTION_TABLE_H

#include "parser/grammar.h"
#include <stdbool.h>

/**
 * @brief LR parser action types
 */
typedef enum {
  ACTION_SHIFT,  /* Shift a token and go to state */
  ACTION_REDUCE, /* Reduce by production */
  ACTION_ACCEPT, /* Accept the input */
  ACTION_ERROR   /* Error */
} ActionType;

/**
 * @brief LR parser action
 */
typedef struct {
  ActionType type; /* Action type */
  int value;       /* State for shift, production for reduce */
} Action;

/**
 * @brief LR parsing table
 */
typedef struct {
  /* Action table: action[state][terminal] */
  Action **action_table;

  /* Goto table: goto_table[state][non-terminal] */
  int **goto_table;

  int state_count;       /* Number of states */
  int terminal_count;    /* Number of terminals */
  int nonterminal_count; /* Number of non-terminals */
} ActionTable;

/**
 * @brief Create a new action table
 *
 * @param state_count Number of states
 * @param terminal_count Number of terminals
 * @param nonterminal_count Number of non-terminals
 * @return ActionTable* Created table
 */
ActionTable *action_table_create(int state_count, int terminal_count,
                                 int nonterminal_count);

/**
 * @brief Destroy an action table
 *
 * @param table Table to destroy
 */
void action_table_destroy(ActionTable *table);

/**
 * @brief Set an action in the table
 *
 * @param table Action table
 * @param state State ID
 * @param terminal Terminal ID
 * @param action_type Action type
 * @param action_value Action value
 * @return bool Success status
 */
bool action_table_set_action(ActionTable *table, int state, int terminal,
                             ActionType action_type, int action_value);

/**
 * @brief Set a goto entry in the table
 *
 * @param table Action table
 * @param state State ID
 * @param nonterminal Non-terminal ID
 * @param goto_state Goto state ID
 * @return bool Success status
 */
bool action_table_set_goto(ActionTable *table, int state, int nonterminal,
                           int goto_state);

/**
 * @brief Get an action from the table
 *
 * @param table Action table
 * @param state State ID
 * @param terminal Terminal ID
 * @return Action Action to take
 */
Action action_table_get_action(ActionTable *table, int state, int terminal);

/**
 * @brief Get a goto entry from the table
 *
 * @param table Action table
 * @param state State ID
 * @param nonterminal Non-terminal ID
 * @return int Goto state ID
 */
int action_table_get_goto(ActionTable *table, int state, int nonterminal);

/**
 * @brief Print the action table
 *
 * @param table Action table
 * @param grammar Grammar
 */
void action_table_print(ActionTable *table, Grammar *grammar);

#endif /* ACTION_TABLE_H */
