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

# Default goal
all: build

# Project directories and files
BUILD_DIR        := build
OBJ_DIR          := $(BUILD_DIR)$(SEP)obj
UNITTEST_SRCS    := src$(SEP)unittest$(SEP)unittest.c
UTILS_SRCS       := src$(SEP)utils$(SEP)utils.c
LEXER_SRCS       := src$(SEP)lexer_analyzer$(SEP)token.c \
                    src$(SEP)lexer_analyzer$(SEP)lexer.c
MAIN_LEXER_SRC   := src$(SEP)lexer_main.c

UNITTEST_OBJS    := $(patsubst src$(SEP)%.c,$(OBJ_DIR)$(SEP)%.o,$(UNITTEST_SRCS))
UTILS_OBJS       := $(patsubst src$(SEP)%.c,$(OBJ_DIR)$(SEP)%.o,$(UTILS_SRCS))
LEXER_OBJS       := $(patsubst src$(SEP)%.c,$(OBJ_DIR)$(SEP)%.o,$(LEXER_SRCS))
MAIN_LEXER_OBJ   := $(patsubst src$(SEP)%.c,$(OBJ_DIR)$(SEP)%.o,$(MAIN_LEXER_SRC))

COMMON_LIB       := $(BUILD_DIR)$(SEP)libcommon.a
LEXER_LIB        := $(BUILD_DIR)$(SEP)liblexer.a

MAIN_LEXER_EXEC  := $(BUILD_DIR)$(SEP)lexer$(if $(filter Windows_NT,$(OS)),.exe,)

CC               := gcc
CFLAGS           := -Wall -Wextra -O2
LDFLAGS          :=

build: $(COMMON_LIB) $(LEXER_LIB) $(MAIN_LEXER_EXEC)

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

# Build main executable
$(MAIN_LEXER_EXEC): $(MAIN_LEXER_OBJ) $(COMMON_LIB) $(LEXER_LIB)
	@echo "Linking main executable..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(LDFLAGS) -o $@ $< -L$(BUILD_DIR) -llexer -lcommon

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
