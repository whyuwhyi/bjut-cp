/**
 * @file tac.c
 * @brief Three-address code representation implementation
 */

#include "codegen/tac.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 64

/**
 * @brief Create a new TAC program
 */
TACProgram *tac_program_create(void) {
  TACProgram *program = (TACProgram *)safe_malloc(sizeof(TACProgram));
  if (!program) {
    return NULL;
  }

  program->instructions =
      (TACInst **)safe_malloc(INITIAL_CAPACITY * sizeof(TACInst *));
  if (!program->instructions) {
    free(program);
    return NULL;
  }

  program->count = 0;
  program->capacity = INITIAL_CAPACITY;

  DEBUG_PRINT("Created TAC program");
  return program;
}

/**
 * @brief Add an instruction to a TAC program
 */
int tac_program_add_inst(TACProgram *program, TACOpType op, const char *result,
                         const char *arg1, const char *arg2, int lineno) {
  if (!program) {
    return -1;
  }

  /* Check if program needs to be resized */
  if (program->count >= program->capacity) {
    int new_capacity = program->capacity * 2;
    TACInst **new_instructions = (TACInst **)safe_realloc(
        program->instructions, new_capacity * sizeof(TACInst *));
    if (!new_instructions) {
      return -1;
    }

    program->instructions = new_instructions;
    program->capacity = new_capacity;
  }

  /* Create new instruction */
  TACInst *inst = (TACInst *)safe_malloc(sizeof(TACInst));
  if (!inst) {
    return -1;
  }

  inst->op = op;
  inst->lineno = lineno;

  /* Copy operands if provided */
  inst->result = result ? safe_strdup(result) : NULL;
  inst->arg1 = arg1 ? safe_strdup(arg1) : NULL;
  inst->arg2 = arg2 ? safe_strdup(arg2) : NULL;

  /* Add instruction to program */
  program->instructions[program->count] = inst;
  program->count++;

  DEBUG_PRINT("Added TAC instruction: %s %s %s %s", tac_op_type_to_string(op),
              result ? result : "", arg1 ? arg1 : "", arg2 ? arg2 : "");

  return program->count - 1;
}

/**
 * @brief Get an instruction from a TAC program
 */
TACInst *tac_program_get_inst(TACProgram *program, int index) {
  if (!program || index < 0 || index >= program->count) {
    return NULL;
  }

  return program->instructions[index];
}

/**
 * @brief Print a TAC program to stdout
 */
void tac_program_print(TACProgram *program) {
  if (!program) {
    return;
  }

  printf("Three-Address Code Program (%d instructions):\n", program->count);
  printf("--------------------------------------------\n");

  for (int i = 0; i < program->count; i++) {
    TACInst *inst = program->instructions[i];

    switch (inst->op) {
    case TAC_OP_ASSIGN:
      printf("%s := %s\n", inst->result, inst->arg1);
      break;

    case TAC_OP_ADD:
      printf("%s := %s + %s\n", inst->result, inst->arg1, inst->arg2);
      break;

    case TAC_OP_SUB:
      printf("%s := %s - %s\n", inst->result, inst->arg1, inst->arg2);
      break;

    case TAC_OP_MUL:
      printf("%s := %s * %s\n", inst->result, inst->arg1, inst->arg2);
      break;

    case TAC_OP_DIV:
      printf("%s := %s / %s\n", inst->result, inst->arg1, inst->arg2);
      break;

    case TAC_OP_EQ:
      printf("if %s = %s goto %s\n", inst->arg1, inst->arg2, inst->result);
      break;

    case TAC_OP_NE:
      printf("if %s != %s goto %s\n", inst->arg1, inst->arg2, inst->result);
      break;

    case TAC_OP_LT:
      printf("if %s < %s goto %s\n", inst->arg1, inst->arg2, inst->result);
      break;

    case TAC_OP_LE:
      printf("if %s <= %s goto %s\n", inst->arg1, inst->arg2, inst->result);
      break;

    case TAC_OP_GT:
      printf("if %s > %s goto %s\n", inst->arg1, inst->arg2, inst->result);
      break;

    case TAC_OP_GE:
      printf("if %s >= %s goto %s\n", inst->arg1, inst->arg2, inst->result);
      break;

    case TAC_OP_GOTO:
      printf("goto %s\n", inst->result);
      break;

    case TAC_OP_LABEL:
      printf("%s:\n", inst->result);
      break;

    case TAC_OP_PARAM:
      printf("param %s\n", inst->result);
      break;

    case TAC_OP_CALL:
      printf("call %s, %s\n", inst->result, inst->arg1);
      break;

    case TAC_OP_RETURN:
      printf("return %s\n", inst->result);
      break;

    default:
      printf("Unknown operation\n");
      break;
    }
  }

  printf("--------------------------------------------\n");
}

