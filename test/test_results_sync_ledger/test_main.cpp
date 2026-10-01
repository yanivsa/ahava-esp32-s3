#include <unity.h>
#include "results_sync_ledger.h"

void test_increment_marks_subject_dirty() {
    ResultSyncLedger_t ledger{};
    results_sync_ledger_clear(&ledger);
    TEST_ASSERT_TRUE(results_sync_ledger_increment(&ledger, 20261001u, 0));
    const int idx = results_sync_ledger_find(&ledger, 20261001u);
    TEST_ASSERT_GREATER_OR_EQUAL(0, idx);
    TEST_ASSERT_EQUAL_UINT16(1, ledger.days[idx].correct[0]);
    TEST_ASSERT_TRUE((ledger.days[idx].dirty_mask & 0x01u) != 0);
}

void test_floor_is_absolute_and_monotonic() {
    ResultSyncLedger_t ledger{};
    results_sync_ledger_clear(&ledger);
    TEST_ASSERT_TRUE(results_sync_ledger_set_floor(&ledger, 20261001u, 1, 7));
    TEST_ASSERT_FALSE(results_sync_ledger_set_floor(&ledger, 20261001u, 1, 5));
    const int idx = results_sync_ledger_find(&ledger, 20261001u);
    TEST_ASSERT_EQUAL_UINT16(7, ledger.days[idx].correct[1]);
}

void test_ack_does_not_clear_newer_increment() {
    ResultSyncLedger_t ledger{};
    results_sync_ledger_clear(&ledger);
    results_sync_ledger_set_floor(&ledger, 20261001u, 2, 3);
    results_sync_ledger_increment(&ledger, 20261001u, 2);
    TEST_ASSERT_FALSE(results_sync_ledger_ack(&ledger, 20261001u, 2, 3));
    const int idx = results_sync_ledger_find(&ledger, 20261001u);
    TEST_ASSERT_TRUE((ledger.days[idx].dirty_mask & (1u << 2)) != 0);
    TEST_ASSERT_TRUE(results_sync_ledger_ack(&ledger, 20261001u, 2, 4));
    TEST_ASSERT_TRUE((ledger.days[idx].dirty_mask & (1u << 2)) == 0);
}

void test_full_ledger_never_evicts_dirty_days() {
    ResultSyncLedger_t ledger{};
    results_sync_ledger_clear(&ledger);
    for (int i = 0; i < RESULT_SYNC_LEDGER_DAYS; ++i) {
        TEST_ASSERT_TRUE(results_sync_ledger_increment(&ledger, 20260101u + (uint32_t)i, 0));
    }
    TEST_ASSERT_EQUAL_INT(-1, results_sync_ledger_alloc(&ledger, 20991231u));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_increment_marks_subject_dirty);
    RUN_TEST(test_floor_is_absolute_and_monotonic);
    RUN_TEST(test_ack_does_not_clear_newer_increment);
    RUN_TEST(test_full_ledger_never_evicts_dirty_days);
    return UNITY_END();
}
