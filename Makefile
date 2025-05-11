# Default goal
all: build

# Project directories and files
BUILD_DIR         := build
OBJ_DIR           := $(BUILD_DIR)/obj
SRC_DIR           := src
INCLUDE_DIR       := include

# Find all source files automatically
COMMON_SRCS       := $(shell find $(SRC_DIR)/utils $(SRC_DIR)/error_handler -name '*.c')
LEXER_SRCS        := $(shell find $(SRC_DIR)/lexer -name '*.c')
PARSER_SRCS       := $(shell find $(SRC_DIR)/parser -name '*.c')
CODEGEN_SRCS      := $(shell find $(SRC_DIR)/codegen -name '*.c')

# Main source files
LEXER_MAIN_SRC    := $(SRC_DIR)/lexer_main.c
PARSER_MAIN_SRC   := $(SRC_DIR)/parser_main.c
CODEGEN_MAIN_SRC  := $(SRC_DIR)/codegen_main.c

# Find all header files for dependency tracking
COMMON_HEADERS    := $(shell find $(INCLUDE_DIR)/utils $(INCLUDE_DIR)/error_handler -name '*.h' 2>/dev/null)
LEXER_HEADERS     := $(shell find $(INCLUDE_DIR)/lexer -name '*.h')
PARSER_HEADERS    := $(shell find $(INCLUDE_DIR)/parser -name '*.h')
CODEGEN_HEADERS   := $(shell find $(INCLUDE_DIR)/codegen -name '*.h')
CONFIG_HEADERS    := $(shell find $(INCLUDE_DIR)/config $(INCLUDE_DIR)/generated -name '*.h')
ALL_HEADERS       := $(shell find $(INCLUDE_DIR) -name '*.h')

# Define object directories for each component
COMMON_OBJ_DIR    := $(OBJ_DIR)/common
LEXER_OBJ_DIR     := $(OBJ_DIR)/lexer
PARSER_OBJ_DIR    := $(OBJ_DIR)/parser
CODEGEN_OBJ_DIR   := $(OBJ_DIR)/codegen
PARSER_TAC_OBJ_DIR := $(OBJ_DIR)/parser_tac

# Library directory
LIB_DIR           := $(BUILD_DIR)/lib

# Map source files to object files
COMMON_OBJS       := $(patsubst $(SRC_DIR)/%.c,$(COMMON_OBJ_DIR)/%.o,$(COMMON_SRCS))
LEXER_OBJS        := $(patsubst $(SRC_DIR)/%.c,$(LEXER_OBJ_DIR)/%.o,$(LEXER_SRCS))
PARSER_OBJS       := $(patsubst $(SRC_DIR)/%.c,$(PARSER_OBJ_DIR)/%.o,$(PARSER_SRCS))
CODEGEN_OBJS      := $(patsubst $(SRC_DIR)/%.c,$(CODEGEN_OBJ_DIR)/%.o,$(CODEGEN_SRCS))
PARSER_TAC_OBJS   := $(patsubst $(SRC_DIR)/%.c,$(PARSER_TAC_OBJ_DIR)/%.o,$(PARSER_SRCS))

# Main executables object files
LEXER_MAIN_OBJ    := $(patsubst $(SRC_DIR)/%.c,$(LEXER_OBJ_DIR)/%.o,$(LEXER_MAIN_SRC))
PARSER_MAIN_OBJ   := $(patsubst $(SRC_DIR)/%.c,$(PARSER_OBJ_DIR)/%.o,$(PARSER_MAIN_SRC))
CODEGEN_MAIN_OBJ  := $(patsubst $(SRC_DIR)/%.c,$(CODEGEN_OBJ_DIR)/%.o,$(CODEGEN_MAIN_SRC))

# Static libraries
COMMON_LIB        := $(LIB_DIR)/libcommon.a
LEXER_LIB         := $(LIB_DIR)/liblexer.a
PARSER_LIB        := $(LIB_DIR)/libparser.a
PARSER_TAC_LIB    := $(LIB_DIR)/libparser_tac.a

# Executables
LEXER_EXEC        := $(BUILD_DIR)/lexer
PARSER_EXEC       := $(BUILD_DIR)/parser
CODEGEN_EXEC      := $(BUILD_DIR)/codegen

# Compiler flags
CC               := gcc
COMMON_CFLAGS    := -Wall -Wextra -O2 -I$(INCLUDE_DIR)
LEXER_CFLAGS     := $(COMMON_CFLAGS)
PARSER_CFLAGS    := $(COMMON_CFLAGS)
PARSER_TAC_CFLAGS := $(COMMON_CFLAGS) -DCONFIG_TAC=1
CODEGEN_CFLAGS   := $(COMMON_CFLAGS) -DCONFIG_TAC=1

