/**
 * @file lexer_state_machine.c
 * @brief Implementation of state machine-based lexical analyzer
 */

#include "lexer_analyzer/lexer.h"
#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * State types for the lexer state machine
 */
typedef enum {
  STATE_START,          /**< Initial state */
  STATE_IDENTIFIER,     /**< Parsing identifier */
  STATE_DECIMAL,        /**< Parsing decimal integer */
  STATE_OCTAL,          /**< Parsing octal integer */
  STATE_HEX_PREFIX,     /**< Parsed '0x' prefix */
  STATE_HEX,            /**< Parsing hexadecimal integer */
  STATE_INVALID_OCTAL,  /**< Invalid octal (digits 8-9 found) */
  STATE_INVALID_HEX,    /**< Invalid hex (non-hex chars found) */
  STATE_OPERATOR_START, /**< Start of multi-char operator */
  STATE_END             /**< End state */
} StateType;

/* Keyword lookup table */
static struct {
  const char *keyword;
  TokenType token_type;
} keywords[] = {
    {"if", TK_IF},       {"then", TK_THEN}, {"else", TK_ELSE},
    {"while", TK_WHILE}, {"do", TK_DO},     {"begin", TK_BEGIN},
    {"end", TK_END},     {NULL, TK_NOTYPE} /* Sentinel */
};

/**
 * Check if a string is a keyword and return its token type
 */
static TokenType check_keyword(const char *str) {
  for (int i = 0; keywords[i].keyword != NULL; i++) {
    if (strcmp(str, keywords[i].keyword) == 0) {
      return keywords[i].token_type;
    }
  }
  return TK_IDN; /* Not a keyword, so it's an identifier */
}

/**
 * Add a token to the lexer
 */
static bool add_token(Lexer *lexer, TokenType type, const char *start, int len,
                      int *value) {
  if (lexer->nr_token >= CONFIG_MAX_TOKENS) {
    fprintf(stderr, "Too many tokens (max: %d)\n", CONFIG_MAX_TOKENS);
    return false;
  }

  Token *token = &lexer->tokens[lexer->nr_token++];
  token->type = type;

  switch (type) {
  case TK_DEC:
  case TK_OCT:
  case TK_HEX:
    token->num_val = *value;
    DEBUG_PRINT("Added numeric token: %d", token->num_val);
    break;
  case TK_IDN:
  case TK_ILOCT:
  case TK_ILHEX:
    if (len >= CONFIG_MAX_TOKEN_LEN) {
      fprintf(stderr, "Token is too long: %.*s\n", len, start);
      return false;
    }
    strncpy(token->str_val, start, len);
    token->str_val[len] = '\0';

    /* Check if identifier is actually a keyword */
    if (type == TK_IDN) {
      TokenType keyword_type = check_keyword(token->str_val);
      if (keyword_type != TK_IDN) {
        token->type = keyword_type;
        DEBUG_PRINT("Identified keyword: %s", token->str_val);
      } else {
        DEBUG_PRINT("Added identifier: %s", token->str_val);
      }
    } else {
      DEBUG_PRINT("Added string token: %s", token->str_val);
    }
    break;
  default:
    DEBUG_PRINT("Added token type: %s", token_type_to_string(type));
    break;
  }

  return true;
}

/**
 * Tokenize an input string using a state machine approach
 */
