# Default goal
all: build

# Project directories and files
BUILD_DIR         := build
OBJ_DIR           := $(BUILD_DIR)/obj
LEXER_OBJ_DIR     := $(BUILD_DIR)/obj_lexer
PARSER_OBJ_DIR    := $(BUILD_DIR)/obj_parser
CODEGEN_OBJ_DIR   := $(BUILD_DIR)/obj_codegen

# Source files
UTILS_SRCS        := src/utils/utils.c
LEXER_SRCS        := src/lexer/token.c \
                     src/lexer/lexer.c \
                     src/lexer/lexer_state_machine.c
LEXER_MAIN_SRC    := src/lexer_main.c
PARSER_SRCS       := src/parser/grammar.c \
                     src/parser/parser.c \
                     src/parser/parser_common.c \
                     src/parser/syntax_tree.c \
                     src/parser/rd/rd_parser.c \
                     src/parser/lr/action_table.c \
                     src/parser/lr/automaton.c \
                     src/parser/lr/item.c \
                     src/parser/lr/lr_common.c \
                     src/parser/lr/lr_parser.c \
                     src/parser/lr/lr0/lr0_parser.c \
                     src/parser/lr/lr1/lr1_parser.c \
                     src/parser/lr/slr1/slr1_parser.c
PARSER_MAIN_SRC   := src/parser_main.c
CODEGEN_SRCS      := src/codegen/sdt_codegen.c \
                     src/codegen/tac.c \
                     src/codegen/sdt/label_manager/label_manager.c \
                     src/codegen/sdt/symbol_table/symbol_table.c \
                     src/codegen/sdt/sdt_actions.c \
                     src/codegen/sdt/sdt_attributes.c
CODEGEN_MAIN_SRC  := src/codegen_main.c

# Object files for common components (with different compiler flags for each target)
LEXER_COMMON_OBJS  := $(patsubst src/%.c,$(LEXER_OBJ_DIR)/%.o,$(UTILS_SRCS))
PARSER_COMMON_OBJS := $(patsubst src/%.c,$(PARSER_OBJ_DIR)/%.o,$(UTILS_SRCS))
CODEGEN_COMMON_OBJS := $(patsubst src/%.c,$(CODEGEN_OBJ_DIR)/%.o,$(UTILS_SRCS))

# Object files for lexer (built differently for each target)
LEXER_LEXER_OBJS  := $(patsubst src/%.c,$(LEXER_OBJ_DIR)/%.o,$(LEXER_SRCS))
PARSER_LEXER_OBJS := $(patsubst src/%.c,$(PARSER_OBJ_DIR)/%.o,$(LEXER_SRCS))
CODEGEN_LEXER_OBJS := $(patsubst src/%.c,$(CODEGEN_OBJ_DIR)/%.o,$(LEXER_SRCS))

# Object files for parser (built with -DCONFIG_TAC=1 for codegen only)
PARSER_PARSER_OBJS := $(patsubst src/%.c,$(PARSER_OBJ_DIR)/%.o,$(PARSER_SRCS))
CODEGEN_PARSER_OBJS := $(patsubst src/%.c,$(CODEGEN_OBJ_DIR)/%.o,$(PARSER_SRCS))

# Object files for codegen
CODEGEN_CODEGEN_OBJS := $(patsubst src/%.c,$(CODEGEN_OBJ_DIR)/%.o,$(CODEGEN_SRCS))

# Main objects
LEXER_MAIN_OBJ   := $(patsubst src/%.c,$(LEXER_OBJ_DIR)/%.o,$(LEXER_MAIN_SRC))
PARSER_MAIN_OBJ  := $(patsubst src/%.c,$(PARSER_OBJ_DIR)/%.o,$(PARSER_MAIN_SRC))
CODEGEN_MAIN_OBJ := $(patsubst src/%.c,$(CODEGEN_OBJ_DIR)/%.o,$(CODEGEN_MAIN_SRC))

# Executables
LEXER_EXEC   := $(BUILD_DIR)/lexer
PARSER_EXEC  := $(BUILD_DIR)/parser
CODEGEN_EXEC := $(BUILD_DIR)/codegen

# Compiler flags
CC             := gcc
COMMON_CFLAGS  := -Wall -Wextra -O2
LEXER_CFLAGS   := $(COMMON_CFLAGS)
PARSER_CFLAGS  := $(COMMON_CFLAGS)
CODEGEN_CFLAGS := $(COMMON_CFLAGS) -DCONFIG_TAC=1
LDFLAGS        :=

# Auto generate include file
AUTOGEN_INCLUDE := include/generated/autoconf.h

# Warn if no .config exists
ifeq ($(wildcard .config),)
  $(warning Warning: .config does not exist!)
  $(warning Please run 'make menuconfig' first.)
endif

# Directory creation and removal operations
MKDIR = mkdir -p $1
RM    = rm -rf

# Define separate build targets
.PHONY: all build build_lexer build_parser build_codegen clean

# Main build targets
build: build_lexer build_parser build_codegen
build_lexer: $(LEXER_EXEC)
build_parser: $(PARSER_EXEC)
build_codegen: $(CODEGEN_EXEC)

# Build lexer executable
$(LEXER_EXEC): $(LEXER_MAIN_OBJ) $(LEXER_LEXER_OBJS) $(LEXER_COMMON_OBJS)
	@echo "Linking lexer executable..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(LDFLAGS) -o $@ $^

# Build parser executable
$(PARSER_EXEC): $(PARSER_MAIN_OBJ) $(PARSER_PARSER_OBJS) $(PARSER_LEXER_OBJS) $(PARSER_COMMON_OBJS)
	@echo "Linking parser executable..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(LDFLAGS) -o $@ $^

# Build codegen executable
$(CODEGEN_EXEC): $(CODEGEN_MAIN_OBJ) $(CODEGEN_CODEGEN_OBJS) $(CODEGEN_PARSER_OBJS) $(CODEGEN_LEXER_OBJS) $(CODEGEN_COMMON_OBJS)
	@echo "Linking codegen executable..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(LDFLAGS) -o $@ $^

# Compile lexer source files
$(LEXER_OBJ_DIR)/%.o: src/%.c $(AUTOGEN_INCLUDE)
	@echo "Compiling $< for lexer..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(LEXER_CFLAGS) -Iinclude -c $< -o $@

# Compile parser source files
$(PARSER_OBJ_DIR)/%.o: src/%.c $(AUTOGEN_INCLUDE)
	@echo "Compiling $< for parser..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(PARSER_CFLAGS) -Iinclude -c $< -o $@

# Compile codegen source files
$(CODEGEN_OBJ_DIR)/%.o: src/%.c $(AUTOGEN_INCLUDE)
	@echo "Compiling $< for codegen..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(CODEGEN_CFLAGS) -Iinclude -c $< -o $@

# Clean all build artifacts
clean:
	@echo "Cleaning project artifacts..."
	$(RM) $(BUILD_DIR)

SCRIPTS_DIR   := scripts
include $(SCRIPTS_DIR)/kconfig.mk
