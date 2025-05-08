/**
 * @file utils.c
 * @brief Implementation of common utility functions
 */

#include "utils.h"
#include <ctype.h>
#include <limits.h>

/**
 * Safe memory allocation with error checking
 */
void *safe_malloc(size_t size) {
  void *ptr = malloc(size);
  if (!ptr && size > 0) {
    FATAL_ERROR("Memory allocation failed for %zu bytes", size);
  }
  return ptr;
}

/**
 * Safe memory reallocation with error checking
 */
void *safe_realloc(void *ptr, size_t size) {
  void *new_ptr = realloc(ptr, size);
  if (!new_ptr && size > 0) {
    FATAL_ERROR("Memory reallocation failed for %zu bytes", size);
  }
  return new_ptr;
}

/**
 * Safe string duplication with error checking
 */
char *safe_strdup(const char *str) {
  if (str == NULL) {
    return NULL;
  }

  char *dup = STRDUP(str);
  if (!dup) {
    FATAL_ERROR("String duplication failed for %zu bytes", strlen(str) + 1);
  }
  return dup;
}

/**
 * Read entire file into memory
 */
char *read_file(const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    DEBUG_PRINT("Failed to open file: %s", filename);
    return NULL;
  }

  // Get file size
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  rewind(file);

  // Allocate buffer with extra space for null terminator
  char *buffer = (char *)safe_malloc(file_size + 1);

  // Read file data
  size_t bytes_read = fread(buffer, 1, file_size, file);
  if (bytes_read != (size_t)file_size) {
    DEBUG_PRINT("Failed to read file: %s", filename);
    free(buffer);
    fclose(file);
    return NULL;
  }

  // Null-terminate the buffer
  buffer[file_size] = '\0';

  fclose(file);
  return buffer;
}

/**
 * Write string to file
 */
bool write_file(const char *filename, const char *content) {
  FILE *file = fopen(filename, "wb");
  if (!file) {
    DEBUG_PRINT("Failed to open file for writing: %s", filename);
    return false;
  }

  size_t length = strlen(content);
  size_t bytes_written = fwrite(content, 1, length, file);
  fclose(file);

  if (bytes_written != length) {
    DEBUG_PRINT("Failed to write complete data to file: %s", filename);
    return false;
  }

  return true;
}

/**
 * Check if file exists
 */
bool file_exists(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (file) {
    fclose(file);
    return true;
  }
  return false;
}

/**
 * Trim whitespace from the beginning and end of a string
 */
void trim_string(char *str) {
  if (!str)
    return;

  // Trim leading spaces
  char *start = str;
  while (isspace((unsigned char)*start)) {
    start++;
  }

  // All spaces?
  if (*start == '\0') {
    *str = '\0';
    return;
  }

  // Trim trailing spaces
  char *end = start + strlen(start) - 1;
  while (end > start && isspace((unsigned char)*end)) {
    end--;
  }

  // Null terminate the trimmed string
  *(end + 1) = '\0';

  // If there were leading spaces, shift the trimmed string
  if (start != str) {
    memmove(str, start, (end - start) + 2);
  }
}

/**
 * Compare two strings, case-insensitive
 */
int strcasecmp_custom(const char *s1, const char *s2) {
  if (!s1 && !s2)
    return 0;
  if (!s1)
    return -1;
  if (!s2)
    return 1;

  while (*s1 && *s2) {
    int diff = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
    if (diff != 0) {
      return diff;
    }
    s1++;
    s2++;
  }

  return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

/**
 * Join two path strings
 */
char *path_join(const char *base, const char *path) {
  if (!base || !path)
    return NULL;

  size_t base_len = strlen(base);
  size_t path_len = strlen(path);

  // Check if base ends with separator
  bool has_separator = (base_len > 0 && base[base_len - 1] == PATH_SEPARATOR);

  // Allocate memory: base + possible separator + path + null terminator
  char *result =
      (char *)safe_malloc(base_len + (has_separator ? 0 : 1) + path_len + 1);

  // Copy base
  strcpy(result, base);

  // Add separator if needed
  if (!has_separator && base_len > 0) {
    result[base_len] = PATH_SEPARATOR;
    strcpy(result + base_len + 1, path);
  } else {
    strcpy(result + base_len, path);
  }

  return result;
}

/**
 * Get file extension
 */
const char *get_file_extension(const char *filename) {
  if (!filename)
    return NULL;

  const char *dot = strrchr(filename, '.');
  if (!dot || dot == filename)
    return NULL;

  return dot + 1;
}

/**
 * Calculate simple file hash
 */
unsigned long file_hash(const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (!file)
    return 0;

  unsigned long hash = 5381;
  int c;

  while ((c = fgetc(file)) != EOF) {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }

  fclose(file);
  return hash;
}

/**
 * @brief Convert integer to string safely
 *
 * This function safely converts an integer to a string representation
 * with the specified base (e.g., decimal, hexadecimal).
 */
char *safe_itoa(int value, char *buffer, size_t buffer_size, int base) {
  /* Check arguments */
  if (!buffer || buffer_size == 0 || base < 2 || base > 36) {
    return NULL;
  }

  /* Ensure buffer has space for at least one digit and null terminator */
  if (buffer_size < 2) {
    return NULL;
  }

  /* Handle 0 as a special case */
  if (value == 0) {
    buffer[0] = '0';
    buffer[1] = '\0';
    return buffer;
  }

  /* Handle negative numbers */
  int is_negative = 0;
  if (value < 0 && base == 10) {
    is_negative = 1;
    /* Avoid overflow when value is INT_MIN */
    if (value == INT_MIN) {
      /* Handle INT_MIN specially */
      static const char int_min_str[] = "-2147483648";
      if (buffer_size < sizeof(int_min_str)) {
        return NULL;
      }
      strcpy(buffer, int_min_str);
      return buffer;
    }
    value = -value;
  }

  /* Convert integer to string in reverse order */
  static const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  size_t pos = 0;
  char temp_buffer[33]; /* Max 32 digits for 32-bit int in binary + null
                           terminator */

  /* Generate digits in reverse order */
  while (value > 0 && pos < sizeof(temp_buffer) - 1) {
    temp_buffer[pos++] = digits[value % base];
    value /= base;
  }

  /* Check if we have enough buffer space */
  if (pos + is_negative + 1 > buffer_size) {
    return NULL;
  }

  /* Copy to output buffer in correct order */
  size_t buffer_pos = 0;

  /* Add negative sign if necessary */
  if (is_negative) {
    buffer[buffer_pos++] = '-';
  }

  /* Copy digits in correct order */
  while (pos > 0) {
    buffer[buffer_pos++] = temp_buffer[--pos];
  }

  /* Add null terminator */
  buffer[buffer_pos] = '\0';

  return buffer;
}
