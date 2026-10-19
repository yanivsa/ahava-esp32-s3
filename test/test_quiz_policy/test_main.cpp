#include <unity.h>
#include "quiz_policy.h"
#include "subjects.h"

void test_only_first_try_correct_counts() {
    TEST_ASSERT_TRUE(quiz_policy_should_count(true, false));
    TEST_ASSERT_FALSE(quiz_policy_should_count(false, false));
    TEST_ASSERT_FALSE(quiz_policy_should_count(true, true));
    TEST_ASSERT_FALSE(quiz_policy_should_count(false, true));
}

void test_only_religion_has_daily_cap() {
    for (int p = 1; p <= 3; ++p) {
        TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(p, AHAVA_SUBJECT_MATH));
        TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(p, AHAVA_SUBJECT_HEBREW));
        TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(p, AHAVA_SUBJECT_ENGLISH));
        TEST_ASSERT_EQUAL_UINT32(10u, quiz_policy_daily_limit(p, AHAVA_SUBJECT_RELIGION));
        TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(p, AHAVA_SUBJECT_CHALLENGES));
#ifdef AHAVA_SUBJECT_EXAM_PREP
        TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(p, AHAVA_SUBJECT_EXAM_PREP));
        TEST_ASSERT_FALSE(quiz_policy_grants_timeplus_credit(AHAVA_SUBJECT_EXAM_PREP));
#endif
    }
}

void test_valid_date_rollover_resets_only_without_unsynced_activity() {
    TEST_ASSERT_EQUAL(QUIZ_DAILY_KEEP,
                      quiz_policy_daily_bucket_action(0, 20260908, false));
    TEST_ASSERT_EQUAL(QUIZ_DAILY_KEEP,
                      quiz_policy_daily_bucket_action(20260909, 20260909, false));
    TEST_ASSERT_EQUAL(QUIZ_DAILY_RESET,
                      quiz_policy_daily_bucket_action(20260909, 20260908, false));
    TEST_ASSERT_EQUAL(QUIZ_DAILY_ADOPT_DATE_KEEP_COUNTS,
                      quiz_policy_daily_bucket_action(20260909, 20260908, true));
    TEST_ASSERT_EQUAL(QUIZ_DAILY_ADOPT_DATE_KEEP_COUNTS,
                      quiz_policy_daily_bucket_action(20260909, 0, true));
}

void test_typed_answer_normalizes_numbers_english_and_hebrew() {
    TEST_ASSERT_TRUE(quiz_policy_typed_answer_matches(
        QUIZ_ANSWER_NUMBER, " 12.0 ", "12", nullptr, nullptr, nullptr));
    TEST_ASSERT_TRUE(quiz_policy_typed_answer_matches(
        QUIZ_ANSWER_TEXT, "  JERUSALEM! ", "jerusalem", nullptr, nullptr, nullptr));
    TEST_ASSERT_TRUE(quiz_policy_typed_answer_matches(
        QUIZ_ANSWER_TEXT, "שָׁאוּל", "שאול", nullptr, nullptr, nullptr));
    TEST_ASSERT_FALSE(quiz_policy_typed_answer_matches(
        QUIZ_ANSWER_TEXT, "דוד", "שאול", nullptr, nullptr, nullptr));
    TEST_ASSERT_TRUE(quiz_policy_typed_answer_matches(
        QUIZ_ANSWER_NUMBER, "27", "27 שקלים", nullptr, nullptr, nullptr));
    TEST_ASSERT_TRUE(quiz_policy_typed_answer_matches(
        QUIZ_ANSWER_NUMBER, "0", "x = 0", nullptr, nullptr, nullptr));
    TEST_ASSERT_FALSE(quiz_policy_typed_answer_matches(
        QUIZ_ANSWER_NUMBER, "2", "x^2", nullptr, nullptr, nullptr));
}

void test_typed_answer_accepts_curated_aliases() {
    TEST_ASSERT_TRUE(quiz_policy_typed_answer_matches(
        QUIZ_ANSWER_TEXT, "המלך שאול", "שאול", "שאול המלך", "המלך שאול", nullptr));
}

