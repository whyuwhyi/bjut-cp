# Detect platform
ifeq ($(OS),Windows_NT)
  MKDIR = if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
  RM    = del /Q
  SEP   = \\
else
  MKDIR = mkdir -p $1
  RM    = rm -rf
  SEP   = /
endif

# Warn if no .config exists
ifeq ($(wildcard .config),)
  $(warning Warning: .config does not exist!)
  $(warning Please run 'make menuconfig' first.)
endif

# Default goal
all: build

# Project directories and files
BUILD_DIR         := build
OBJ_DIR           := $(BUILD_DIR)$(SEP)obj
LEXER_OBJ_DIR     := $(BUILD_DIR)$(SEP)obj_lexer
PARSER_OBJ_DIR    := $(BUILD_DIR)$(SEP)obj_parser
CODEGEN_OBJ_DIR   := $(BUILD_DIR)$(SEP)obj_codegen

# Source files
UTILS_SRCS        := src$(SEP)utils$(SEP)utils.c
LEXER_SRCS        := src$(SEP)lexer_analyzer$(SEP)token.c \
                     src$(SEP)lexer_analyzer$(SEP)lexer.c \
                     src$(SEP)lexer_analyzer$(SEP)lexer_state_machine.c
LEXER_MAIN_SRC    := src$(SEP)lexer_main.c

PARSER_SRCS       := src$(SEP)parser$(SEP)grammar.c \
                     src$(SEP)parser$(SEP)parser.c \
                     src$(SEP)parser$(SEP)parser_common.c \
                     src$(SEP)parser$(SEP)syntax_tree.c \
                     src$(SEP)parser$(SEP)recursive_descent$(SEP)rd_parser.c \
                     src$(SEP)parser$(SEP)lr$(SEP)action_table.c \
                     src$(SEP)parser$(SEP)lr$(SEP)automaton.c \
                     src$(SEP)parser$(SEP)lr$(SEP)item.c \
                     src$(SEP)parser$(SEP)lr$(SEP)lr_common.c \
                     src$(SEP)parser$(SEP)lr$(SEP)lr_parser.c \
                     src$(SEP)parser$(SEP)lr$(SEP)lr0$(SEP)lr0_parser.c \
                     src$(SEP)parser$(SEP)lr$(SEP)lr1$(SEP)lr1_parser.c \
                     src$(SEP)parser$(SEP)lr$(SEP)slr1$(SEP)slr1_parser.c
PARSER_MAIN_SRC   := src$(SEP)parser_main.c

CODEGEN_SRCS      := src$(SEP)codegen$(SEP)sdt_codegen.c \
                     src$(SEP)codegen$(SEP)tac.c \
                     src$(SEP)codegen$(SEP)label_manager$(SEP)label_manager.c \
                     src$(SEP)codegen$(SEP)symbol_table$(SEP)symbol_table.c \
                     src$(SEP)codegen$(SEP)sdt$(SEP)sdt_actions.c \
                     src$(SEP)codegen$(SEP)sdt$(SEP)sdt_attributes.c
CODEGEN_MAIN_SRC  := src$(SEP)codegen_main.c

# Object files for common components (with different compiler flags for each target)
LEXER_COMMON_OBJS  := $(patsubst src$(SEP)%.c,$(LEXER_OBJ_DIR)$(SEP)%.o,$(UTILS_SRCS))
PARSER_COMMON_OBJS := $(patsubst src$(SEP)%.c,$(PARSER_OBJ_DIR)$(SEP)%.o,$(UTILS_SRCS))
CODEGEN_COMMON_OBJS := $(patsubst src$(SEP)%.c,$(CODEGEN_OBJ_DIR)$(SEP)%.o,$(UTILS_SRCS))

# Object files for lexer (built differently for each target)
LEXER_LEXER_OBJS  := $(patsubst src$(SEP)%.c,$(LEXER_OBJ_DIR)$(SEP)%.o,$(LEXER_SRCS))
PARSER_LEXER_OBJS := $(patsubst src$(SEP)%.c,$(PARSER_OBJ_DIR)$(SEP)%.o,$(LEXER_SRCS))
CODEGEN_LEXER_OBJS := $(patsubst src$(SEP)%.c,$(CODEGEN_OBJ_DIR)$(SEP)%.o,$(LEXER_SRCS))

# Object files for parser (built with -DCONFIG_TAC=1 for codegen only)
PARSER_PARSER_OBJS := $(patsubst src$(SEP)%.c,$(PARSER_OBJ_DIR)$(SEP)%.o,$(PARSER_SRCS))
CODEGEN_PARSER_OBJS := $(patsubst src$(SEP)%.c,$(CODEGEN_OBJ_DIR)$(SEP)%.o,$(PARSER_SRCS))

# Object files for codegen
CODEGEN_CODEGEN_OBJS := $(patsubst src$(SEP)%.c,$(CODEGEN_OBJ_DIR)$(SEP)%.o,$(CODEGEN_SRCS))

# Main objects
LEXER_MAIN_OBJ   := $(patsubst src$(SEP)%.c,$(LEXER_OBJ_DIR)$(SEP)%.o,$(LEXER_MAIN_SRC))
PARSER_MAIN_OBJ  := $(patsubst src$(SEP)%.c,$(PARSER_OBJ_DIR)$(SEP)%.o,$(PARSER_MAIN_SRC))
CODEGEN_MAIN_OBJ := $(patsubst src$(SEP)%.c,$(CODEGEN_OBJ_DIR)$(SEP)%.o,$(CODEGEN_MAIN_SRC))

# Executables
LEXER_EXEC   := $(BUILD_DIR)$(SEP)lexer$(if $(filter Windows_NT,$(OS)),.exe,)
PARSER_EXEC  := $(BUILD_DIR)$(SEP)parser$(if $(filter Windows_NT,$(OS)),.exe,)
CODEGEN_EXEC := $(BUILD_DIR)$(SEP)codegen$(if $(filter Windows_NT,$(OS)),.exe,)

# Compiler flags
CC             := gcc
COMMON_CFLAGS  := -Wall -Wextra -O2
LEXER_CFLAGS   := $(COMMON_CFLAGS)
PARSER_CFLAGS  := $(COMMON_CFLAGS)
CODEGEN_CFLAGS := $(COMMON_CFLAGS) -DCONFIG_TAC=1
LDFLAGS        :=

# Auto generate include file
AUTOGEN_INCLUDE := include$(SEP)generated$(SEP)autoconf.h

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
$(LEXER_OBJ_DIR)$(SEP)%.o: src$(SEP)%.c $(AUTOGEN_INCLUDE)
	@echo "Compiling $< for lexer..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(LEXER_CFLAGS) -Iinclude -c $< -o $@

# Compile parser source files
$(PARSER_OBJ_DIR)$(SEP)%.o: src$(SEP)%.c $(AUTOGEN_INCLUDE)
	@echo "Compiling $< for parser..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(PARSER_CFLAGS) -Iinclude -c $< -o $@

# Compile codegen source files
$(CODEGEN_OBJ_DIR)$(SEP)%.o: src$(SEP)%.c $(AUTOGEN_INCLUDE)
	@echo "Compiling $< for codegen..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(CODEGEN_CFLAGS) -Iinclude -c $< -o $@

# Clean all build artifacts
clean:
	@echo "Cleaning project artifacts..."
	$(call RM) $(BUILD_DIR)

SCRIPTS_DIR   := scripts
include $(SCRIPTS_DIR)/kconfig.mk
