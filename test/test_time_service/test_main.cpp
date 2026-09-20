#include <unity.h>
#include "time_service.h"

void test_unknown_clock_stays_unknown() {
    TEST_ASSERT_EQUAL_INT(TIME_SERVICE_UNKNOWN,
        time_service_transition(TIME_SERVICE_UNKNOWN, false, false, false));
}

void test_valid_offline_clock_becomes_running() {
    TEST_ASSERT_EQUAL_INT(TIME_SERVICE_RUNNING,
        time_service_transition(TIME_SERVICE_UNKNOWN, true, false, false));
}

void test_completed_network_sync_becomes_synced() {
    TEST_ASSERT_EQUAL_INT(TIME_SERVICE_SYNCED,
        time_service_transition(TIME_SERVICE_RUNNING, true, true, true));
}

void test_disconnect_keeps_clock_but_downgrades_to_running() {
    TEST_ASSERT_EQUAL_INT(TIME_SERVICE_RUNNING,
        time_service_transition(TIME_SERVICE_SYNCED, true, false, false));
}

void test_invalid_clock_never_claims_synced() {
    TEST_ASSERT_EQUAL_INT(TIME_SERVICE_UNKNOWN,
        time_service_transition(TIME_SERVICE_SYNCED, false, true, true));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_unknown_clock_stays_unknown);
    RUN_TEST(test_valid_offline_clock_becomes_running);
    RUN_TEST(test_completed_network_sync_becomes_synced);
    RUN_TEST(test_disconnect_keeps_clock_but_downgrades_to_running);
    RUN_TEST(test_invalid_clock_never_claims_synced);
    return UNITY_END();
}
