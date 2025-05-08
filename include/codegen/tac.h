/**
 * @file tac.h
 * @brief Three-address code representation
 */

#ifndef TAC_H
#define TAC_H

#include <stdbool.h>

/**
 * @brief Three-address code operation types
 */
typedef enum {
  TAC_OP_ASSIGN, /* x := y (simple assignment) */
  TAC_OP_ADD,    /* x := y + z */
  TAC_OP_SUB,    /* x := y - z */
  TAC_OP_MUL,    /* x := y * z */
  TAC_OP_DIV,    /* x := y / z */
  TAC_OP_EQ,     /* if y = z goto L */
  TAC_OP_NE,     /* if y != z goto L */
  TAC_OP_LT,     /* if y < z goto L */
  TAC_OP_LE,     /* if y <= z goto L */
  TAC_OP_GT,     /* if y > z goto L */
  TAC_OP_GE,     /* if y >= z goto L */
  TAC_OP_GOTO,   /* goto L */
  TAC_OP_LABEL,  /* L: */
  TAC_OP_PARAM,  /* param x */
  TAC_OP_CALL,   /* call p, n */
  TAC_OP_RETURN  /* return x */
} TACOpType;

/**
 * @brief Three-address code instruction
 */
typedef struct TACInst {
  TACOpType op; /* Operation type */
  char *result; /* Result operand */
  char *arg1;   /* First argument */
  char *arg2;   /* Second argument */
  int lineno;   /* Line number for error reporting */
} TACInst;

/**
 * @brief Three-address code program
 */
typedef struct TACProgram {
  TACInst **instructions; /* Array of instructions */
  int count;              /* Number of instructions */
  int capacity;           /* Capacity of instructions array */
} TACProgram;

/**
 * @brief Create a new TAC program
 *
 * @return TACProgram* Created program, or NULL on failure
 */
TACProgram *tac_program_create(void);

/**
 * @brief Add an instruction to a TAC program
 *
 * @param program TAC program
 * @param op Operation type
 * @param result Result operand (can be NULL)
 * @param arg1 First argument (can be NULL)
 * @param arg2 Second argument (can be NULL)
 * @param lineno Line number
 * @return int Index of added instruction, or -1 on failure
 */
int tac_program_add_inst(TACProgram *program, TACOpType op, const char *result,
                         const char *arg1, const char *arg2, int lineno);

/**
 * @brief Get an instruction from a TAC program
 *
 * @param program TAC program
 * @param index Instruction index
 * @return TACInst* Instruction at index, or NULL if index is out of bounds
 */
TACInst *tac_program_get_inst(TACProgram *program, int index);

/**
 * @brief Print a TAC program to stdout
 *
 * @param program TAC program to print
 */
void tac_program_print(TACProgram *program);

/**
 * @brief Write a TAC program to a file
 *
 * @param program TAC program
 * @param filename Output filename
 * @return bool Success status
 */
bool tac_program_write_to_file(TACProgram *program, const char *filename);

/**
 * @brief Free TAC program resources
 *
 * @param program TAC program to destroy
 */
void tac_program_destroy(TACProgram *program);

/**
 * @brief Create a string representation of a TAC operation type
 *
 * @param op Operation type
 * @return const char* String representation
 */
const char *tac_op_type_to_string(TACOpType op);

#endif /* TAC_H */
