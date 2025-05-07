/**
 * @file action_table.c
 * @brief LR action/goto table representation implementation
 */

#include "action_table.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create a new action table
 */
ActionTable *action_table_create(int state_count, int terminal_count,
                                 int nonterminal_count) {
  if (state_count <= 0 || terminal_count <= 0 || nonterminal_count <= 0) {
    return NULL;
  }

  ActionTable *table = (ActionTable *)safe_malloc(sizeof(ActionTable));
  if (!table) {
    return NULL;
  }

  table->state_count = state_count;
  table->terminal_count = terminal_count;
  table->nonterminal_count = nonterminal_count;

  /* Allocate action table */
  table->action_table = (Action **)safe_malloc(state_count * sizeof(Action *));
  if (!table->action_table) {
    free(table);
    return NULL;
  }

  for (int i = 0; i < state_count; i++) {
    table->action_table[i] =
        (Action *)safe_malloc(terminal_count * sizeof(Action));
    if (!table->action_table[i]) {
      for (int j = 0; j < i; j++) {
        free(table->action_table[j]);
      }
      free(table->action_table);
      free(table);
      return NULL;
    }

    /* Initialize to error */
    for (int j = 0; j < terminal_count; j++) {
      table->action_table[i][j].type = ACTION_ERROR;
      table->action_table[i][j].value = -1;
    }
  }

  /* Allocate goto table */
  table->goto_table = (int **)safe_malloc(state_count * sizeof(int *));
  if (!table->goto_table) {
    for (int i = 0; i < state_count; i++) {
      free(table->action_table[i]);
    }
    free(table->action_table);
    free(table);
    return NULL;
  }

  for (int i = 0; i < state_count; i++) {
    table->goto_table[i] = (int *)safe_malloc(nonterminal_count * sizeof(int));
    if (!table->goto_table[i]) {
      for (int j = 0; j < i; j++) {
        free(table->goto_table[j]);
      }
      free(table->goto_table);

      for (int j = 0; j < state_count; j++) {
        free(table->action_table[j]);
      }
      free(table->action_table);
      free(table);
      return NULL;
    }

    /* Initialize to -1 (error) */
    for (int j = 0; j < nonterminal_count; j++) {
      table->goto_table[i][j] = -1;
    }
  }

  DEBUG_PRINT(
      "Created action table with %d states, %d terminals, %d non-terminals",
      state_count, terminal_count, nonterminal_count);
  return table;
}

/**
 * @brief Destroy an action table
 */
void action_table_destroy(ActionTable *table) {
  if (!table) {
    return;
  }

  if (table->action_table) {
    for (int i = 0; i < table->state_count; i++) {
      if (table->action_table[i]) {
        free(table->action_table[i]);
      }
    }
    free(table->action_table);
  }

  if (table->goto_table) {
    for (int i = 0; i < table->state_count; i++) {
      if (table->goto_table[i]) {
        free(table->goto_table[i]);
      }
    }
    free(table->goto_table);
  }

  free(table);
  DEBUG_PRINT("Destroyed action table");
}

/**
 * @brief Set an action in the table
 */
