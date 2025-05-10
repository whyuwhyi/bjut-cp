/**
 * @file common.h
 * @brief Common header file included by all source files in the project
 */

#ifndef COMMON_H
#define COMMON_H

/* Include auto-generated configuration */
#include "generated/autoconf.h"

/* Include common module headers */
#include "utils.h"

/* Version information */
#define PROJECT_VERSION_MAJOR 0
#define PROJECT_VERSION_MINOR 1
#define PROJECT_VERSION_PATCH 0
#define PROJECT_VERSION_STRING "0.1.0"

/* Default configuration values */
#ifndef CONFIG_MAX_TOKEN_LEN
#define CONFIG_MAX_TOKEN_LEN 32
#endif

#ifndef CONFIG_MAX_TOKENS
#define CONFIG_MAX_TOKENS 1000
#endif

#ifndef CONFIG_GENERATE_SAMPLES
#define CONFIG_GENERATE_SAMPLES 5
#endif

/* Terminal colors for error reporting */
#define COLOR_RESET "\033[0m"
#define COLOR_ERROR "\033[1;31m"     /* Bold Red */
#define COLOR_WARNING "\033[1;33m"   /* Bold Yellow */
#define COLOR_HIGHLIGHT "\033[1;36m" /* Bold Cyan */
#define COLOR_NOTE "\033[1;32m"      /* Bold Green */
#define COLOR_LOCATION "\033[1;34m"  /* Bold Blue */
#define COLOR_CODE "\033[0;36m"      /* Cyan */
#define COLOR_POINTER "\033[1;31m"   /* Bold Red (for error pointer) */
#define COLOR_UNDERLINE "\033[4m"    /* Underline */

/* Error reporting macros */
#define PRINT_ERROR(line, col, fmt, ...)                                       \
  do {                                                                         \
    fprintf(stderr, COLOR_ERROR "error" COLOR_RESET ": " fmt "\n",             \
            ##__VA_ARGS__);                                                    \
    fprintf(stderr,                                                            \
            "  " COLOR_LOCATION "--> " COLOR_RESET "[line:%d col:%d]\n", line, \
            col);                                                              \
  } while (0)

/* Print Error Message */
#define PRINT_ERROR_HIGHLIGHT(line, col, source_line, col_pos, error_len, fmt, \
                              ...)                                             \
  do {                                                                         \
    fprintf(stderr, COLOR_ERROR "error" COLOR_RESET ": " fmt "\n",             \
            ##__VA_ARGS__);                                                    \
    fprintf(stderr,                                                            \
            "  " COLOR_LOCATION "--> " COLOR_RESET "[line:%d col:%d]\n", line, \
            col);                                                              \
    fprintf(stderr, "   " COLOR_LOCATION "|\n" COLOR_RESET);                   \
    fprintf(stderr, COLOR_LOCATION "%4d" COLOR_RESET " | %s\n", line,          \
            source_line);                                                      \
    fprintf(stderr, "   " COLOR_LOCATION "|" COLOR_RESET " ");                 \
    int pos = (col_pos > 0) ? (col_pos + 1) : 0;                               \
    int len = (error_len > 0) ? error_len : 1;                                 \
    if (len > 20)                                                              \
      len = 20;                                                                \
    for (int i = 0; i < pos; i++)                                              \
      fprintf(stderr, " ");                                                    \
    for (int i = 0; i < len; i++)                                              \
      fprintf(stderr, COLOR_POINTER "^" COLOR_RESET);                          \
    fprintf(stderr, " " COLOR_ERROR "here\n" COLOR_RESET);                     \
    fprintf(stderr, "   " COLOR_LOCATION "|\n" COLOR_RESET);                   \
  } while (0)

#define PRINT_ERROR_HELP(help_message)                                         \
  do {                                                                         \
    fprintf(stderr,                                                            \
            "   " COLOR_LOCATION "= " COLOR_NOTE "help" COLOR_RESET ": %s\n",  \
            help_message);                                                     \
  } while (0)

/* Compiler stage identifiers, for future expansion */
#define STAGE_LEXER 0x01    /* Lexical analysis stage */
#define STAGE_PARSER 0x02   /* Syntax analysis stage */
#define STAGE_SEMANTIC 0x04 /* Semantic analysis stage */
#define STAGE_CODEGEN 0x08  /* Code generation stage */
#define STAGE_OPTIMIZE 0x10 /* Optimization stage */

/* Currently supported stages, update as project progresses */
#define SUPPORTED_STAGES (STAGE_LEXER)

/* Compiler status codes */
typedef enum {
  COMP_OK = 0,         /* Success */
  COMP_ERROR_IO,       /* IO error */
  COMP_ERROR_MEMORY,   /* Memory error */
  COMP_ERROR_LEXER,    /* Lexical analysis error */
  COMP_ERROR_PARSER,   /* Syntax analysis error */
  COMP_ERROR_SEMANTIC, /* Semantic analysis error */
  COMP_ERROR_CODEGEN,  /* Code generation error */
  COMP_ERROR_INTERNAL  /* Internal error */
} CompilerStatus;

#endif /* COMMON_H */
