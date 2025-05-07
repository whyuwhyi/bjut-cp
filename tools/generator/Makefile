# Detect Windows vs Unix
ifeq ($(OS),Windows_NT)
    MKDIR   = if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
    RM      = del /Q 
    EXEEXT  = .exe
    SEP      = \\
else
    MKDIR   = mkdir -p
    RM      = rm -rf
    EXEEXT  = 
    SEP      = /
endif

# Compiler settings
CC      := gcc
CFLAGS  := -Wall -Wextra -O2

# Directories
BUILD_DIR := build
OBJ_DIR   := $(BUILD_DIR)$(SEP)obj

# Files
SRC       := generator.c
OBJ       := $(OBJ_DIR)$(SEP)generator.o
EXEC      := $(BUILD_DIR)$(SEP)generator$(EXEEXT)

.PHONY: all test clean

all: $(EXEC)

# Link executable
$(EXEC): $(OBJ)
	@echo Linking $@
	@$(MKDIR) $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Compile object
$(OBJ): $(SRC)
	@echo Compiling $<
	@$(MKDIR) $(OBJ_DIR)
	$(CC) $(CFLAGS) -c generator.c -o $@

# Run basic test
test: all
	@echo Testing generator...
ifdef COMSPEC
	@$(EXEC) -n 2 -o "$(BUILD_DIR)$(SEP)test_samples.txt"
else
	@$(EXEC) -n 2 -o "$(BUILD_DIR)/test_samples.txt"
endif

# Clean up
clean:
	@echo Cleaning...
	@$(RM) "$(BUILD_DIR)"