bool action_table_set_action(ActionTable *table, int state, int terminal,
                             ActionType action_type, int action_value) {
  if (!table || state < 0 || state >= table->state_count || terminal < 0 ||
      terminal >= table->terminal_count) {
    return false;
  }

  /* Check for conflicts */
  if (table->action_table[state][terminal].type != ACTION_ERROR) {
    fprintf(stderr,
            "Warning: Conflict in LR parsing table at state %d, terminal %d\n",
            state, terminal);
    fprintf(stderr, "  Existing: ");
    switch (table->action_table[state][terminal].type) {
    case ACTION_SHIFT:
      fprintf(stderr, "shift %d\n", table->action_table[state][terminal].value);
      break;
    case ACTION_REDUCE:
      fprintf(stderr, "reduce by production %d\n",
              table->action_table[state][terminal].value);
      break;
    case ACTION_ACCEPT:
      fprintf(stderr, "accept\n");
      break;
    default:
      fprintf(stderr, "error\n");
      break;
    }

    fprintf(stderr, "  New: ");
    switch (action_type) {
    case ACTION_SHIFT:
      fprintf(stderr, "shift %d\n", action_value);
      break;
    case ACTION_REDUCE:
      fprintf(stderr, "reduce by production %d\n", action_value);
      break;
    case ACTION_ACCEPT:
      fprintf(stderr, "accept\n");
      break;
    default:
      fprintf(stderr, "error\n");
      break;
    }

    /* Resolve conflicts (prefer shift over reduce) */
    if (table->action_table[state][terminal].type == ACTION_SHIFT &&
        action_type == ACTION_REDUCE) {
      /* Keep existing shift action */
      DEBUG_PRINT("Resolved shift-reduce conflict in favor of shift");
      return true;
    }
  }

  table->action_table[state][terminal].type = action_type;
  table->action_table[state][terminal].value = action_value;

  DEBUG_PRINT("Set action in parsing table at state %d, terminal %d: %s %d",
              state, terminal,
              action_type == ACTION_SHIFT    ? "shift"
              : action_type == ACTION_REDUCE ? "reduce"
              : action_type == ACTION_ACCEPT ? "accept"
                                             : "error",
              action_value);
  return true;
}

/**
 * @brief Set a goto entry in the table
 */
bool action_table_set_goto(ActionTable *table, int state, int nonterminal,
                           int goto_state) {
  if (!table || state < 0 || state >= table->state_count || nonterminal < 0 ||
      nonterminal >= table->nonterminal_count) {
    return false;
  }

  table->goto_table[state][nonterminal] = goto_state;

  DEBUG_PRINT("Set goto in parsing table at state %d, non-terminal %d: %d",
              state, nonterminal, goto_state);
  return true;
}

/**
 * @brief Get an action from the table
 */
Action action_table_get_action(ActionTable *table, int state, int terminal) {
  Action error_action = {ACTION_ERROR, -1};

  if (!table || state < 0 || state >= table->state_count || terminal < 0 ||
      terminal >= table->terminal_count) {
    return error_action;
  }

  return table->action_table[state][terminal];
}

/**
 * @brief Get a goto entry from the table
 */
int action_table_get_goto(ActionTable *table, int state, int nonterminal) {
  if (!table || state < 0 || state >= table->state_count || nonterminal < 0 ||
      nonterminal >= table->nonterminal_count) {
    return -1;
  }

  return table->goto_table[state][nonterminal];
}

/**
 * @brief Print the action table
 */
void action_table_print(ActionTable *table, Grammar *grammar) {
  if (!table || !grammar) {
    return;
  }

  printf("LR Parsing Table:\n");

  /* Print header */
  printf("     |");
  for (int i = 0; i < grammar->terminals_count; i++) {
    printf(" %-8s |", grammar->symbols[grammar->terminal_indices[i]].name);
  }
  printf(" | ");
  for (int i = 0; i < grammar->nonterminals_count; i++) {
    printf(" %-8s |", grammar->symbols[grammar->nonterminal_indices[i]].name);
  }
  printf("\n");

  /* Print separator */
  printf("-----|");
  for (int i = 0; i < grammar->terminals_count; i++) {
    printf("---------|");
  }
  printf("-|");
  for (int i = 0; i < grammar->nonterminals_count; i++) {
    printf("---------|");
  }
  printf("\n");

  /* Print rows */
  for (int state = 0; state < table->state_count; state++) {
    printf("%4d |", state);

    /* Print action part */
    for (int term = 0; term < grammar->terminals_count; term++) {
      Action action = table->action_table[state][term];

      switch (action.type) {
      case ACTION_SHIFT:
        printf(" s%-7d |", action.value);
        break;
      case ACTION_REDUCE:
        printf(" r%-7d |", action.value);
        break;
      case ACTION_ACCEPT:
        printf(" acc     |");
        break;
      default:
        printf("         |");
        break;
      }
    }

    printf(" | ");

    /* Print goto part */
    for (int nt = 0; nt < grammar->nonterminals_count; nt++) {
      int goto_state = table->goto_table[state][nt];

      if (goto_state >= 0) {
        printf(" %-8d |", goto_state);
      } else {
        printf("         |");
      }
    }

    printf("\n");
  }
}
