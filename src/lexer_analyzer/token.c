/**
 * @file token.c
 * @brief Implementation of token functions
 */

#include "lexer_analyzer/token.h"
#include <stdio.h>
#include <string.h>

/**
 * Token type string representations
 */
static const char *token_type_strings[] = {
    "NOTYPE", "SPC",
    /* Keywords */
    "IF", "THEN", "ELSE", "WHILE", "DO", "BEGIN", "END",
    /* Operators */
    "ADD", "SUB", "MUL", "DIV", "GT", "LT", "EQ", "GE", "LE", "NEQ",
    /* Delimiters */
    "SLP", "SRP", "SEMI",
    /* Identifiers and literals */
    "IDN", "DEC", "OCT", "HEX", "ILOCT", "ILHEX", "EOF"};

/**
 * Get string representation of token type
 */
const char *token_type_to_string(TokenType type) {
  if (type >= 0 &&
      type < sizeof(token_type_strings) / sizeof(token_type_strings[0])) {
    return token_type_strings[type];
  }
  return "UNKNOWN";
}

/**
 * Format token into string
 */
void token_to_string(const Token *token, char *buffer, size_t buffer_size) {
  if (!token || !buffer || buffer_size == 0) {
    return;
  }

  const char *type_str = token_type_to_string(token->type);

  switch (token->type) {
  case TK_DEC:
  case TK_OCT:
  case TK_HEX:
    snprintf(buffer, buffer_size, "%-10s %d", type_str, token->num_val);
    break;
  case TK_IDN:
    snprintf(buffer, buffer_size, "%-10s %s", type_str, token->str_val);
    break;
  default:
    snprintf(buffer, buffer_size, "%-10s -", type_str);
    break;
  }
}

/**
 * Create a token with numeric value
 */
Token token_create_num(TokenType type, int value) {
  Token token;
  token.type = type;
  token.num_val = value;
  return token;
}

/**
 * Create a token with string value
 */
Token token_create_str(TokenType type, const char *value) {
  Token token;
  token.type = type;

  if (value) {
    strncpy(token.str_val, value, CONFIG_MAX_TOKEN_LEN - 1);
    token.str_val[CONFIG_MAX_TOKEN_LEN - 1] = '\0';
  } else {
    token.str_val[0] = '\0';
  }

  return token;
}

/**
 * Create a token with no value
 */
Token token_create(TokenType type) {
  Token token;
  token.type = type;
  token.num_val = 0; /* Initialize union to avoid undefined behavior */
  return token;
}
