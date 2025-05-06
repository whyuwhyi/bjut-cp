#include "lexer-analyzer/lexer.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Default rules for lexical analysis
 */
static Rule default_rules[NR_REGEX] = {
    {"[[:space:]]+", TK_SPC},
    {"\\<if\\>", TK_IF},
    {"\\<then\\>", TK_THEN},
    {"\\<else\\>", TK_ELSE},
    {"\\<while\\>", TK_WHILE},
    {"\\<do\\>", TK_DO},
    {"\\<begin\\>", TK_BEGIN},
    {"\\<end\\>", TK_END},
    {"\\+", TK_ADD},
    {"-", TK_SUB},
    {"\\*", TK_MUL},
    {"/", TK_DIV},
    {">=", TK_GE},
    {"<=", TK_LE},
    {"<>", TK_NEQ},
    {">", TK_GT},
    {"<", TK_LT},
    {"=", TK_EQ},
    {"\\(", TK_SLP},
    {"\\)", TK_SRP},
    {";", TK_SEMI},
    {"[a-zA-Z][a-zA-Z0-9]*", TK_IDN},
    {"0[0-7]*[8-9][0-9]*", TK_ILOCT},
    {"0[0-7]+", TK_OCT},
    {"0[xX][0-9a-fA-F]*[g-zG-Z]+[0-9a-zA-Z]*", TK_ILHEX},
    {"0[xX][0-9a-fA-F]+", TK_HEX},
    {"0|[1-9][0-9]*", TK_DEC}};

/**
 * Create a new lexer
 */
Lexer *lexer_create(void) {
  Lexer *lexer = (Lexer *)safe_malloc(sizeof(Lexer));

  memset(lexer, 0, sizeof(Lexer));
  memcpy(lexer->rules, default_rules, sizeof(default_rules));
  lexer->initialized = false;
  lexer->nr_token = 0;

  DEBUG_PRINT("Lexer created");
  return lexer;
}

/**
 * Destroy a lexer and free all resources
 */
void lexer_destroy(Lexer *lexer) {
  if (!lexer) {
    return;
  }

  /* Free regex resources if initialized */
  if (lexer->initialized) {
    for (int i = 0; i < NR_REGEX; i++) {
      regfree(&lexer->re[i]);
    }
  }

  DEBUG_PRINT("Lexer destroyed");
  free(lexer);
}

/**
 * Initialize the lexer with regular expressions
 */
bool lexer_init(Lexer *lexer) {
  if (!lexer) {
    return false;
  }

  /* Already initialized */
  if (lexer->initialized) {
    return true;
  }

  DEBUG_PRINT("Initializing lexer with %d rules", NR_REGEX);

  char error_msg[128];
  for (int i = 0; i < NR_REGEX; i++) {
    DEBUG_PRINT("Compiling regex pattern: %s", lexer->rules[i].regex);

    int ret = regcomp(&lexer->re[i], lexer->rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &lexer->re[i], error_msg, sizeof(error_msg));
      fprintf(stderr, "Regex compilation failed: %s\n%s\n", error_msg,
              lexer->rules[i].regex);

      /* Free previously compiled regex */
      for (int j = 0; j < i; j++) {
        regfree(&lexer->re[j]);
      }
      return false;
    }
  }

  lexer->initialized = true;
  DEBUG_PRINT("Lexer initialization completed successfully");
  return true;
}

/**
 * Tokenize an input string
 */
bool lexer_tokenize(Lexer *lexer, const char *input) {
  if (!lexer || !input || !lexer->initialized) {
    return false;
  }

  lexer->nr_token = 0;
  int position = 0;

  DEBUG_PRINT("Starting tokenization of input (length: %zu)", strlen(input));

  while (input[position] != '\0') {
    bool match_found = false;
    regmatch_t pmatch;

    /* Try to match each regex pattern */
    for (int i = 0; i < NR_REGEX; i++) {
      if (regexec(&lexer->re[i], input + position, 1, &pmatch, 0) == 0 &&
          pmatch.rm_so == 0) {
        const char *substr_start = input + position;
        int substr_len = pmatch.rm_eo;

        /* Check for token buffer overflow */
        if (lexer->nr_token >= CONFIG_MAX_TOKENS) {
          fprintf(stderr, "Too many tokens (max: %d)\n", CONFIG_MAX_TOKENS);
          return false;
        }

        /* Check for token length overflow (skip for whitespace) */
        if (substr_len >= CONFIG_MAX_TOKEN_LEN &&
            lexer->rules[i].token_type != TK_SPC) {
          fprintf(stderr, "Token at position %d is too long: %.*s\n", position,
                  substr_len, substr_start);
          return false;
        }

        DEBUG_PRINT("Match '%s' at position %d: %.*s",
                    token_type_to_string(lexer->rules[i].token_type), position,
                    substr_len, substr_start);

        position += substr_len;
        TokenType type = lexer->rules[i].token_type;

        /* Skip whitespace tokens */
        if (type == TK_SPC) {
          match_found = true;
          break;
        }

        /* Add the token */
        Token *token = &lexer->tokens[lexer->nr_token++];
        token->type = type;

        switch (type) {
        case TK_DEC:
          sscanf(substr_start, "%d", &token->num_val);
          DEBUG_PRINT("Added decimal token: %d", token->num_val);
          break;
        case TK_OCT:
          sscanf(substr_start + 1, "%o",
                 &token->num_val); /* Skip the leading '0' */
          DEBUG_PRINT("Added octal token: %o (%d decimal)", token->num_val,
                      token->num_val);
          break;
        case TK_HEX:
          sscanf(substr_start + 2, "%x",
                 &token->num_val); /* Skip the leading '0x' or '0X' */
          DEBUG_PRINT("Added hex token: 0x%x (%d decimal)", token->num_val,
                      token->num_val);
          break;
        case TK_IDN:
        case TK_ILOCT:
        case TK_ILHEX:
          /* Copy the string value */
          strncpy(token->str_val, substr_start, substr_len);
          token->str_val[substr_len] = '\0';
          DEBUG_PRINT("Added string token: %s", token->str_val);
          break;
        default:
          /* Nothing to do for operators and delimiters */
          DEBUG_PRINT("Added token type: %s", token_type_to_string(type));
          break;
        }

        match_found = true;
        break;
      }
    }

    /* Skip unrecognized characters with a warning */
    if (!match_found) {
      fprintf(stderr, "No match at position %d: '%c'\n", position,
              input[position]);
      position++;
    }
  }

  DEBUG_PRINT("Tokenization completed: %d tokens recognized", lexer->nr_token);
  return true;
}

/**
 * Print all tokens in the lexer
 */
void lexer_print_tokens(Lexer *lexer) {
  if (!lexer) {
    return;
  }

  DEBUG_PRINT("Printing %d tokens", lexer->nr_token);

  char buffer[CONFIG_MAX_TOKEN_LEN * 2];
  for (int i = 0; i < lexer->nr_token; i++) {
    token_to_string(&lexer->tokens[i], buffer, sizeof(buffer));
    printf("%s\n", buffer);
  }
}

/**
 * Get the token at a specific index
 */
const Token *lexer_get_token(const Lexer *lexer, int index) {
  if (!lexer || index < 0 || index >= lexer->nr_token) {
    return NULL;
  }
  return &lexer->tokens[index];
}

/**
 * Get the number of tokens in the lexer
 */
int lexer_token_count(const Lexer *lexer) {
  return lexer ? lexer->nr_token : 0;
}
