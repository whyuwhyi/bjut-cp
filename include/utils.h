/**
 * @file utils.h
 * @brief Common utility functions
 *
 * This file contains utility functions used throughout the project,
 * such as file handling, memory management, and string processing.
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Platform-specific macros
 */
#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#define STRDUP _strdup
#else
#define PATH_SEPARATOR '/'
#define STRDUP strdup
#endif

/**
 * @brief Debug output macros
 */
#ifdef CONFIG_DEBUG
#define DEBUG_PRINT(fmt, ...)                                                  \
  fprintf(stderr, "[DEBUG] %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)                                                  \
  do {                                                                         \
  } while (0)
#endif

/**
 * @brief Critical error handling macro
 */
#define FATAL_ERROR(fmt, ...)                                                  \
  do {                                                                         \
    fprintf(stderr, "[FATAL] %s:%d: " fmt "\n", __FILE__, __LINE__,            \
            ##__VA_ARGS__);                                                    \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

/**
 * @brief Safe memory allocation with error checking
 *
 * @param size Number of bytes to allocate
 * @return void* Pointer to allocated memory
 */
void *safe_malloc(size_t size);

/**
 * @brief Safe memory reallocation with error checking
 *
 * @param ptr Pointer to existing memory block
 * @param size New size in bytes
 * @return void* Pointer to reallocated memory
 */
void *safe_realloc(void *ptr, size_t size);

/**
 * @brief Safe string duplication with error checking
 *
 * @param str String to duplicate
 * @return char* Pointer to new string
 */
char *safe_strdup(const char *str);

/**
 * @brief Read entire file into memory
 *
 * @param filename Path to the file
 * @return char* File contents (caller must free)
 */
char *read_file(const char *filename);

/**
 * @brief Write string to file
 *
 * @param filename Path to the file
 * @param content String to write
 * @return bool Success status
 */
bool write_file(const char *filename, const char *content);

/**
 * @brief Check if file exists
 *
 * @param filename Path to the file
 * @return bool True if file exists
 */
bool file_exists(const char *filename);

/**
 * @brief Trim whitespace from the beginning and end of a string
 *
 * @param str String to trim (modified in-place)
 */
void trim_string(char *str);

/**
 * @brief Compare two strings, case-insensitive
 *
 * @param s1 First string
 * @param s2 Second string
 * @return int 0 if equal, negative if s1<s2, positive if s1>s2
 */
int strcasecmp_custom(const char *s1, const char *s2);

/**
 * @brief Join two path strings
 *
 * @param base Base path
 * @param path Path to append
 * @return char* Joined path (caller must free)
 */
char *path_join(const char *base, const char *path);

/**
 * @brief Get file extension
 *
 * @param filename Filename
 * @return const char* Pointer to extension, or NULL if none
 */
const char *get_file_extension(const char *filename);

/**
 * @brief Calculate file hash (simple implementation)
 *
 * @param filename Path to file
 * @return unsigned long Hash value, 0 on error
 */
unsigned long file_hash(const char *filename);

/**
 * @brief Convert integer to string safely
 *
 * Converts an integer value to a null-terminated string using the specified
 * base and stores the result in the provided buffer. The function ensures
 * buffer boundaries are respected to prevent buffer overflow.
 *
 * @param value        The integer value to convert
 * @param buffer       The buffer where the result will be stored
 * @param buffer_size  The size of the buffer in bytes
 * @param base         The base to use for conversion (e.g., 10 for decimal, 16
 * for hex)
 * @return             A pointer to the resulting string, or NULL on failure
 */
char *safe_itoa(int value, char *buffer, size_t buffer_size, int base);

#endif /* UTILS_H */
