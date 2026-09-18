#include <unity.h>
#include "subjects.h"
#include "weekly_stats.h"

void test_navigation_has_five_categories_but_stats_have_four_subjects() {
    TEST_ASSERT_EQUAL_INT(5, AHAVA_SUBJECT_COUNT);
    TEST_ASSERT_EQUAL_INT(4, AHAVA_SUBJECT_CHALLENGES);
    TEST_ASSERT_EQUAL_INT(4, AHAVA_ACADEMIC_SUBJECT_COUNT);
    TEST_ASSERT_EQUAL_INT(AHAVA_ACADEMIC_SUBJECT_COUNT, WEEKLY_STATS_SUBJECTS);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_navigation_has_five_categories_but_stats_have_four_subjects);
    return UNITY_END();
}
