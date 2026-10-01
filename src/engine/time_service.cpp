#include "time_service.h"

#include <Arduino.h>
#include <time.h>
#include <stdlib.h>
#include "nvs.h"
#include "esp_sntp.h"

namespace {

static constexpr const char *TIME_NVS_NAMESPACE = "time_svc";
static constexpr const char *ISRAEL_TZ = "IST-2IDT,M3.4.4/26,M10.5.0";
static constexpr time_t MIN_TRUSTED_EPOCH = 1735689600;
static constexpr uint32_t MIN_SYNC_SETTLE_MS = 500;

static TimeServiceState_t s_state = TIME_SERVICE_UNKNOWN;
static bool s_network_connected = false;
static bool s_sync_requested = false;
static uint32_t s_sync_request_ms = 0;
static int64_t s_last_sync_epoch = 0;

static bool valid_epoch(time_t epoch) {
    return epoch >= MIN_TRUSTED_EPOCH;
}

static uint64_t read_u64(const char *key, uint64_t fallback) {
    nvs_handle_t handle;
    if (nvs_open(TIME_NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) return fallback;
    uint64_t value = fallback;
    const esp_err_t err = nvs_get_u64(handle, key, &value);
    nvs_close(handle);
    return err == ESP_OK ? value : fallback;
}

static bool write_u64(const char *key, uint64_t value) {
    nvs_handle_t handle;
    if (nvs_open(TIME_NVS_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return false;
    esp_err_t err = nvs_set_u64(handle, key, value);
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    return err == ESP_OK;
}

static void persist_confirmed_sync(time_t now) {
    if (!valid_epoch(now)) return;
    s_last_sync_epoch = (int64_t)now;
    if (!write_u64("last_sync", (uint64_t)now)) {
        Serial.println("[TIME] WARN: failed to persist last confirmed SNTP sync.");
    }
}

static void format_epoch(char *buf, size_t len, time_t epoch, const char *fallback) {
    if (!buf || len == 0) return;
    if (!valid_epoch(epoch)) {
        snprintf(buf, len, "%s", fallback);
        return;
    }
    struct tm info{};
    if (localtime_r(&epoch, &info) == nullptr) {
        snprintf(buf, len, "%s", fallback);
        return;
    }
    snprintf(buf, len, "%02d/%02d/%04d %02d:%02d",
             info.tm_mday, info.tm_mon + 1, info.tm_year + 1900,
             info.tm_hour, info.tm_min);
}

} // namespace

bool time_service_init(void) {
    setenv("TZ", ISRAEL_TZ, 1);
    tzset();

    s_last_sync_epoch = (int64_t)read_u64("last_sync", 0);
    const time_t now = time(nullptr);
    s_state = valid_epoch(now) ? TIME_SERVICE_RUNNING : TIME_SERVICE_UNKNOWN;

    Serial.printf("[TIME] TimeService initialized: state=%d last_sync=%lld current=%lld\n",
                  (int)s_state, (long long)s_last_sync_epoch, (long long)now);
    return true;
}

void time_service_request_sync(void) {
    Serial.println("[TIME] Requesting SNTP sync (Israel timezone)...");
    configTzTime(ISRAEL_TZ, "pool.ntp.org", "time.google.com");
    s_sync_requested = true;
    s_sync_request_ms = millis();
}

void time_service_poll(void) {
    const time_t now = time(nullptr);
    const bool clock_valid = valid_epoch(now);

    bool sync_completed = false;
    if (s_sync_requested && s_network_connected &&
        (millis() - s_sync_request_ms) >= MIN_SYNC_SETTLE_MS) {
        sync_completed = (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED);
    }

    const TimeServiceState_t before = s_state;
    s_state = time_service_transition(s_state, clock_valid, sync_completed, s_network_connected);

    if (sync_completed) {
        persist_confirmed_sync(now);
        s_sync_requested = false;
        Serial.printf("[TIME] SNTP confirmed at epoch=%lld date=%u.\n",
                      (long long)now, (unsigned)time_service_today_id());
    } else if (before != s_state) {
        Serial.printf("[TIME] State changed %d -> %d.\n", (int)before, (int)s_state);
    }
}

void time_service_set_network_connected(bool connected) {
    s_network_connected = connected;
    const time_t now = time(nullptr);
    s_state = time_service_transition(s_state, valid_epoch(now), false, connected);
}

TimeServiceState_t time_service_get_state(void) {
    return s_state;
}

bool time_service_has_trusted_time(void) {
    return valid_epoch(time(nullptr));
}

uint32_t time_service_today_id(void) {
    const time_t now = time(nullptr);
    if (!valid_epoch(now)) return 0;

    struct tm info{};
    if (localtime_r(&now, &info) == nullptr) return 0;
    return (uint32_t)((info.tm_year + 1900) * 10000 +
                      (info.tm_mon + 1) * 100 +
                      info.tm_mday);
}

int64_t time_service_now_epoch(void) {
    const time_t now = time(nullptr);
    return valid_epoch(now) ? (int64_t)now : 0;
}

int64_t time_service_last_sync_epoch(void) {
    return s_last_sync_epoch;
}

const char *time_service_state_label_he(void) {
    switch (s_state) {
        case TIME_SERVICE_SYNCED: return "מסונכרן";
        case TIME_SERVICE_RUNNING: return "שעון פנימי";
        default: return "לא ידוע";
    }
}

void time_service_format_now(char *buf, size_t len) {
    format_epoch(buf, len, time(nullptr), "--");
}

void time_service_format_last_sync(char *buf, size_t len) {
    format_epoch(buf, len, (time_t)s_last_sync_epoch, "טרם");
}
