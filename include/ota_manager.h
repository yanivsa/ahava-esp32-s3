/**
 * @file ota_manager.h
 * @brief Wizard Academy (אקדמיית הקוסמים) Wi-Fi & HTTPS OTA Update Manager
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Default Wi-Fi Credentials (Can be customized or configured via NVS/SmartConfig) */
#define DEFAULT_WIFI_SSID       ""
#define DEFAULT_WIFI_PASS       ""
#define DEFAULT_OTA_FIRMWARE_URL "https://github.com/yanivsa/ahava-esp32-s3/releases/latest/download/firmware.bin"

typedef enum {
    OTA_STATUS_IDLE = 0,
    OTA_STATUS_CONNECTING_WIFI,
    OTA_STATUS_DOWNLOADING,
    OTA_STATUS_SUCCESS,
    OTA_STATUS_FAILED
} OtaStatus_t;

/**
 * @brief Initialize Wi-Fi subsystem and OTA manager.
 * @return true on success, false otherwise.
 */
bool ota_manager_init(void);

/**
 * @brief Connect to Wi-Fi network in Station mode.
 * @param ssid Wi-Fi network SSID (NULL for default).
 * @param pass Wi-Fi network Password (NULL for default).
 * @param timeout_ms Connection timeout in milliseconds.
 * @return true if connected with IP, false on timeout.
 */
bool ota_wifi_connect(const char *ssid, const char *pass, uint32_t timeout_ms);

/**
 * @brief Perform HTTPS OTA firmware update synchronously.
 *        Downloads binary, writes to next partition, and restarts upon completion.
 * @param url HTTPS URL to the compiled firmware .bin file.
 * @return true on success (will restart), false on failure.
 */
bool ota_perform_update(const char *url);

/**
 * @brief Launch non-blocking OTA update in background FreeRTOS task on Core 0.
 * @param url HTTPS URL to firmware .bin.
 */
void ota_start_async_update(const char *url);

/**
 * @brief Check current OTA status.
 */
OtaStatus_t ota_get_status(void);

/**
 * @brief Get current OTA download progress percentage (0 - 100).
 */
uint8_t ota_get_progress_pct(void);

/**
 * @brief Check if Wi-Fi is currently connected.
 */
bool ota_is_wifi_connected(void);

#ifdef __cplusplus
}
#endif
