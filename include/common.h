/**
 * @file common.h
 * @brief Common header file included by all source files in the project
 */

#ifndef COMMON_H
#define COMMON_H

/* Include auto-generated configuration */
#include "generated/autoconf.h"

/* Include common module headers */
// #include "error_handler.h"
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
