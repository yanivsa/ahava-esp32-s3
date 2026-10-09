#include <unity.h>
#include <stdint.h>

#if __has_include("exam_prep_workflow.h")
#include "exam_prep_workflow.h"
#else
typedef enum { EXAM_WORK_1 = 1, EXAM_WORK_2 = 2, EXAM_WORK_FINAL = 3 } ExamPrepWorkPhase_t;
typedef struct { ExamPrepWorkPhase_t phase; uint8_t correct1; uint8_t correct2; bool has_second; } ExamPrepWorkState_t;
static inline void exam_prep_workflow_reset(ExamPrepWorkState_t *s, uint8_t c1, bool second, uint8_t c2) { s->phase=EXAM_WORK_FINAL; s->correct1=c1; s->correct2=c2; s->has_second=second; }
static inline bool exam_prep_workflow_choose(ExamPrepWorkState_t *, uint8_t) { return false; }
#endif

void test_one_checkpoint_requires_correct_work_before_final() {
    ExamPrepWorkState_t s{};
    exam_prep_workflow_reset(&s, 2, false, 0);
    TEST_ASSERT_EQUAL(EXAM_WORK_1, s.phase);
    TEST_ASSERT_FALSE(exam_prep_workflow_choose(&s, 1));
    TEST_ASSERT_EQUAL(EXAM_WORK_1, s.phase);
    TEST_ASSERT_TRUE(exam_prep_workflow_choose(&s, 2));
    TEST_ASSERT_EQUAL(EXAM_WORK_FINAL, s.phase);
}

void test_two_checkpoints_cannot_be_skipped() {
    ExamPrepWorkState_t s{};
    exam_prep_workflow_reset(&s, 1, true, 3);
    TEST_ASSERT_EQUAL(EXAM_WORK_1, s.phase);
    TEST_ASSERT_TRUE(exam_prep_workflow_choose(&s, 1));
    TEST_ASSERT_EQUAL(EXAM_WORK_2, s.phase);
    TEST_ASSERT_FALSE(exam_prep_workflow_choose(&s, 2));
    TEST_ASSERT_EQUAL(EXAM_WORK_2, s.phase);
    TEST_ASSERT_TRUE(exam_prep_workflow_choose(&s, 3));
    TEST_ASSERT_EQUAL(EXAM_WORK_FINAL, s.phase);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_one_checkpoint_requires_correct_work_before_final);
    RUN_TEST(test_two_checkpoints_cannot_be_skipped);
    return UNITY_END();
}
