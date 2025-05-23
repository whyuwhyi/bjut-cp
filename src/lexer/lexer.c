#include "lexer/lexer.h"
#include "error_handler.h"
#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CONFIG_LEXER_REGEX
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
#endif

/**
 * Check if lexer has encountered any errors
 */
bool lexer_has_errors(const Lexer *lexer) {
  return lexer ? lexer->has_error : false;
}

/**
 * Create a new lexer
 */
Lexer *lexer_create(void) {
  Lexer *lexer = (Lexer *)safe_malloc(sizeof(Lexer));

  memset(lexer, 0, sizeof(Lexer));

#ifdef CONFIG_LEXER_REGEX
  memcpy(lexer->rules, default_rules, sizeof(default_rules));
#endif

  lexer->initialized = false;
  lexer->nr_token = 0;
  lexer->has_error = false;
  lexer->error_count = 0;
  lexer->current_line = 1;
  lexer->current_column = 1;
  lexer->input = NULL;

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

#ifdef CONFIG_LEXER_REGEX
  /* Free regex resources if initialized */
  if (lexer->initialized) {
    for (int i = 0; i < NR_REGEX; i++) {
      regfree(&lexer->re[i]);
    }
  }
#endif

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

#ifdef CONFIG_LEXER_REGEX
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
#endif

#ifdef CONFIG_LEXER_STATE_MACHINE
  DEBUG_PRINT("Initializing state machine lexer");
  /* No specific initialization needed for state machine implementation */
#endif

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

  // Store input reference for error reporting
  lexer->input = input;
  lexer->has_error = false;
  lexer->error_count = 0;
  lexer->current_line = 1;
  lexer->current_column = 1;

#ifdef CONFIG_LEXER_REGEX
#ifdef CONFIG_LEXER_STATE_MACHINE
  /* Log which implementation is being used */
  DEBUG_PRINT("Using regular expression based lexer (both methods available)");
#endif
  return lexer_tokenize_regex(lexer, input);
#else
#ifdef CONFIG_LEXER_STATE_MACHINE
  /* Only state machine implementation is available */
  DEBUG_PRINT("Using state machine based lexer");
  return lexer_tokenize_state_machine(lexer, input);
#else
  /* Neither implementation is available - configuration error */
  fprintf(stderr, "No lexer implementation is enabled in configuration\n");
  return false;
#endif
#endif
}

#ifdef CONFIG_LEXER_REGEX
/**
 * Tokenize an input string using regular expressions
 */
bool lexer_tokenize_regex(Lexer *lexer, const char *input) {
  if (!lexer || !input || !lexer->initialized) {
    return false;
  }

  lexer->nr_token = 0;
  lexer->has_error = false;
  lexer->error_count = 0;
  lexer->current_line = 1;
  lexer->current_column = 1;
  int position = 0;

  DEBUG_PRINT("Starting regex tokenization of input (length: %zu)",
              strlen(input));

  while (input[position] != '\0') {
    bool match_found = false;
    regmatch_t pmatch;
    int current_line = lexer->current_line;
    int current_column = lexer->current_column;

    /* Try to match each regex pattern */
    for (int i = 0; i < NR_REGEX; i++) {
      if (regexec(&lexer->re[i], input + position, 1, &pmatch, 0) == 0 &&
          pmatch.rm_so == 0) {
        const char *substr_start = input + position;
        int substr_len = pmatch.rm_eo;

        /* Check for token buffer overflow */
        if (lexer->nr_token >= CONFIG_MAX_TOKENS) {
          lexer_report_error(lexer, current_line, current_column, 0,
                             "Too many tokens (max: %d)", CONFIG_MAX_TOKENS);
          lexer->has_error = true;
          return false;
        }

        /* Check for token length overflow (skip for whitespace) */
        if (substr_len >= CONFIG_MAX_TOKEN_LEN &&
            lexer->rules[i].token_type != TK_SPC) {
          lexer_report_error(lexer, current_line, current_column, substr_len,
                             "Token too long: %.*s", substr_len, substr_start);
          lexer->has_error = true;
        }

        DEBUG_PRINT("Match '%s' at position %d: %.*s",
                    token_type_to_string(lexer->rules[i].token_type), position,
                    substr_len, substr_start);

        /* Update line and column counters */
        for (int j = 0; j < substr_len; j++) {
          if (substr_start[j] == '\n') {
            lexer->current_line++;
            lexer->current_column = 1;
          } else {
            lexer->current_column++;
          }
        }

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
        token->line = current_line;
        token->column = current_column;

        switch (type) {
        case TK_DEC:
          sscanf(substr_start, "%d", &token->num_val);
          DEBUG_PRINT("Added decimal token: %d at line %d, column %d",
                      token->num_val, current_line, current_column);
          break;
        case TK_OCT:
          sscanf(substr_start + 1, "%o",
                 &token->num_val); /* Skip the leading '0' */
          DEBUG_PRINT(
              "Added octal token: %o (%d decimal) at line %d, column %d",
              token->num_val, token->num_val, current_line, current_column);
          break;
        case TK_HEX:
          sscanf(substr_start + 2, "%x",
                 &token->num_val); /* Skip the leading '0x' or '0X' */
          DEBUG_PRINT(
              "Added hex token: 0x%x (%d decimal) at line %d, column %d",
              token->num_val, token->num_val, current_line, current_column);
          break;
        case TK_IDN:
        case TK_ILOCT:
        case TK_ILHEX:
          /* Copy the string value */
          if (substr_len < CONFIG_MAX_TOKEN_LEN) {
            strncpy(token->str_val, substr_start, substr_len);
            token->str_val[substr_len] = '\0';
            DEBUG_PRINT("Added string token: %s at line %d, column %d",
                        token->str_val, current_line, current_column);
          }
          break;
        default:
          /* Nothing to do for operators and delimiters */
          DEBUG_PRINT("Added token type: %s at line %d, column %d",
                      token_type_to_string(type), current_line, current_column);
          break;
        }

        match_found = true;
        break;
      }
    }

    /* Handle unrecognized characters */
    if (!match_found) {
      lexer_report_error(lexer, current_line, current_column, 1,
                         "Unrecognized character: '%c'", input[position]);

      // Update position and column
      if (input[position] == '\n') {
        lexer->current_line++;
        lexer->current_column = 1;
      } else {
        lexer->current_column++;
      }
      position++;
      lexer->has_error = true;
    }
  }

  // Add EOF token
  if (lexer->nr_token < CONFIG_MAX_TOKENS) {
    Token *token = &lexer->tokens[lexer->nr_token++];
    token->type = TK_EOF;
    token->line = lexer->current_line;
    token->column = lexer->current_column;
  }

  DEBUG_PRINT("Regex tokenization completed: %d tokens recognized, %d errors",
              lexer->nr_token, lexer->error_count);
  return !lexer->has_error;
}
#endif

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
    token_to_string_famt(&lexer->tokens[i], buffer, sizeof(buffer));
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
