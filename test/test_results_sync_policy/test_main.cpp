#include <unity.h>
#include "results_sync_policy.h"

void test_ori_and_eitan_are_cloud_enabled() {
    TEST_ASSERT_TRUE(results_sync_profile_id_enabled(1));
    TEST_ASSERT_TRUE(results_sync_profile_id_enabled(2));
}

void test_other_profiles_are_not_part_of_this_results_feed() {
    TEST_ASSERT_FALSE(results_sync_profile_id_enabled(0));
    TEST_ASSERT_FALSE(results_sync_profile_id_enabled(3));
}

void test_profile_keys_are_stable_for_timeplus_mapping() {
    TEST_ASSERT_EQUAL_STRING("ori", results_sync_profile_key_from_id(1));
    TEST_ASSERT_EQUAL_STRING("eitan", results_sync_profile_key_from_id(2));
    TEST_ASSERT_NULL(results_sync_profile_key_from_id(3));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_ori_and_eitan_are_cloud_enabled);
    RUN_TEST(test_other_profiles_are_not_part_of_this_results_feed);
    RUN_TEST(test_profile_keys_are_stable_for_timeplus_mapping);
    return UNITY_END();
}
