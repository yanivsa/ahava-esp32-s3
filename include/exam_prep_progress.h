#pragma once

#include <stdint.h>

#define EXAM_PREP_QUESTION_COUNT 70u
#define EXAM_PREP_TIER_COUNT 7u
#define EXAM_PREP_PER_TIER 10u
#define EXAM_PREP_PROGRESS_WORDS 3u

static inline bool exam_prep_progress_is_solved(const uint32_t words[3], uint16_t ordinal) {
    if (!words || ordinal >= EXAM_PREP_QUESTION_COUNT) return false;
    return (words[ordinal / 32u] & (1u << (ordinal % 32u))) != 0u;
}

static inline void exam_prep_progress_mark_solved(uint32_t words[3], uint16_t ordinal) {
    if (!words || ordinal >= EXAM_PREP_QUESTION_COUNT) return;
    words[ordinal / 32u] |= (1u << (ordinal % 32u));
}

static inline uint16_t exam_prep_progress_total_solved(const uint32_t words[3]) {
    if (!words) return 0;
    uint16_t total = 0;
    for (uint16_t i = 0; i < EXAM_PREP_QUESTION_COUNT; ++i) {
        if (exam_prep_progress_is_solved(words, i)) ++total;
    }
    return total;
}

static inline uint8_t exam_prep_progress_tier_solved(const uint32_t words[3], uint8_t tier) {
    if (!words || tier < 1u || tier > EXAM_PREP_TIER_COUNT) return 0;
    uint8_t count = 0;
    const uint16_t start = (uint16_t)(tier - 1u) * EXAM_PREP_PER_TIER;
    for (uint16_t i = 0; i < EXAM_PREP_PER_TIER; ++i) {
        if (exam_prep_progress_is_solved(words, start + i)) ++count;
    }
    return count;
}

/** Returns 1..7 for the first incomplete tier, or 8 after all 70 are mastered. */
static inline uint8_t exam_prep_progress_current_tier(const uint32_t words[3]) {
    if (!words) return 1;
    for (uint8_t tier = 1; tier <= EXAM_PREP_TIER_COUNT; ++tier) {
        if (exam_prep_progress_tier_solved(words, tier) < EXAM_PREP_PER_TIER) return tier;
    }
    return EXAM_PREP_TIER_COUNT + 1u;
}
