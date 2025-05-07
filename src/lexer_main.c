/**
 * @file lexer_main.c
 * @brief Lexical analyzer driver program
 */
#include "common.h"
#include "lexer_analyzer/lexer.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Command-line options */
static struct option long_options[] = {{"help", no_argument, NULL, 'h'},
                                       {"file", required_argument, NULL, 'f'},
                                       {"output", required_argument, NULL, 'o'},
                                       {NULL, 0, NULL, 0}};

/**
 * @brief Print usage information
 */
static void print_usage(const char *program_name) {
  printf("Usage: %s [options]\n", program_name);
  printf("Options:\n");
  printf("  -h, --help                Display this help message\n");
  printf("  -f, --file FILEPATH       Input file path (default: stdin)\n");
  printf("  -o, --output FILEPATH     Output file path (default: stdout)\n");
}

/**
 * @brief Read contents from stdin into a string
 */
static char *read_stdin() {
  size_t capacity = 1024;
  size_t length = 0;
  char *source = malloc(capacity);
  if (!source) {
    fprintf(stderr, "Out of memory\n");
    return NULL;
  }
  int ch;
  while ((ch = getchar()) != EOF) {
    /* Grow buffer if needed */
    if (length + 1 >= capacity) {
      capacity *= 2;
      char *newbuf = realloc(source, capacity);
      if (!newbuf) {
        free(source);
        fprintf(stderr, "Out of memory\n");
        return NULL;
      }
      source = newbuf;
    }
    source[length++] = (char)ch;
  }
  source[length] = '\0'; /* Null-terminate */
  return source;
}

/**
 * @brief Write tokenization results to a file
 */
static bool write_tokens_to_file(Lexer *lexer, const char *filename) {
  FILE *file = fopen(filename, "w");
  if (!file) {
    fprintf(stderr, "Error: Cannot open file '%s' for writing\n", filename);
    return false;
  }

  /* Save original stdout */
  int orig_stdout = dup(fileno(stdout));

  /* Redirect stdout to file */
  if (dup2(fileno(file), fileno(stdout)) == -1) {
    fprintf(stderr, "Error: Failed to redirect stdout\n");
    fclose(file);
    return false;
  }

  /* Print tokens */
  lexer_print_tokens(lexer);

  /* Restore stdout */
  fflush(stdout);
  dup2(orig_stdout, fileno(stdout));
  close(orig_stdout);

  fclose(file);
  return true;
}

int main(int argc, char *argv[]) {
  /* Parse command-line arguments */
  char *input_file = NULL;
  char *output_file = NULL;
  int c;
  int option_index = 0;
  while ((c = getopt_long(argc, argv, "hf:o:", long_options, &option_index)) !=
         -1) {
    switch (c) {
    case 'h':
      print_usage(argv[0]);
      return EXIT_SUCCESS;
    case 'f':
      input_file = optarg;
      break;
    case 'o':
      output_file = optarg;
      break;
    case '?':
      /* getopt_long already printed an error message */
      print_usage(argv[0]);
      return EXIT_FAILURE;
    default:
      return EXIT_FAILURE;
    }
  }

  printf("Compiler v%s\n", PROJECT_VERSION_STRING);

  /* Read input source */
  char *source;
  if (input_file) {
    printf("Reading source from file: %s\n", input_file);
    source = read_file(input_file);
  } else {
    printf("Reading source from stdin (end with Ctrl+D on Unix or Ctrl+Z on "
           "Windows)\n");
    source = read_stdin();
  }

  if (!source) {
    return EXIT_FAILURE;
  }

  /* Create and initialize lexer */
  Lexer *lexer = lexer_create();
  if (!lexer || !lexer_init(lexer)) {
    fprintf(stderr, "Failed to initialize lexer\n");
    free(source);
    lexer_destroy(lexer);
    return EXIT_FAILURE;
  }

  /* Tokenize input */
  printf("Tokenizing input...\n");
  if (!lexer_tokenize(lexer, source)) {
    fprintf(stderr, "Tokenization failed\n");
    lexer_destroy(lexer);
    free(source);
    return EXIT_FAILURE;
  }

  /* Print tokenization result */
  printf("\nTokenization successful!\n");

  /* Output tokenization results */
  if (output_file) {
    printf("Writing tokens to file: %s\n", output_file);
    if (!write_tokens_to_file(lexer, output_file)) {
      lexer_destroy(lexer);
      free(source);
      return EXIT_FAILURE;
    }
  } else {
    printf("\nTokenization result:\n");
    lexer_print_tokens(lexer);
  }

  /* Clean up */
  lexer_destroy(lexer);
  free(source);
  return EXIT_SUCCESS;
}