# Linker flags
LDFLAGS          :=

# Archive utility and flags
AR               := ar
ARFLAGS          := rcs

# Auto generate include file
AUTOGEN_INCLUDE := $(INCLUDE_DIR)/generated/autoconf.h

# Check if .config exists
ifeq ($(wildcard .config),)
  $(warning Warning: .config does not exist!)
  $(warning Please run 'make menuconfig' first.)
endif

# Directory creation and removal operations
MKDIR = mkdir -p $1
RM    = rm -rf

# Define build targets
.PHONY: all build build_lexer build_parser build_codegen clean

# Main build targets
build: build_lexer build_parser build_codegen

build_lexer: $(LEXER_EXEC)
build_parser: $(PARSER_EXEC)
build_codegen: $(CODEGEN_EXEC)

# Build common library
$(COMMON_LIB): $(COMMON_OBJS)
	@echo "Creating common library..."
	@$(call MKDIR,$(dir $@))
	$(AR) $(ARFLAGS) $@ $^

# Build lexer library
$(LEXER_LIB): $(LEXER_OBJS)
	@echo "Creating lexer library..."
	@$(call MKDIR,$(dir $@))
	$(AR) $(ARFLAGS) $@ $^

# Build parser library
$(PARSER_LIB): $(PARSER_OBJS)
	@echo "Creating parser library..."
	@$(call MKDIR,$(dir $@))
	$(AR) $(ARFLAGS) $@ $^

# Build parser library with CONFIG_TAC
$(PARSER_TAC_LIB): $(PARSER_TAC_OBJS)
	@echo "Creating parser library with CONFIG_TAC..."
	@$(call MKDIR,$(dir $@))
	$(AR) $(ARFLAGS) $@ $^

# Build lexer executable
$(LEXER_EXEC): $(LEXER_MAIN_OBJ) $(LEXER_LIB) $(COMMON_LIB)
	@echo "Linking lexer executable..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(LDFLAGS) -o $@ $(LEXER_MAIN_OBJ) $(LEXER_LIB) $(COMMON_LIB)

# Build parser executable
$(PARSER_EXEC): $(PARSER_MAIN_OBJ) $(PARSER_LIB) $(LEXER_LIB) $(COMMON_LIB)
	@echo "Linking parser executable..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(LDFLAGS) -o $@ $(PARSER_MAIN_OBJ) $(PARSER_LIB) $(LEXER_LIB) $(COMMON_LIB)

# Build codegen executable (using parser with CONFIG_TAC)
$(CODEGEN_EXEC): $(CODEGEN_MAIN_OBJ) $(CODEGEN_OBJS) $(PARSER_TAC_LIB) $(LEXER_LIB) $(COMMON_LIB)
	@echo "Linking codegen executable..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(LDFLAGS) -o $@ $(CODEGEN_MAIN_OBJ) $(CODEGEN_OBJS) $(PARSER_TAC_LIB) $(LEXER_LIB) $(COMMON_LIB)

# Rules for compiling source files with proper header dependencies

# Compile common library source files
$(COMMON_OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(COMMON_HEADERS) $(AUTOGEN_INCLUDE)
	@echo "Compiling $< for common library..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(COMMON_CFLAGS) -c $< -o $@

# Compile lexer source files
$(LEXER_OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(LEXER_HEADERS) $(COMMON_HEADERS) $(AUTOGEN_INCLUDE)
	@echo "Compiling $< for lexer..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(LEXER_CFLAGS) -c $< -o $@

# Compile parser source files
$(PARSER_OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(PARSER_HEADERS) $(LEXER_HEADERS) $(COMMON_HEADERS) $(AUTOGEN_INCLUDE)
	@echo "Compiling $< for parser..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(PARSER_CFLAGS) -c $< -o $@

# Compile parser source files with CONFIG_TAC
$(PARSER_TAC_OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(PARSER_HEADERS) $(LEXER_HEADERS) $(COMMON_HEADERS) $(AUTOGEN_INCLUDE)
	@echo "Compiling $< for parser with CONFIG_TAC..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(PARSER_TAC_CFLAGS) -c $< -o $@

# Compile codegen source files
$(CODEGEN_OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(CODEGEN_HEADERS) $(PARSER_HEADERS) $(LEXER_HEADERS) $(COMMON_HEADERS) $(AUTOGEN_INCLUDE)
	@echo "Compiling $< for codegen..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(CODEGEN_CFLAGS) -c $< -o $@

# Clean all build artifacts
clean:
	@echo "Cleaning project artifacts..."
	$(RM) $(BUILD_DIR)

# Include Kconfig system
SCRIPTS_DIR := scripts
include $(SCRIPTS_DIR)/kconfig.mk
