#pragma once

#include <stdint.h>

typedef enum {
    EXAM_PREP_FRACTION_TIMES_WHOLE = 1,
    EXAM_PREP_MIXED_TIMES_WHOLE = 2
} ExamPrepOperation_t;

static inline bool exam_prep_operation_valid(int operation) {
    return operation == EXAM_PREP_FRACTION_TIMES_WHOLE ||
           operation == EXAM_PREP_MIXED_TIMES_WHOLE;
}

static inline bool exam_prep_tier_valid(uint8_t tier) {
    return tier >= 1u && tier <= 7u;
}
