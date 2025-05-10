# Local build directories 
BUILD_DIR := build
OBJ_DIR   := $(BUILD_DIR)/obj

# Test sources and objects
TEST_SRCS := test_token.c test_lexer.c test_main.c
TEST_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(TEST_SRCS))

# Common and lexer sources needed for tests
COMMON_SRCS := ../../src/utils/utils.c
LEXER_SRCS  := ../../src/lexer/token.c \
               ../../src/lexer/lexer.c \
               ../../src/lexer/lexer_state_machine.c

# Object files for sources
COMMON_OBJS := $(patsubst ../../%.c,$(OBJ_DIR)/%.o,$(COMMON_SRCS))
LEXER_OBJS  := $(patsubst ../../%.c,$(OBJ_DIR)/%.o,$(LEXER_SRCS))

# Test executables
TEST_TOKEN_EXE := $(BUILD_DIR)/test_token
TEST_LEXER_EXE := $(BUILD_DIR)/test_lexer
TEST_MAIN_EXE  := $(BUILD_DIR)/test_main

# Sample files for test_main
SAMPLE_DIR := samples
SAMPLE_FILES := $(SAMPLE_DIR)/sample.txt

# Unittest files
UNITTEST_DIR := ../
UNITTEST_SRCS := $(UNITTEST_DIR)unittest.c
UNITTEST_OBJS := $(patsubst $(UNITTEST_DIR)%.c,$(OBJ_DIR)/%.o,$(UNITTEST_SRCS))

# Directory operations
MKDIR = mkdir -p $1
RM    = rm -rf

# Compiler settings
CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -I../../include
LDFLAGS :=

# -----------------------------------------------------------------------------
# Default: build test executables
# -----------------------------------------------------------------------------
all: $(TEST_TOKEN_EXE) $(TEST_LEXER_EXE) $(TEST_MAIN_EXE)

# -----------------------------------------------------------------------------
# Run all tests
# -----------------------------------------------------------------------------
test: all
	@echo "Running token test..."
	@$(TEST_TOKEN_EXE)
	@echo "Running lexer test..."
	@$(TEST_LEXER_EXE)
	@echo "Running main test..."
	@$(TEST_MAIN_EXE) $(SAMPLE_FILES)

# -----------------------------------------------------------------------------
# Link rules for each test executable
# -----------------------------------------------------------------------------
$(TEST_TOKEN_EXE): $(OBJ_DIR)/test_token.o $(UNITTEST_OBJS) $(COMMON_OBJS) $(LEXER_OBJS)
	@echo "Linking token test..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(LDFLAGS) -o $@ $^

$(TEST_LEXER_EXE): $(OBJ_DIR)/test_lexer.o $(UNITTEST_OBJS) $(COMMON_OBJS) $(LEXER_OBJS)
	@echo "Linking lexer test..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(LDFLAGS) -o $@ $^

$(TEST_MAIN_EXE): $(OBJ_DIR)/test_main.o $(UNITTEST_OBJS) $(COMMON_OBJS) $(LEXER_OBJS)
	@echo "Linking main test..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(LDFLAGS) -o $@ $^

# -----------------------------------------------------------------------------
# Compile test sources
# -----------------------------------------------------------------------------
$(OBJ_DIR)/%.o: %.c
	@echo "Compiling test source $<..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS) -c $< -o $@

# -----------------------------------------------------------------------------
# Compile unittest sources
# -----------------------------------------------------------------------------
$(OBJ_DIR)/%.o: $(UNITTEST_DIR)%.c
	@echo "Compiling unittest source $<..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS) -c $< -o $@

# -----------------------------------------------------------------------------
# Compile project sources (common and lexer)
# -----------------------------------------------------------------------------
$(OBJ_DIR)/%.o: ../../%.c
	@echo "Compiling project source $<..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS) -c $< -o $@

# -----------------------------------------------------------------------------
# Clean only this test's artifacts
# -----------------------------------------------------------------------------
clean:
	@echo "Cleaning lexer tests..."
	$(RM) $(BUILD_DIR)

.PHONY: all test clean
