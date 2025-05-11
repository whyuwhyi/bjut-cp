/**
 * @file parser_error_handler.c
 * @brief Implementation of parser-specific error handling functions
 */

#include "../parser/lr/lr_common.h"
#include "error_handler.h"
#include "lexer/lexer.h"
#include "parser/grammar.h"
#include "parser/parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Check if a token is a synchronization point for error recovery
 */
SyncPointType is_sync_point(TokenType token_type) {
  switch (token_type) {
  case TK_SEMI: /* Semicolon */
    return SYNC_STATEMENT;
  case TK_END: /* 'end' keyword */
    return SYNC_BLOCK;
  case TK_SRP: /* Right parenthesis */
    return SYNC_EXPRESSION;
  case TK_THEN: /* 'then' keyword */
  case TK_ELSE: /* 'else' keyword */
  case TK_DO:   /* 'do' keyword */
    return SYNC_STATEMENT;
  case TK_BEGIN: /* 'begin' keyword */
    return SYNC_BLOCK;
  case TK_EOF: /* End of file */
    return SYNC_BLOCK;
  default:
    return SYNC_NONE;
  }
}

/**
 * @brief Determine expected tokens in the current parser state
 */
int determine_expected_tokens(Parser *parser, LRParserData *data,
                              int current_state, TokenType *expected,
                              int max_expected) {
  int count = 0;
  Grammar *grammar = parser->grammar;

  /* Check which terminals have valid actions in current state */
  for (int t = 0; t < grammar->terminals_count && count < max_expected; t++) {
    Action action = action_table_get_action(data->table, current_state, t);
    if (action.type != ACTION_ERROR) {
      TokenType token = grammar->symbols[grammar->terminal_indices[t]].token;

      /* Avoid duplicates */
      bool already_included = false;
      for (int i = 0; i < count; i++) {
        if (expected[i] == token) {
          already_included = true;
          break;
        }
      }

      if (!already_included) {
        expected[count++] = token;
      }
    }
  }

  return count;
}

/**
 * @brief Try to find a possible missing token based on current context
 */
TokenType find_missing_token(Parser *parser, LRParserData *data,
                             int current_state, const Token *current_token) {
  /* Get the expected tokens in current state */
  TokenType expected[5];
  int expected_count =
      determine_expected_tokens(parser, data, current_state, expected, 5);

  /* Common token pairs that are often forgotten */
  static const struct {
    TokenType open;
    TokenType close;
  } pairs[] = {
      {TK_SLP, TK_SRP}, /* Parenthesis pair */
      {TK_IF, TK_THEN}, /* if-then pair */
      {TK_WHILE, TK_DO} /* while-do pair */
  };
  const int pairs_count = sizeof(pairs) / sizeof(pairs[0]);

  /* Check for missing closing tokens */
  for (int i = 0; i < expected_count; i++) {
    for (int j = 0; j < pairs_count; j++) {
      if (expected[i] == pairs[j].close) {
        /* Check stack for an opening token */
        bool found_opening = false;
        for (int k = data->stack_top; k >= 0; k--) {
          if (data->node_stack[k] &&
              data->node_stack[k]->type == NODE_TERMINAL &&
              data->node_stack[k]->token.type == pairs[j].open) {
            found_opening = true;
            break;
          }
        }

        if (found_opening) {
          /* Found opening token but missing closing token */
          return pairs[j].close;
        }
      }
    }
  }

  /* Special case for semicolons */
  for (int i = 0; i < expected_count; i++) {
    if (expected[i] == TK_SEMI) {
      return TK_SEMI;
    }
  }

  return TK_NOTYPE; /* No missing token identified */
}

/**
 * @brief Check if parser is currently in an expression context
 */
bool is_expression_context(Parser *parser, LRParserData *data) {
  if (!parser || !data) {
    return false;
  }

  /* Check stack for expression-related non-terminals */
  for (int i = data->stack_top; i >= 0; i--) {
    if (data->node_stack[i] && data->node_stack[i]->type == NODE_NONTERMINAL) {
      /* Using Nonterminal enum values from grammar.h */
      int nt_type = data->node_stack[i]->nonterminal_id;
      if (nt_type == NT_E || nt_type == NT_R || nt_type == NT_F ||
          nt_type == NT_X || nt_type == NT_Y) {
        return true;
      }

      /* If we encounter a statement non-terminal before an expression,
         then we're not in an expression context */
      if (nt_type == NT_S || nt_type == NT_L) {
        return false;
      }
    }
  }

  return false; /* Default: not in expression context */
}

/**
 * @brief Check if parser is currently in a statement context
 */
bool is_statement_context(Parser *parser, LRParserData *data) {
  if (!parser || !data) {
    return false;
  }

  /* Check stack for statement-related non-terminals */
  for (int i = data->stack_top; i >= 0; i--) {
    if (data->node_stack[i] && data->node_stack[i]->type == NODE_NONTERMINAL) {
      /* Using Nonterminal enum values from grammar.h */
      int nt_type = data->node_stack[i]->nonterminal_id;
      if (nt_type == NT_S || nt_type == NT_L || nt_type == NT_N) {
        return true;
      }
    }
  }

  return false; /* Default: not in statement context */
}

/**
 * @brief Get the current token from the lexer
 */
static const Token *get_current_token(LRParserData *data) {
  if (!data || !data->lexer) {
    return NULL;
  }
  return lexer_get_token(data->lexer, data->current_token);
}

/**
 * @brief Skip tokens until a synchronization point is found
 */
