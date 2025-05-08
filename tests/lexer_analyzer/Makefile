# Detect platform
ifeq ($(OS),Windows_NT)
  MKDIR   = if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
  RM      = del /Q
  SEP     = \\
  EXEEXT  = .exe
else
  MKDIR   = mkdir -p $1
  RM      = rm -rf
  SEP     = /
  EXEEXT  =
endif

# Compiler settings
CC      := gcc
CFLAGS  := -Wall -Wextra -O2
LDFLAGS :=

# Local build directories (side-by-side with this Makefile)
BUILD_DIR := build
OBJ_DIR   := $(BUILD_DIR)$(SEP)obj

# Test sources and objects
TEST_SRCS := test_token.c test_lexer.c test_main.c
TEST_OBJS := $(patsubst %.c,$(OBJ_DIR)$(SEP)%.o,$(TEST_SRCS))

# Dependencies: libraries built by root Makefile into ../..$(SEP)build
ROOT_BUILD := ..$(SEP)..$(SEP)build
COMMON_LIB := $(ROOT_BUILD)$(SEP)libcommon.a
LEXER_LIB  := $(ROOT_BUILD)$(SEP)liblexer.a

# Test executables
TEST_TOKEN_EXE := $(BUILD_DIR)$(SEP)test_token$(EXEEXT)
TEST_LEXER_EXE := $(BUILD_DIR)$(SEP)test_lexer$(EXEEXT)
TEST_MAIN_EXE  := $(BUILD_DIR)$(SEP)test_main$(EXEEXT)

# Sample files for test_main
SAMPLE_DIR := samples
SAMPLE_FILES := $(SAMPLE_DIR)$(SEP)sample1.txt

# Unittest files from tests directory
UNITTEST_DIR := ..$(SEP)
UNITTEST_SRCS := $(UNITTEST_DIR)unittest.c
UNITTEST_OBJS := $(patsubst $(UNITTEST_DIR)%.c,$(OBJ_DIR)$(SEP)%.o,$(UNITTEST_SRCS))

.PHONY: all prepare test clean

# -----------------------------------------------------------------------------
# Default: build root libs, then test executables
# -----------------------------------------------------------------------------
all: prepare $(TEST_TOKEN_EXE) $(TEST_LEXER_EXE) $(TEST_MAIN_EXE)

# -----------------------------------------------------------------------------
# Prepare dependencies by invoking the root Makefile's build target
# -----------------------------------------------------------------------------
prepare:
	@echo "Invoking root 'build' target for dependencies..."
	@$(MAKE) -C ..$(SEP).. build

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
# Link rules for each test executable - include unittest objects directly
# -----------------------------------------------------------------------------
$(TEST_TOKEN_EXE): $(OBJ_DIR)$(SEP)test_token.o $(UNITTEST_OBJS) $(COMMON_LIB) $(LEXER_LIB)
	@echo "Linking token test..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(LDFLAGS) -o $@ $(OBJ_DIR)$(SEP)test_token.o $(UNITTEST_OBJS) -L$(ROOT_BUILD) -llexer -lcommon

$(TEST_LEXER_EXE): $(OBJ_DIR)$(SEP)test_lexer.o $(UNITTEST_OBJS) $(COMMON_LIB) $(LEXER_LIB)
	@echo "Linking lexer test..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(LDFLAGS) -o $@ $(OBJ_DIR)$(SEP)test_lexer.o $(UNITTEST_OBJS) -L$(ROOT_BUILD) -llexer -lcommon

$(TEST_MAIN_EXE): $(OBJ_DIR)$(SEP)test_main.o $(UNITTEST_OBJS) $(COMMON_LIB) $(LEXER_LIB)
	@echo "Linking main test..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(LDFLAGS) -o $@ $(OBJ_DIR)$(SEP)test_main.o $(UNITTEST_OBJS) -L$(ROOT_BUILD) -llexer -lcommon

# -----------------------------------------------------------------------------
# Compile test sources
# -----------------------------------------------------------------------------
$(OBJ_DIR)$(SEP)%.o: %.c
	@echo "Compiling test source $<..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS) -I..$(SEP)..$(SEP)include -c $< -o $@

# -----------------------------------------------------------------------------
# Compile unittest sources from parent directory
# -----------------------------------------------------------------------------
$(OBJ_DIR)$(SEP)%.o: $(UNITTEST_DIR)%.c
	@echo "Compiling unittest source $<..."
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS) -I..$(SEP)..$(SEP)include -c $< -o $@

# -----------------------------------------------------------------------------
# Clean only this test's artifacts
# -----------------------------------------------------------------------------
clean:
	@echo "Cleaning lexer-analyzer tests..."
	@$(call RM) $(BUILD_DIR)
