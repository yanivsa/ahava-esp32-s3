#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TIME_SERVICE_UNKNOWN = 0,
    TIME_SERVICE_RUNNING,
    TIME_SERVICE_SYNCED
} TimeServiceState_t;

static inline TimeServiceState_t time_service_transition(TimeServiceState_t current,
                                                         bool clock_valid,
                                                         bool sync_completed,
                                                         bool network_connected) {
    if (!clock_valid) return TIME_SERVICE_UNKNOWN;
    if (sync_completed && network_connected) return TIME_SERVICE_SYNCED;
    if (!network_connected && current == TIME_SERVICE_SYNCED) return TIME_SERVICE_RUNNING;
    return current == TIME_SERVICE_UNKNOWN ? TIME_SERVICE_RUNNING : current;
}

bool time_service_init(void);
void time_service_request_sync(void);
void time_service_poll(void);
void time_service_set_network_connected(bool connected);
TimeServiceState_t time_service_get_state(void);
bool time_service_has_trusted_time(void);
uint32_t time_service_today_id(void);
int64_t time_service_now_epoch(void);
int64_t time_service_last_sync_epoch(void);
const char *time_service_state_label_he(void);
void time_service_format_now(char *buf, size_t len);
void time_service_format_last_sync(char *buf, size_t len);

#ifdef __cplusplus
}
#endif
