/**
 * @file generator.c
 * @brief Implementation of sample generator for lexical analyzer testing
 */

#include "generator.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Platform-specific compatibility */
#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#define STRDUP _strdup
#else
#define PATH_SEPARATOR '/'
#define STRDUP strdup
#endif

/* Private constants */
static const char *operators[] = {"+",  "-",  "*",  "/", ">", "<", "=",
                                  ">=", "<=", "<>", "(", ")", ";"};

static const char *keywords[] = {"if", "then",  "else", "while",
                                 "do", "begin", "end"};

/* Private helper functions */
static char *string_dup(const char *str) {
  if (!str)
    return NULL;
  return STRDUP(str);
}

/**
 * Generate a random identifier
 */
static char *generate_random_identifier(int max_length) {
  int length = rand() % max_length + 1;
  char *id = (char *)malloc(length + 1);
  if (!id)
    return NULL;

  /* First character must be a letter */
  id[0] = 'a' + rand() % 26;

  /* Rest can be letters or digits */
  for (int i = 1; i < length; i++) {
    int choice = rand() % 36; /* 26 letters + 10 digits */
    if (choice < 26) {
      id[i] = 'a' + choice;
    } else {
      id[i] = '0' + (choice - 26);
    }
  }

  id[length] = '\0';
  return id;
}

/**
 * Generate a random decimal number
 */
static char *generate_random_decimal(int max_length) {
  int length = rand() % max_length + 1;
  char *num = (char *)malloc(length + 1);
  if (!num)
    return NULL;

  if (length == 1) {
    /* Single digit */
    num[0] = '0' + rand() % 10;
  } else {
    /* Multi-digit number (non-zero first digit) */
    num[0] = '1' + rand() % 9;
    for (int i = 1; i < length; i++) {
      num[i] = '0' + rand() % 10;
    }
  }

  num[length] = '\0';
  return num;
}

/**
 * Generate a random octal number
 */
static char *generate_random_octal(int max_length) {
  int length = rand() % max_length + 1;
  char *num =
      (char *)malloc(length + 2); /* +1 for the leading '0', +1 for '\0' */
  if (!num)
    return NULL;

  num[0] = '0'; /* Leading '0' for octal */

  for (int i = 1; i <= length; i++) {
    num[i] = '0' + rand() % 8; /* Octal digits (0-7) */
  }

  num[length + 1] = '\0';
  return num;
}

/**
 * Generate a random hexadecimal number
 */
static char *generate_random_hex(int max_length) {
  int length = rand() % max_length + 1;
  char *num =
      (char *)malloc(length + 3); /* +2 for the leading '0x', +1 for '\0' */
  if (!num)
    return NULL;

  num[0] = '0';
  num[1] = 'x';

  static const char hex_chars[] = "0123456789abcdef";

  for (int i = 0; i < length; i++) {
    num[i + 2] = hex_chars[rand() % 16];
  }

  num[length + 2] = '\0';
  return num;
}

/**
 * Generate an invalid octal number
 */
static char *generate_invalid_octal(int max_length) {
  int length =
      rand() % max_length + 2; /* At least 2 digits (0 + invalid digit) */
  char *num = (char *)malloc(length + 1);
  if (!num)
    return NULL;

  num[0] = '0'; /* Leading '0' for octal */

  /* Ensure at least one invalid octal digit (8 or 9) */
  int invalid_pos = 1 + rand() % (length - 1);

  for (int i = 1; i < length; i++) {
    if (i == invalid_pos) {
      num[i] = '8' + rand() % 2; /* 8 or 9 (invalid octal) */
    } else {
      num[i] = '0' + rand() % 10; /* Any digit */
    }
  }

  num[length] = '\0';
  return num;
}

/**
 * Generate an invalid hexadecimal number
 */
static char *generate_invalid_hex(int max_length) {
  int length = rand() % max_length + 2; /* At least 2 digits */
  char *num =
      (char *)malloc(length + 3); /* +2 for the leading '0x', +1 for '\0' */
  if (!num)
    return NULL;

  num[0] = '0';
  num[1] = 'x';

  static const char hex_chars[] = "0123456789abcdef";
  static const char invalid_hex_chars[] = "ghijklmnopqrstuvwxyz";

  /* Ensure at least one invalid hex digit (g-z) */
  int invalid_pos = rand() % length;

  for (int i = 0; i < length; i++) {
    if (i == invalid_pos) {
      num[i + 2] = invalid_hex_chars[rand() % 20]; /* Invalid hex char */
    } else {
      num[i + 2] = hex_chars[rand() % 16]; /* Valid hex char */
    }
  }

  num[length + 2] = '\0';
  return num;
}

/**
 * Generate a random operator
 */
