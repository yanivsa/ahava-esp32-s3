#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#define RESULT_SYNC_LEDGER_VERSION 1u
#define RESULT_SYNC_LEDGER_DAYS 120
#define RESULT_SYNC_LEDGER_SUBJECTS 4

typedef struct {
    uint32_t date;
    uint16_t correct[RESULT_SYNC_LEDGER_SUBJECTS];
    uint8_t dirty_mask;
} ResultSyncLedgerDay_t;

typedef struct {
    uint32_t version;
    ResultSyncLedgerDay_t days[RESULT_SYNC_LEDGER_DAYS];
} ResultSyncLedger_t;

static inline void results_sync_ledger_clear(ResultSyncLedger_t *ledger) {
    if (!ledger) return;
    memset(ledger, 0, sizeof(*ledger));
    ledger->version = RESULT_SYNC_LEDGER_VERSION;
}

static inline int results_sync_ledger_find(const ResultSyncLedger_t *ledger, uint32_t date) {
    if (!ledger || date == 0) return -1;
    for (int i = 0; i < RESULT_SYNC_LEDGER_DAYS; ++i) {
        if (ledger->days[i].date == date) return i;
    }
    return -1;
}

static inline int results_sync_ledger_alloc(ResultSyncLedger_t *ledger, uint32_t date) {
    if (!ledger || date == 0) return -1;
    int existing = results_sync_ledger_find(ledger, date);
    if (existing >= 0) return existing;

    int free_slot = -1;
    int oldest_clean = -1;
    uint32_t oldest_clean_date = UINT32_MAX;
    for (int i = 0; i < RESULT_SYNC_LEDGER_DAYS; ++i) {
        if (ledger->days[i].date == 0) {
            free_slot = i;
            break;
        }
        if (ledger->days[i].dirty_mask == 0 && ledger->days[i].date < oldest_clean_date) {
            oldest_clean_date = ledger->days[i].date;
            oldest_clean = i;
        }
    }

    const int slot = free_slot >= 0 ? free_slot : oldest_clean;
    if (slot < 0) return -1;

    memset(&ledger->days[slot], 0, sizeof(ledger->days[slot]));
    ledger->days[slot].date = date;
    return slot;
}

static inline bool results_sync_ledger_set_floor(ResultSyncLedger_t *ledger,
                                                 uint32_t date,
                                                 int subject,
                                                 uint16_t value) {
    if (!ledger || date == 0 || subject < 0 || subject >= RESULT_SYNC_LEDGER_SUBJECTS) return false;
    const int idx = results_sync_ledger_alloc(ledger, date);
    if (idx < 0) return false;

    uint16_t *current = &ledger->days[idx].correct[subject];
    if (value <= *current) return false;
    *current = value;
    ledger->days[idx].dirty_mask |= (uint8_t)(1u << subject);
    return true;
}

static inline bool results_sync_ledger_increment(ResultSyncLedger_t *ledger,
                                                 uint32_t date,
                                                 int subject) {
    if (!ledger || date == 0 || subject < 0 || subject >= RESULT_SYNC_LEDGER_SUBJECTS) return false;
    const int idx = results_sync_ledger_alloc(ledger, date);
    if (idx < 0) return false;

    uint16_t *current = &ledger->days[idx].correct[subject];
    if (*current < UINT16_MAX) ++(*current);
    ledger->days[idx].dirty_mask |= (uint8_t)(1u << subject);
    return true;
}

static inline bool results_sync_ledger_ack(ResultSyncLedger_t *ledger,
                                           uint32_t date,
                                           int subject,
                                           uint16_t sent_value) {
    if (!ledger || subject < 0 || subject >= RESULT_SYNC_LEDGER_SUBJECTS) return false;
    const int idx = results_sync_ledger_find(ledger, date);
    if (idx < 0) return false;

    if (ledger->days[idx].correct[subject] != sent_value) {
        return false; // New activity happened while the request was in flight.
    }

    ledger->days[idx].dirty_mask &= (uint8_t)~(1u << subject);
    return true;
}
