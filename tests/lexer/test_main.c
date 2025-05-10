/**
 * @file test_main.c
 * @brief Integration test for the lexical analyzer
 */

#include "../unittest.h"
#include "common.h"
#include "lexer/lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

TestSuite *current_suite = NULL;
Test *current_test = NULL;

/* Test process structure to handle example */
typedef struct {
  char *input;
  int num_tokens;
  TokenType *token_types;
} TokenTest;

/* Free resources used by a TokenTest */
static void free_token_test(TokenTest *test) {
  if (test) {
    free(test->input);
    free(test->token_types);
  }
}

/* Process a sample and extract the input and expected output */
static TokenTest process_sample(const char *sample) {
  TokenTest test = {NULL, 0, NULL};

  /* Find the separator between input and expected output */
  const char *expected_output_marker = strstr(sample, "expected output:");
  if (!expected_output_marker) {
    fprintf(stderr,
            "Invalid sample format: missing 'expected output:' marker\n");
    return test;
  }

  /* Extract the input part */
  const char *input_marker = strstr(sample, "sample:");
  if (!input_marker) {
    fprintf(stderr, "Invalid sample format: missing 'sample:' marker\n");
    return test;
  }

  /* Calculate the input length */
  input_marker += strlen("sample:");
  size_t input_len = expected_output_marker - input_marker;

  /* Trim whitespace */
  while (input_len > 0 && (input_marker[0] == ' ' || input_marker[0] == '\n' ||
                           input_marker[0] == '\r')) {
    input_marker++;
    input_len--;
  }
  while (input_len > 0 && (input_marker[input_len - 1] == ' ' ||
                           input_marker[input_len - 1] == '\n' ||
                           input_marker[input_len - 1] == '\r')) {
    input_len--;
  }

  /* Copy the input */
  test.input = (char *)malloc(input_len + 1);
  if (!test.input) {
    fprintf(stderr, "Memory allocation failed\n");
    return test;
  }
  strncpy(test.input, input_marker, input_len);
  test.input[input_len] = '\0';

  /* Count the expected tokens */
  const char *expected_output =
      expected_output_marker + strlen("expected output:");
  int token_count = 0;
  const char *line_start = expected_output;

  while ((line_start = strstr(line_start, "\n")) != NULL) {
    line_start++; /* Skip the newline */
    if (strncmp(line_start, "sample:", 7) == 0) {
      /* Reached the next sample */
      break;
    }
    if (*line_start == '\0' || *line_start == '\n' || *line_start == '\r') {
      /* Empty line or end of string */
      continue;
    }
    token_count++;
  }

  /* Allocate memory for the expected token types */
  test.num_tokens = token_count;
  test.token_types = (TokenType *)malloc(token_count * sizeof(TokenType));
  if (!test.token_types) {
    fprintf(stderr, "Memory allocation failed\n");
    free(test.input);
    test.input = NULL;
    test.num_tokens = 0;
    return test;
  }

  /* Parse the expected tokens */
  line_start = expected_output;
  int token_index = 0;

  while ((line_start = strstr(line_start, "\n")) != NULL &&
         token_index < token_count) {
    line_start++; /* Skip the newline */
    if (strncmp(line_start, "sample:", 7) == 0) {
      /* Reached the next sample */
      break;
    }
    if (*line_start == '\0' || *line_start == '\n' || *line_start == '\r') {
      /* Empty line or end of string */
      continue;
    }

    /* Parse the token type */
    char token_type_str[20];
    if (sscanf(line_start, "%s", token_type_str) == 1) {
      /* Convert the string token type to enum */
      if (strcmp(token_type_str, "IF") == 0) {
        test.token_types[token_index] = TK_IF;
      } else if (strcmp(token_type_str, "THEN") == 0) {
        test.token_types[token_index] = TK_THEN;
      } else if (strcmp(token_type_str, "ELSE") == 0) {
        test.token_types[token_index] = TK_ELSE;
      } else if (strcmp(token_type_str, "WHILE") == 0) {
        test.token_types[token_index] = TK_WHILE;
      } else if (strcmp(token_type_str, "DO") == 0) {
        test.token_types[token_index] = TK_DO;
      } else if (strcmp(token_type_str, "BEGIN") == 0) {
        test.token_types[token_index] = TK_BEGIN;
      } else if (strcmp(token_type_str, "END") == 0) {
        test.token_types[token_index] = TK_END;
      } else if (strcmp(token_type_str, "ADD") == 0) {
        test.token_types[token_index] = TK_ADD;
      } else if (strcmp(token_type_str, "SUB") == 0) {
        test.token_types[token_index] = TK_SUB;
      } else if (strcmp(token_type_str, "MUL") == 0) {
        test.token_types[token_index] = TK_MUL;
      } else if (strcmp(token_type_str, "DIV") == 0) {
        test.token_types[token_index] = TK_DIV;
      } else if (strcmp(token_type_str, "GT") == 0) {
        test.token_types[token_index] = TK_GT;
      } else if (strcmp(token_type_str, "LT") == 0) {
        test.token_types[token_index] = TK_LT;
      } else if (strcmp(token_type_str, "EQ") == 0) {
        test.token_types[token_index] = TK_EQ;
      } else if (strcmp(token_type_str, "GE") == 0) {
        test.token_types[token_index] = TK_GE;
      } else if (strcmp(token_type_str, "LE") == 0) {
        test.token_types[token_index] = TK_LE;
      } else if (strcmp(token_type_str, "NEQ") == 0) {
        test.token_types[token_index] = TK_NEQ;
      } else if (strcmp(token_type_str, "SLP") == 0) {
        test.token_types[token_index] = TK_SLP;
      } else if (strcmp(token_type_str, "SRP") == 0) {
        test.token_types[token_index] = TK_SRP;
      } else if (strcmp(token_type_str, "SEMI") == 0) {
        test.token_types[token_index] = TK_SEMI;
      } else if (strcmp(token_type_str, "IDN") == 0) {
        test.token_types[token_index] = TK_IDN;
      } else if (strcmp(token_type_str, "DEC") == 0) {
        test.token_types[token_index] = TK_DEC;
      } else if (strcmp(token_type_str, "OCT") == 0) {
        test.token_types[token_index] = TK_OCT;
      } else if (strcmp(token_type_str, "HEX") == 0) {
        test.token_types[token_index] = TK_HEX;
      } else if (strcmp(token_type_str, "ILOCT") == 0) {
        test.token_types[token_index] = TK_ILOCT;
      } else if (strcmp(token_type_str, "ILHEX") == 0) {
        test.token_types[token_index] = TK_ILHEX;
      } else if (strcmp(token_type_str, "EOF") == 0) {
        test.token_types[token_index] = TK_EOF;
      } else {
        fprintf(stderr, "Unknown token type: %s\n", token_type_str);
        test.token_types[token_index] = TK_NOTYPE;
      }
    }

    token_index++;
  }

  return test;
}

