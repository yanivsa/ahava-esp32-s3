#pragma once

#include <stdint.h>

/**
 * A question counts only when the current display was answered correctly
 * before any hint/retry was used.
 */
static inline bool quiz_policy_should_count(bool is_correct, bool hint_used) {
    return is_correct && !hint_used;
}

/**
 * Daily subject cap. Profile 1 is Ori and subject 3 is Judaism/Halacha.
 * Other profile/subject combinations remain unlimited.
 */
static inline uint32_t quiz_policy_daily_limit(int profile, int subject_id) {
    return (profile == 1 && subject_id == 3) ? 10u : UINT32_MAX;
}