static char *generate_random_operator(void) {
  int index = rand() % (sizeof(operators) / sizeof(operators[0]));
  return string_dup(operators[index]);
}

/**
 * Generate a random keyword
 */
static char *generate_random_keyword(void) {
  int index = rand() % (sizeof(keywords) / sizeof(keywords[0]));
  return string_dup(keywords[index]);
}

/**
 * Generate a random expression
 */
static char *generate_random_expression(int max_terms) {
  int terms = rand() % max_terms + 1;
  char *expression =
      (char *)malloc(1024); /* Allocate a reasonably large buffer */
  if (!expression)
    return NULL;

  expression[0] = '\0';

  for (int i = 0; i < terms; i++) {
    char *term;
    int type = rand() % 5;

    switch (type) {
    case 0:
      term = generate_random_identifier(8);
      break;
    case 1:
      term = generate_random_decimal(5);
      break;
    case 2:
      term = generate_random_octal(4);
      break;
    case 3:
      term = generate_random_hex(4);
      break;
    case 4:
      term = generate_random_operator();
      break;
    default:
      term = string_dup(" ");
      break;
    }

    strcat(expression, term);
    strcat(expression, " ");
    free(term);
  }

  return expression;
}

/**
 * Generate a random program
 */
static char *generate_random_program(int max_lines) {
  int lines = rand() % max_lines + 1;
  char *program = (char *)malloc(4096); /* Allocate a reasonably large buffer */
  if (!program)
    return NULL;

  program[0] = '\0';

  for (int i = 0; i < lines; i++) {
    int type = rand() % 3;
    char *line;

    switch (type) {
    case 0:
      line = generate_random_expression(5);
      break;
    case 1:
      line = generate_random_keyword();
      strcat(program, line);
      free(line);
      strcat(program, " ");
      line = generate_random_expression(3);
      break;
    case 2:
      if (rand() % 2 == 0) {
        line = generate_invalid_octal(4);
      } else {
        line = generate_invalid_hex(4);
      }
      break;
    default:
      line = string_dup("");
      break;
    }

    strcat(program, line);
    strcat(program, ";\n");
    free(line);
  }

  return program;
}

/**
 * Generate expected output for a token
 */
static char *generate_expected_output_for_token(const char *token) {
  char *output = (char *)malloc(256);
  if (!output)
    return NULL;

  output[0] = '\0';

  /* Determine token type based on content */
  if (strcmp(token, "if") == 0) {
    strcpy(output, "IF       -\n");
  } else if (strcmp(token, "then") == 0) {
    strcpy(output, "THEN     -\n");
  } else if (strcmp(token, "else") == 0) {
    strcpy(output, "ELSE     -\n");
  } else if (strcmp(token, "while") == 0) {
    strcpy(output, "WHILE    -\n");
  } else if (strcmp(token, "do") == 0) {
    strcpy(output, "DO       -\n");
  } else if (strcmp(token, "begin") == 0) {
    strcpy(output, "BEGIN    -\n");
  } else if (strcmp(token, "end") == 0) {
    strcpy(output, "END      -\n");
  } else if (strcmp(token, "+") == 0) {
    strcpy(output, "ADD      -\n");
  } else if (strcmp(token, "-") == 0) {
    strcpy(output, "SUB      -\n");
  } else if (strcmp(token, "*") == 0) {
    strcpy(output, "MUL      -\n");
  } else if (strcmp(token, "/") == 0) {
    strcpy(output, "DIV      -\n");
  } else if (strcmp(token, ">") == 0) {
    strcpy(output, "GT       -\n");
  } else if (strcmp(token, "<") == 0) {
    strcpy(output, "LT       -\n");
  } else if (strcmp(token, "=") == 0) {
    strcpy(output, "EQ       -\n");
  } else if (strcmp(token, ">=") == 0) {
    strcpy(output, "GE       -\n");
  } else if (strcmp(token, "<=") == 0) {
    strcpy(output, "LE       -\n");
  } else if (strcmp(token, "<>") == 0) {
    strcpy(output, "NEQ      -\n");
  } else if (strcmp(token, "(") == 0) {
    strcpy(output, "SLP      -\n");
  } else if (strcmp(token, ")") == 0) {
    strcpy(output, "SRP      -\n");
  } else if (strcmp(token, ";") == 0) {
    strcpy(output, "SEMI     -\n");
  } else if (token[0] == '0' && token[1] == 'x') {
    /* Check if it's a valid hexadecimal */
    int valid = 1;
    for (int i = 2; token[i] != '\0'; i++) {
      if (!((token[i] >= '0' && token[i] <= '9') ||
            (token[i] >= 'a' && token[i] <= 'f') ||
            (token[i] >= 'A' && token[i] <= 'F'))) {
        valid = 0;
        break;
      }
    }

    if (valid) {
      int value;
      sscanf(token + 2, "%x", &value);
      sprintf(output, "HEX      %d\n", value);
    } else {
      sprintf(output, "ILHEX    %s\n", token);
    }
  } else if (token[0] == '0' && token[1] != '\0') {
    /* Check if it's a valid octal */
    int valid = 1;
    for (int i = 1; token[i] != '\0'; i++) {
      if (token[i] < '0' || token[i] > '7') {
        valid = 0;
        break;
      }
    }

    if (valid) {
      int value;
      sscanf(token + 1, "%o", &value);
      sprintf(output, "OCT      %d\n", value);
    } else {
      sprintf(output, "ILOCT    %s\n", token);
    }
  } else if (token[0] >= '0' && token[0] <= '9') {
    /* Decimal number */
    int value;
    sscanf(token, "%d", &value);
    sprintf(output, "DEC      %d\n", value);
  } else {
    /* Default to identifier */
    sprintf(output, "IDN      %s\n", token);
  }

  return output;
}