bool skip_to_sync_point(LRParserData *data, SyncPointType min_sync_level) {
  if (!data) {
    return false;
  }

  const Token *token = NULL;
  int original_current_token = data->current_token;

  /* Skip tokens until we find a synchronization point */
  do {
    token = get_current_token(data);

    if (!token || token->type == TK_EOF) {
      /* Reached end of file during error recovery, restore original position */
      data->current_token = original_current_token;
      return false;
    }

    SyncPointType sync_type = is_sync_point(token->type);
    if (sync_type >= min_sync_level) {
      /* Found a suitable synchronization point */
      DEBUG_PRINT("Found sync point at token %s",
                  token_type_to_string(token->type));
      return true;
    }

    /* Skip current token */
    data->current_token++;
  } while (token && token->type != TK_EOF);

  /* Reached EOF without finding sync point, restore original position */
  data->current_token = original_current_token;
  return false;
}

/**
 * @brief Report a syntax error with source highlighting and suggestions
 */
void report_syntax_error(Parser *parser, LRParserData *data, const Token *token,
                         const char *source_line) {
  if (!parser || !data || !token) {
    return;
  }

  int current_state =
      data->stack_top >= 0 ? data->state_stack[data->stack_top] : 0;

  /* If source line not provided, try to get it */
  if (!source_line) {
    static char line_buffer[256];
    if (extract_line_from_input(data->lexer->input, token->line, line_buffer,
                                sizeof(line_buffer)) > 0) {
      source_line = line_buffer;
    } else {
      source_line = ""; /* Default if source line cannot be retrieved */
    }
  }

  /* Format token info for error message */
  char token_str[128];
  token_to_string(token, token_str, sizeof(token_str));

  /* Generate error message */
  char error_message[256];
  snprintf(error_message, sizeof(error_message), "Unexpected token '%s'",
           token_str);

  /* Token length approximation */
  int token_length = strlen(token_str);
  if (token_length > 20)
    token_length = 20;
  if (token_length < 1)
    token_length = 1;

  /* Print main error message with source line highlight */
  PRINT_ERROR_HIGHLIGHT(token->line, token->column, source_line, token->column,
                        token_length, "%s", error_message);

  /* Generate helpful suggestions based on the current state */
  char expected_tokens[384] = {0};
  int expected_count = 0;
  bool first_token = true;

  /* Check what tokens would be valid in this state */
  TokenType expected[8];
  expected_count =
      determine_expected_tokens(parser, data, current_state, expected, 8);

  /* Format the expected tokens list */
  for (int i = 0; i < expected_count; i++) {
    /* Get the token name */
    const char *token_name = token_type_to_string(expected[i]);

    /* Add to expected token list */
    if (first_token) {
      first_token = false;
    } else {
      strcat(expected_tokens, ", ");
    }

    strcat(expected_tokens, token_name);

    /* Limit the number of suggestions */
    if (i >= 6 && expected_count > 8) {
      strcat(expected_tokens, ", ...");
      break;
    }
  }

  /* Create the help message */
  char help_message[512];
  if (expected_count > 0) {
    snprintf(help_message, sizeof(help_message),
             "Expected one of: %s. Try adding one of these tokens or check for "
             "missing tokens",
             expected_tokens);
  } else {
    /* If we can't determine expected tokens, give a more general message */
    snprintf(help_message, sizeof(help_message),
             "Unable to continue parsing from this point. Check for syntax "
             "errors earlier in the code");
  }

  /* Add additional suggestion based on missing token detection */
  TokenType missing = find_missing_token(parser, data, current_state, token);
  if (missing != TK_NOTYPE) {
    char missing_suggestion[128];
    snprintf(missing_suggestion, sizeof(missing_suggestion),
             ". A '%s' might be missing before this token",
             token_type_to_string(missing));
    strcat(help_message, missing_suggestion);
  }

  PRINT_ERROR_HELP(help_message);
}

/**
 * @brief Enhanced error recovery for LR parsers
 */
bool enhanced_error_recovery(Parser *parser, LRParserData *data,
                             const Token *error_token) {
  if (!parser || !data || !error_token) {
    return false;
  }

  int current_state =
      data->stack_top >= 0 ? data->state_stack[data->stack_top] : 0;

  DEBUG_PRINT("Starting error recovery for token %s in state %d",
              token_type_to_string(error_token->type), current_state);

  /* 1. Check for missing tokens */
  TokenType missing =
      find_missing_token(parser, data, current_state, error_token);
  if (missing != TK_NOTYPE) {
    /* Report the potential missing token */
    PRINT_WARNING(error_token->line, error_token->column,
                  "Possible missing '%s' before '%s'",
                  token_type_to_string(missing),
                  token_type_to_string(error_token->type));
  }

  /* 2. Report detailed error with suggestions */
  report_syntax_error(parser, data, error_token, NULL);

  /* 3. Determine appropriate sync level based on context */
  SyncPointType required_sync;

  if (is_expression_context(parser, data)) {
    required_sync = SYNC_EXPRESSION;
    DEBUG_PRINT("In expression context, looking for expression sync points");
  } else if (is_statement_context(parser, data)) {
    required_sync = SYNC_STATEMENT;
    DEBUG_PRINT("In statement context, looking for statement sync points");
  } else {
    required_sync = SYNC_BLOCK;
    DEBUG_PRINT("In block context, looking for block sync points");
  }

  /* 4. Skip to appropriate sync point */
  bool found_sync = skip_to_sync_point(data, required_sync);

  if (found_sync) {
    DEBUG_PRINT("Successfully synchronized after error");
    return true;
  } else {
    DEBUG_PRINT("Reached EOF during error recovery");
    return false;
  }
}