/**
 * Run a lexical analysis test on a sample
 */
static bool run_lexer_test(const char *sample) {
  /* Process the sample */
  TokenTest test = process_sample(sample);
  if (!test.input || !test.token_types) {
    fprintf(stderr, "Failed to process sample\n");
    free_token_test(&test);
    return false;
  }

  /* Create and initialize the lexer */
  Lexer *lexer = lexer_create();
  if (!lexer) {
    fprintf(stderr, "Failed to create lexer\n");
    free_token_test(&test);
    return false;
  }

  if (!lexer_init(lexer)) {
    fprintf(stderr, "Failed to initialize lexer\n");
    lexer_destroy(lexer);
    free_token_test(&test);
    return false;
  }

  /* Tokenize the input */
  printf("Testing input:\n%s\n", test.input);
  if (!lexer_tokenize(lexer, test.input)) {
    fprintf(stderr, "Tokenization failed\n");
    lexer_destroy(lexer);
    free_token_test(&test);
    return false;
  }

  /* Print the tokens */
  printf("Tokenization result:\n");
  lexer_print_tokens(lexer);

  /* Verify the tokens */
  bool success = true;
  int token_count = lexer_token_count(lexer);

  if (token_count != test.num_tokens) {
    fprintf(stderr, "Token count mismatch: expected %d, got %d\n",
            test.num_tokens, token_count);
    success = false;
  } else {
    for (int i = 0; i < token_count; i++) {
      const Token *token = lexer_get_token(lexer, i);
      if (token->type != test.token_types[i]) {
        fprintf(stderr,
                "Token type mismatch at index %d: expected %s, got %s\n", i,
                token_type_to_string(test.token_types[i]),
                token_type_to_string(token->type));
        success = false;
        break;
      }
    }
  }

  /* Clean up */
  lexer_destroy(lexer);
  free_token_test(&test);

  return success;
}

/**
 * Main function
 */
int main(int argc, char *argv[]) {
  /* Check if a sample file was provided */
  if (argc >= 2) {
    printf("Reading samples from file: %s\n", argv[1]);
    char *samples = read_file(argv[1]);
    if (!samples) {
      fprintf(stderr, "Failed to read sample file: %s\n", argv[1]);
      return EXIT_FAILURE;
    }

    /* Split the file into samples */
    char *sample = samples;
    char *next_sample;
    int sample_count = 0;
    int pass_count = 0;

    while ((next_sample = strstr(sample + 1, "sample:")) != NULL) {
      /* Null-terminate the current sample */
      *(next_sample - 1) = '\0';

      /* Process the sample */
      sample_count++;
      printf("\n=== Sample %d ===\n", sample_count);
      if (run_lexer_test(sample)) {
        printf("Test passed!\n");
        pass_count++;
      } else {
        printf("Test failed!\n");
      }

      /* Move to the next sample */
      sample = next_sample;
    }

    /* Process the last sample */
    sample_count++;
    printf("\n=== Sample %d ===\n", sample_count);
    if (run_lexer_test(sample)) {
      printf("Test passed!\n");
      pass_count++;
    } else {
      printf("Test failed!\n");
    }

    /* Print test summary */
    printf("\n=== Test Summary ===\n");
    printf("Total samples: %d\n", sample_count);
    printf("Passed: %d\n", pass_count);
    printf("Failed: %d\n", sample_count - pass_count);

    /* Clean up */
    free(samples);

    return (pass_count == sample_count) ? EXIT_SUCCESS : EXIT_FAILURE;
  } else {
    /* No sample file provided, generate samples and test */
    printf("Usage: ./build/test_main samples.txt\n");
    return EXIT_FAILURE;
  }
}