bool lexer_tokenize_state_machine(Lexer *lexer, const char *input) {
  if (!lexer || !input) {
    return false;
  }

  lexer->nr_token = 0;
  int position = 0;

  DEBUG_PRINT("Starting state machine tokenization of input (length: %zu)",
              strlen(input));

  /* Main tokenization loop */
  while (input[position] != '\0') {
    /* Skip whitespace */
    if (isspace((unsigned char)input[position])) {
      position++;
      continue;
    }

    /* Initialize state for this token */
    StateType state = STATE_START;
    const char *token_start = input + position;
    int token_length = 0;
    int token_value = 0;

    /* Process the current token */
    while (state != STATE_END) {
      char c = input[position + token_length];

      switch (state) {
      case STATE_START:
        if (isalpha((unsigned char)c)) {
          state = STATE_IDENTIFIER;
          token_length++;
        } else if (c == '0') {
          token_value = 0;
          token_length++;
          /* Check next character for hex or octal */
          if (input[position + token_length] == 'x' ||
              input[position + token_length] == 'X') {
            state = STATE_HEX_PREFIX;
            token_length++;
          } else {
            state = STATE_OCTAL;
          }
        } else if (isdigit((unsigned char)c)) {
          state = STATE_DECIMAL;
          token_value = c - '0';
          token_length++;
        } else if (c == '+') {
          if (!add_token(lexer, TK_ADD, token_start, 1, NULL)) {
            return false;
          }
          position += 1;
          state = STATE_END;
        } else if (c == '-') {
          if (!add_token(lexer, TK_SUB, token_start, 1, NULL)) {
            return false;
          }
          position += 1;
          state = STATE_END;
        } else if (c == '*') {
          if (!add_token(lexer, TK_MUL, token_start, 1, NULL)) {
            return false;
          }
          position += 1;
          state = STATE_END;
        } else if (c == '/') {
          if (!add_token(lexer, TK_DIV, token_start, 1, NULL)) {
            return false;
          }
          position += 1;
          state = STATE_END;
        } else if (c == '(') {
          if (!add_token(lexer, TK_SLP, token_start, 1, NULL)) {
            return false;
          }
          position += 1;
          state = STATE_END;
        } else if (c == ')') {
          if (!add_token(lexer, TK_SRP, token_start, 1, NULL)) {
            return false;
          }
          position += 1;
          state = STATE_END;
        } else if (c == ';') {
          if (!add_token(lexer, TK_SEMI, token_start, 1, NULL)) {
            return false;
          }
          position += 1;
          state = STATE_END;
        } else if (c == '=') {
          if (!add_token(lexer, TK_EQ, token_start, 1, NULL)) {
            return false;
          }
          position += 1;
          state = STATE_END;
        } else if (c == '>') {
          token_length++;
          state = STATE_OPERATOR_START;
        } else if (c == '<') {
          token_length++;
          state = STATE_OPERATOR_START;
        } else if (c == '\0') {
          /* End of input */
          state = STATE_END;
        } else {
          /* Unrecognized character */
          fprintf(stderr, "No match at position %d: '%c'\n", position, c);
          position++;
          state = STATE_END;
        }
        break;

      case STATE_IDENTIFIER:
        if (isalnum((unsigned char)c)) {
          token_length++;
        } else {
          /* End of identifier */
          if (!add_token(lexer, TK_IDN, token_start, token_length, NULL)) {
            return false;
          }
          position += token_length;
          state = STATE_END;
        }
        break;

      case STATE_DECIMAL:
        if (isdigit((unsigned char)c)) {
          token_value = token_value * 10 + (c - '0');
          token_length++;
        } else {
          /* End of decimal number */
          if (!add_token(lexer, TK_DEC, token_start, token_length,
                         &token_value)) {
            return false;
          }
          position += token_length;
          state = STATE_END;
        }
        break;

      case STATE_OCTAL:
        if (c >= '0' && c <= '7') {
          token_value = token_value * 8 + (c - '0');
          token_length++;
        } else if (c >= '8' && c <= '9') {
          /* Invalid octal */
          token_length++;
          state = STATE_INVALID_OCTAL;
        } else {
          /* End of octal number */
          if (!add_token(lexer, TK_OCT, token_start, token_length,
                         &token_value)) {
            return false;
          }
          position += token_length;
          state = STATE_END;
        }
        break;

      case STATE_HEX_PREFIX:
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F')) {
          state = STATE_HEX;
          /* Convert hex digit to value */
          if (c >= '0' && c <= '9') {
            token_value = c - '0';
          } else if (c >= 'a' && c <= 'f') {
            token_value = 10 + (c - 'a');
          } else {
            token_value = 10 + (c - 'A');
          }
          token_length++;
        } else {
          /* Invalid hex (no digits after 0x) */
          if (!add_token(lexer, TK_ILHEX, token_start, token_length, NULL)) {
            return false;
          }
          position += token_length;
          state = STATE_END;
        }
        break;

      case STATE_HEX:
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F')) {
          /* Convert hex digit to value */
          int digit_value;
          if (c >= '0' && c <= '9') {
            digit_value = c - '0';
          } else if (c >= 'a' && c <= 'f') {
            digit_value = 10 + (c - 'a');
          } else {
            digit_value = 10 + (c - 'A');
          }
          token_value = token_value * 16 + digit_value;
          token_length++;
        } else if (isalnum((unsigned char)c)) {
          /* Invalid hex (contains non-hex digit) */
          token_length++;
          state = STATE_INVALID_HEX;
        } else {
          /* End of hex number */
          if (!add_token(lexer, TK_HEX, token_start, token_length,
                         &token_value)) {
            return false;
          }
          position += token_length;
          state = STATE_END;
        }
        break;

      case STATE_INVALID_OCTAL:
        if (isdigit((unsigned char)c)) {
          token_length++;
        } else {
          /* End of invalid octal */
          if (!add_token(lexer, TK_ILOCT, token_start, token_length, NULL)) {
            return false;
          }
          position += token_length;
          state = STATE_END;
        }
        break;

      case STATE_INVALID_HEX:
        if (isalnum((unsigned char)c)) {
          token_length++;
        } else {
          /* End of invalid hex */
          if (!add_token(lexer, TK_ILHEX, token_start, token_length, NULL)) {
            return false;
          }
          position += token_length;
          state = STATE_END;
        }
        break;

      case STATE_OPERATOR_START:
        if (token_start[0] == '>' && c == '=') {
          /* >= operator */
          if (!add_token(lexer, TK_GE, token_start, 2, NULL)) {
            return false;
          }
          position += 2;
          state = STATE_END;
        } else if (token_start[0] == '<' && c == '=') {
          /* <= operator */
          if (!add_token(lexer, TK_LE, token_start, 2, NULL)) {
            return false;
          }
          position += 2;
          state = STATE_END;
        } else if (token_start[0] == '<' && c == '>') {
          /* <> operator (not equal) */
          if (!add_token(lexer, TK_NEQ, token_start, 2, NULL)) {
            return false;
          }
          position += 2;
          state = STATE_END;
        } else {
          /* Single character operator */
          if (token_start[0] == '>') {
            if (!add_token(lexer, TK_GT, token_start, 1, NULL)) {
              return false;
            }
          } else if (token_start[0] == '<') {
            if (!add_token(lexer, TK_LT, token_start, 1, NULL)) {
              return false;
            }
          }
          position += 1;
          state = STATE_END;
        }
        break;

      case STATE_END:
        /* Should not reach here */
        break;
      }

      /* End of input check */
      if (input[position + token_length] == '\0' && state != STATE_END) {
        /* Handle end of input for each state */
        switch (state) {
        case STATE_IDENTIFIER:
          if (!add_token(lexer, TK_IDN, token_start, token_length, NULL)) {
            return false;
          }
          break;
        case STATE_DECIMAL:
          if (!add_token(lexer, TK_DEC, token_start, token_length,
                         &token_value)) {
            return false;
          }
          break;
        case STATE_OCTAL:
          if (!add_token(lexer, TK_OCT, token_start, token_length,
                         &token_value)) {
            return false;
          }
          break;
        case STATE_HEX:
          if (!add_token(lexer, TK_HEX, token_start, token_length,
                         &token_value)) {
            return false;
          }
          break;
        case STATE_INVALID_OCTAL:
          if (!add_token(lexer, TK_ILOCT, token_start, token_length, NULL)) {
            return false;
          }
          break;
        case STATE_INVALID_HEX:
        case STATE_HEX_PREFIX:
          if (!add_token(lexer, TK_ILHEX, token_start, token_length, NULL)) {
            return false;
          }
          break;
        case STATE_OPERATOR_START:
          /* Single character operator */
          if (token_start[0] == '>') {
            if (!add_token(lexer, TK_GT, token_start, 1, NULL)) {
              return false;
            }
          } else if (token_start[0] == '<') {
            if (!add_token(lexer, TK_LT, token_start, 1, NULL)) {
              return false;
            }
          }
          break;
        default:
          break;
        }
        position += token_length;
        state = STATE_END;
      }
    }
  }

  DEBUG_PRINT("State machine tokenization completed: %d tokens recognized",
              lexer->nr_token);
  return true;
}