void test_ori_prefers_typed_answers_when_recall_is_short_and_direct() {
    TEST_ASSERT_EQUAL(QUIZ_ANSWER_TEXT,
        quiz_policy_answer_mode(1, AHAVA_SUBJECT_RELIGION,
                                "מתי מתחילים בני עדות המזרח לומר סליחות?",
                                "ראש חודש אלול"));
    TEST_ASSERT_EQUAL(QUIZ_ANSWER_TEXT,
        quiz_policy_answer_mode(1, AHAVA_SUBJECT_ENGLISH,
                                "Translate: חלון", "window"));
    TEST_ASSERT_EQUAL(QUIZ_ANSWER_NUMBER,
        quiz_policy_answer_mode(1, AHAVA_SUBJECT_MATH,
                                "כמה הם 7 כפול 8?", "56"));
    TEST_ASSERT_EQUAL(QUIZ_ANSWER_NUMBER,
        quiz_policy_answer_mode(1, AHAVA_SUBJECT_MATH,
                                "פתרו את המשוואה", "x = 0"));
    TEST_ASSERT_EQUAL(QUIZ_ANSWER_NUMBER,
        quiz_policy_answer_mode(1, AHAVA_SUBJECT_MATH,
                                "כמה תעלה הנסיעה?", "27 שקלים"));
    TEST_ASSERT_EQUAL(QUIZ_ANSWER_TEXT,
        quiz_policy_answer_mode(1, AHAVA_SUBJECT_MATH,
                                "האם הפונקציה עולה או יורדת?", "הפונקציה יורדת לכל x"));
}

void test_multiple_choice_is_kept_for_choice_style_or_long_answers() {
    TEST_ASSERT_EQUAL(QUIZ_ANSWER_MULTIPLE_CHOICE,
        quiz_policy_answer_mode(1, AHAVA_SUBJECT_RELIGION,
                                "איזה משפט משווה נכון בין תחילת הסליחות של העדות?",
                                "בני המזרח מתחילים מוקדם יותר מבני אשכנז"));
    TEST_ASSERT_EQUAL(QUIZ_ANSWER_MULTIPLE_CHOICE,
        quiz_policy_answer_mode(1, AHAVA_SUBJECT_HEBREW,
                                "איזו מהאפשרויות מתארת בצורה המדויקת ביותר את המשפט?",
                                "זוהי תשובה ארוכה שמחייבת בחירה בין ניסוחים קרובים ולכן אינה מתאימה להקלדה"));
    TEST_ASSERT_EQUAL(QUIZ_ANSWER_MULTIPLE_CHOICE,
        quiz_policy_answer_mode(3, AHAVA_SUBJECT_MATH,
                                "כמה תפוחים יש?", "2"));
}

void test_eitan_uses_typed_input_conservatively() {
    TEST_ASSERT_EQUAL(QUIZ_ANSWER_NUMBER,
        quiz_policy_answer_mode(2, AHAVA_SUBJECT_MATH,
                                "כמה הם 6 כפול 7?", "42"));
    TEST_ASSERT_EQUAL(QUIZ_ANSWER_MULTIPLE_CHOICE,
        quiz_policy_answer_mode(2, AHAVA_SUBJECT_RELIGION,
                                "מה עושים בחג?", "תשובה"));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_only_first_try_correct_counts);
    RUN_TEST(test_only_religion_has_daily_cap);
    RUN_TEST(test_valid_date_rollover_resets_only_without_unsynced_activity);
    RUN_TEST(test_typed_answer_normalizes_numbers_english_and_hebrew);
    RUN_TEST(test_typed_answer_accepts_curated_aliases);
    RUN_TEST(test_ori_prefers_typed_answers_when_recall_is_short_and_direct);
    RUN_TEST(test_multiple_choice_is_kept_for_choice_style_or_long_answers);
    RUN_TEST(test_eitan_uses_typed_input_conservatively);
    return UNITY_END();
}
