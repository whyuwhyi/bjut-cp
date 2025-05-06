/**
 * @file generator.h
 * @brief Sample generator for lexical analyzer testing
 *
 * This file defines functions for generating test samples for
 * the lexical analyzer, in a format that can be read by the main program.
 */

#ifndef GENERATOR_H
#define GENERATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * @brief Generate lexical analyzer test samples
 *
 * This function generates test samples in the format:
 *
 * sample:
 * [input]
 *
 * expected output:
 * [output tokens]
 *
 * @param count Number of samples to generate
 * @param filename Output filename (NULL for stdout)
 * @param seed Random seed (0 for time-based seed)
 * @return int Number of samples generated, or negative on error
 */
int generate_lexer_samples(int count, const char *filename, unsigned int seed);

#endif /* GENERATOR_H */
