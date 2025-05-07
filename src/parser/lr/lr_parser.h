/**
 * @file lr_parser.h
 * @brief Base LR parser implementation (internal)
 */

#ifndef LR_PARSER_H
#define LR_PARSER_H

#include "../parser_common.h"
#include "lr_common.h"

/**
 * @brief Create an automaton closure
 *
 * @param automaton Automaton
 * @param state State to add closure to
 * @param with_lookaheads Whether to include lookaheads
 * @return bool Success status
 */
bool lr_closure(LRAutomaton *automaton, LRState *state, bool with_lookaheads);

/**
 * @brief Compute GOTO for an LR state
 *
 * @param automaton Automaton
 * @param state Current state
 * @param symbol_id Symbol ID
 * @return LRState* Resulting state, or NULL on failure
 */
LRState *lr_goto(LRAutomaton *automaton, LRState *state, int symbol_id);

/**
 * @brief Create collection of canonical LR items
 *
 * @param automaton Automaton
 * @param grammar Grammar
 * @param with_lookaheads Whether to include lookaheads
 * @return bool Success status
 */
bool lr_create_canonical_collection(LRAutomaton *automaton, Grammar *grammar,
                                    bool with_lookaheads);

/**
 * @brief Calculate FIRST set for a sequence of symbols
 *
 * @param grammar Grammar
 * @param symbols Sequence of symbols
 * @param symbol_count Number of symbols
 * @param first_set Output first set (must be at least terminal_count + 1 in
 * size)
 * @return bool Success status
 */
bool lr_calculate_first_of_sequence(Grammar *grammar, Symbol *symbols,
                                    int symbol_count, bool *first_set);

#endif /* LR_PARSER_H */