/**
 * @brief Write a TAC program to a file
 */
bool tac_program_write_to_file(TACProgram *program, const char *filename) {
  if (!program || !filename) {
    return false;
  }

  FILE *file = fopen(filename, "w");
  if (!file) {
    fprintf(stderr, "Error: Could not open file %s for writing\n", filename);
    return false;
  }

  for (int i = 0; i < program->count; i++) {
    TACInst *inst = program->instructions[i];

    switch (inst->op) {
    case TAC_OP_ASSIGN:
      fprintf(file, "%s := %s\n", inst->result, inst->arg1);
      break;

    case TAC_OP_ADD:
      fprintf(file, "%s := %s + %s\n", inst->result, inst->arg1, inst->arg2);
      break;

    case TAC_OP_SUB:
      fprintf(file, "%s := %s - %s\n", inst->result, inst->arg1, inst->arg2);
      break;

    case TAC_OP_MUL:
      fprintf(file, "%s := %s * %s\n", inst->result, inst->arg1, inst->arg2);
      break;

    case TAC_OP_DIV:
      fprintf(file, "%s := %s / %s\n", inst->result, inst->arg1, inst->arg2);
      break;

    case TAC_OP_EQ:
      fprintf(file, "if %s = %s goto %s\n", inst->arg1, inst->arg2,
              inst->result);
      break;

    case TAC_OP_NE:
      fprintf(file, "if %s != %s goto %s\n", inst->arg1, inst->arg2,
              inst->result);
      break;

    case TAC_OP_LT:
      fprintf(file, "if %s < %s goto %s\n", inst->arg1, inst->arg2,
              inst->result);
      break;

    case TAC_OP_LE:
      fprintf(file, "if %s <= %s goto %s\n", inst->arg1, inst->arg2,
              inst->result);
      break;

    case TAC_OP_GT:
      fprintf(file, "if %s > %s goto %s\n", inst->arg1, inst->arg2,
              inst->result);
      break;

    case TAC_OP_GE:
      fprintf(file, "if %s >= %s goto %s\n", inst->arg1, inst->arg2,
              inst->result);
      break;

    case TAC_OP_GOTO:
      fprintf(file, "goto %s\n", inst->result);
      break;

    case TAC_OP_LABEL:
      fprintf(file, "%s:\n", inst->result);
      break;

    case TAC_OP_PARAM:
      fprintf(file, "param %s\n", inst->result);
      break;

    case TAC_OP_CALL:
      fprintf(file, "call %s, %s\n", inst->result, inst->arg1);
      break;

    case TAC_OP_RETURN:
      fprintf(file, "return %s\n", inst->result);
      break;

    default:
      fprintf(file, "Unknown operation\n");
      break;
    }
  }

  fclose(file);
  return true;
}

/**
 * @brief Free TAC program resources
 */
void tac_program_destroy(TACProgram *program) {
  if (!program) {
    return;
  }

  /* Free all instructions */
  for (int i = 0; i < program->count; i++) {
    TACInst *inst = program->instructions[i];

    if (inst) {
      if (inst->result)
        free(inst->result);
      if (inst->arg1)
        free(inst->arg1);
      if (inst->arg2)
        free(inst->arg2);
      free(inst);
    }
  }

  /* Free instruction array and program */
  free(program->instructions);
  free(program);

  DEBUG_PRINT("Destroyed TAC program");
}

/**
 * @brief Create a string representation of a TAC operation type
 */
const char *tac_op_type_to_string(TACOpType op) {
  switch (op) {
  case TAC_OP_ASSIGN:
    return "ASSIGN";
  case TAC_OP_ADD:
    return "ADD";
  case TAC_OP_SUB:
    return "SUB";
  case TAC_OP_MUL:
    return "MUL";
  case TAC_OP_DIV:
    return "DIV";
  case TAC_OP_EQ:
    return "EQ";
  case TAC_OP_NE:
    return "NE";
  case TAC_OP_LT:
    return "LT";
  case TAC_OP_LE:
    return "LE";
  case TAC_OP_GT:
    return "GT";
  case TAC_OP_GE:
    return "GE";
  case TAC_OP_GOTO:
    return "GOTO";
  case TAC_OP_LABEL:
    return "LABEL";
  case TAC_OP_PARAM:
    return "PARAM";
  case TAC_OP_CALL:
    return "CALL";
  case TAC_OP_RETURN:
    return "RETURN";
  default:
    return "UNKNOWN";
  }
}
