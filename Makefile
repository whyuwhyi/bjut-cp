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
BUILD_DIR        := build
OBJ_DIR          := $(BUILD_DIR)$(SEP)obj

# Source files
UNITTEST_SRCS    := src$(SEP)unittest$(SEP)unittest.c
UTILS_SRCS       := src$(SEP)utils$(SEP)utils.c
LEXER_SRCS       := src$(SEP)lexer_analyzer$(SEP)token.c \
                    src$(SEP)lexer_analyzer$(SEP)lexer.c \
                    src$(SEP)lexer_analyzer$(SEP)lexer_state_machine.c
LEXER_MAIN_SRC   := src$(SEP)lexer_main.c

# Parser source files
PARSER_SRCS      := src$(SEP)parser$(SEP)grammar.c \
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


PARSER_MAIN_SRC  := src$(SEP)parser_main.c

# Object files
UNITTEST_OBJS    := $(patsubst src$(SEP)%.c,$(OBJ_DIR)$(SEP)%.o,$(UNITTEST_SRCS))
UTILS_OBJS       := $(patsubst src$(SEP)%.c,$(OBJ_DIR)$(SEP)%.o,$(UTILS_SRCS))
LEXER_OBJS       := $(patsubst src$(SEP)%.c,$(OBJ_DIR)$(SEP)%.o,$(LEXER_SRCS))
LEXER_MAIN_OBJ   := $(patsubst src$(SEP)%.c,$(OBJ_DIR)$(SEP)%.o,$(LEXER_MAIN_SRC))
PARSER_OBJS      := $(patsubst src$(SEP)%.c,$(OBJ_DIR)$(SEP)%.o,$(PARSER_SRCS))
PARSER_MAIN_OBJ  := $(patsubst src$(SEP)%.c,$(OBJ_DIR)$(SEP)%.o,$(PARSER_MAIN_SRC))

# Libraries and executables
COMMON_LIB       := $(BUILD_DIR)$(SEP)libcommon.a
LEXER_LIB        := $(BUILD_DIR)$(SEP)liblexer.a
PARSER_LIB       := $(BUILD_DIR)$(SEP)libparser.a
LEXER_EXEC       := $(BUILD_DIR)$(SEP)lexer$(if $(filter Windows_NT,$(OS)),.exe,)
PARSER_EXEC      := $(BUILD_DIR)$(SEP)parser$(if $(filter Windows_NT,$(OS)),.exe,)

# Compiler flags
CC               := gcc
CFLAGS           := -Wall -Wextra -O2
LDFLAGS          :=

# Main build target
build: $(COMMON_LIB) $(LEXER_LIB) $(LEXER_EXEC) $(PARSER_LIB) $(PARSER_EXEC)

# Build common library
$(COMMON_LIB): $(UNITTEST_OBJS) $(UTILS_OBJS)
	@echo "Archiving common library..."
	@$(call MKDIR,$(dir $@))
	ar rcs $@ $^

# Build lexer library
$(LEXER_LIB): $(LEXER_OBJS)
	@echo "Archiving lexer library..."
	@$(call MKDIR,$(dir $@))
	ar rcs $@ $^

# Build parser library
$(PARSER_LIB): $(PARSER_OBJS)
	@echo "Archiving parser library..."
	@$(call MKDIR,$(dir $@))
	ar rcs $@ $^

# Build lexer executable
$(LEXER_EXEC): $(LEXER_MAIN_OBJ) $(COMMON_LIB) $(LEXER_LIB)
	@echo "Linking lexer executable..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(LDFLAGS) -o $@ $< -L$(BUILD_DIR) -llexer -lcommon

# Build parser executable
$(PARSER_EXEC): $(PARSER_MAIN_OBJ) $(COMMON_LIB) $(LEXER_LIB) $(PARSER_LIB)
	@echo "Linking parser executable..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(LDFLAGS) -o $@ $< -L$(BUILD_DIR) -lparser -llexer -lcommon

# Compile source files
$(OBJ_DIR)$(SEP)%.o: src$(SEP)%.c
	@echo "Compiling $<..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

# Clean all build artifacts
clean:
	@echo "Cleaning project artifacts..."
	$(call RM) $(BUILD_DIR)

SCRIPTS_DIR   := scripts
include $(SCRIPTS_DIR)/kconfig.mk

.PHONY: all build clean