/**
 * Parse a program and generate expected output
 */
static char *generate_expected_output(const char *program) {
  char *expected_output = (char *)malloc(8192); /* Sufficiently large buffer */
  if (!expected_output)
    return NULL;

  expected_output[0] = '\0';

  /* Copy the program to avoid modifying the original */
  char program_copy[4096];
  strncpy(program_copy, program, sizeof(program_copy));
  program_copy[sizeof(program_copy) - 1] = '\0';

  /* Tokenize the program */
  char *context = NULL;
#ifdef _WIN32
  char *token = strtok_s(program_copy, " \t\n;()", &context);
#else
  char *token = strtok_r(program_copy, " \t\n;()", &context);
#endif

  while (token != NULL) {
    if (strlen(token) > 0) {
      char *token_output = generate_expected_output_for_token(token);
      strcat(expected_output, token_output);
      free(token_output);
    }

#ifdef _WIN32
    token = strtok_s(NULL, " \t\n;()", &context);
#else
    token = strtok_r(NULL, " \t\n;()", &context);
#endif
  }

  /* Process operators and delimiters */
  for (size_t i = 0; program[i] != '\0'; i++) {
    char ch = program[i];
    if (ch == '(' || ch == ')' || ch == ';') {
      char temp[2] = {ch, '\0'};
      char *op_output = generate_expected_output_for_token(temp);
      strcat(expected_output, op_output);
      free(op_output);
    }
  }

  return expected_output;
}

/**
 * Generate lexical analyzer test samples
 */
int generate_lexer_samples(int count, const char *filename, unsigned int seed) {
  FILE *file = NULL;

  /* Open output file if specified */
  if (filename) {
    file = fopen(filename, "w");
    if (!file) {
      perror("Error opening output file");
      return -1;
    }
  }

  /* Set random seed */
  if (seed == 0) {
    seed = (unsigned int)time(NULL);
  }
  srand(seed);

  /* Generate samples */
  for (int i = 0; i < count; i++) {
    /* Generate random program */
    char *program = generate_random_program(rand() % 5 + 1);
    if (!program) {
      if (file)
        fclose(file);
      return -1;
    }

    /* Generate expected output */
    char *expected_output = generate_expected_output(program);
    if (!expected_output) {
      free(program);
      if (file)
        fclose(file);
      return -1;
    }

    /* Write to file or stdout */
    if (file) {
      fprintf(file, "sample:\n%s\n", program);
      fprintf(file, "expected output:\n%s\n", expected_output);
    } else {
      printf("sample:\n%s\n", program);
      printf("expected output:\n%s\n", expected_output);
    }

    /* Clean up */
    free(program);
    free(expected_output);
  }

  /* Close file if opened */
  if (file) {
    fclose(file);
  }

  return count;
}

/**
 * @brief Main entry: parse command-line arguments and generate samples
 *
 * Supported options:
 *   -n <count>   number of samples (default 1)
 *   -o <outfile> output file (default stdout)
 *   -s <seed>    random seed (default time(NULL))
 */
int main(int argc, char **argv) {
  int count = 1;
  unsigned int seed = 0;
  const char *outfile = NULL;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
      count = atoi(argv[++i]);
      if (count <= 0) {
        fprintf(stderr, "Invalid count: %s\n", argv[i]);
        return 1;
      }
    } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
      outfile = argv[++i];
    } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
      seed = (unsigned int)atoi(argv[++i]);
    } else {
      fprintf(stderr, "Usage: %s [-n count] [-o outfile] [-s seed]\n", argv[0]);
      return 1;
    }
  }

  int ret = generate_lexer_samples(count, outfile, seed);
  if (ret < 0) {
    fprintf(stderr, "Error: failed to generate samples\n");
    return 1;
  }
  return 0;
}
