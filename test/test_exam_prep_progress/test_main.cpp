#include <unity.h>
#include <stdint.h>
#include <string.h>

#if __has_include("exam_prep_progress.h")
#include "exam_prep_progress.h"
#else
static inline bool exam_prep_progress_is_solved(const uint32_t[3], uint16_t) { return false; }
static inline void exam_prep_progress_mark_solved(uint32_t[3], uint16_t) {}
static inline uint16_t exam_prep_progress_total_solved(const uint32_t[3]) { return 0; }
static inline uint8_t exam_prep_progress_current_tier(const uint32_t[3]) { return 0; }
static inline uint8_t exam_prep_progress_tier_solved(const uint32_t[3], uint8_t) { return 0; }
#endif

void test_progress_bit_boundaries_and_idempotency() {
    uint32_t words[3] = {0, 0, 0};
    exam_prep_progress_mark_solved(words, 0);
    exam_prep_progress_mark_solved(words, 9);
    exam_prep_progress_mark_solved(words, 10);
    exam_prep_progress_mark_solved(words, 69);
    exam_prep_progress_mark_solved(words, 69);
    TEST_ASSERT_TRUE(exam_prep_progress_is_solved(words, 0));
    TEST_ASSERT_TRUE(exam_prep_progress_is_solved(words, 9));
    TEST_ASSERT_TRUE(exam_prep_progress_is_solved(words, 10));
    TEST_ASSERT_TRUE(exam_prep_progress_is_solved(words, 69));
    TEST_ASSERT_EQUAL_UINT16(4, exam_prep_progress_total_solved(words));
}

void test_tier_unlock_requires_all_ten_questions() {
    uint32_t words[3] = {0, 0, 0};
    TEST_ASSERT_EQUAL_UINT8(1, exam_prep_progress_current_tier(words));
    for (uint16_t i = 0; i < 9; ++i) exam_prep_progress_mark_solved(words, i);
    TEST_ASSERT_EQUAL_UINT8(1, exam_prep_progress_current_tier(words));
    TEST_ASSERT_EQUAL_UINT8(9, exam_prep_progress_tier_solved(words, 1));
    exam_prep_progress_mark_solved(words, 9);
    TEST_ASSERT_EQUAL_UINT8(2, exam_prep_progress_current_tier(words));
}

void test_seventy_solved_enters_master_state() {
    uint32_t words[3] = {0, 0, 0};
    for (uint16_t i = 0; i < 70; ++i) exam_prep_progress_mark_solved(words, i);
    TEST_ASSERT_EQUAL_UINT16(70, exam_prep_progress_total_solved(words));
    TEST_ASSERT_EQUAL_UINT8(8, exam_prep_progress_current_tier(words));
    TEST_ASSERT_EQUAL_UINT8(10, exam_prep_progress_tier_solved(words, 7));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_progress_bit_boundaries_and_idempotency);
    RUN_TEST(test_tier_unlock_requires_all_ten_questions);
    RUN_TEST(test_seventy_solved_enters_master_state);
    return UNITY_END();
}
