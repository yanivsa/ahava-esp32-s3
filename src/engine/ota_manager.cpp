/**
 * @file ota_manager.cpp
 * @brief Wizard Academy HTTPS OTA Firmware Updater & Wi-Fi Management
 */

#include "ota_manager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include "esp_https_ota.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static OtaStatus_t current_status = OTA_STATUS_IDLE;
static uint8_t progress_pct = 0;
static TaskHandle_t ota_task_handle = NULL;

bool ota_manager_init(void) {
    current_status = OTA_STATUS_IDLE;
    progress_pct = 0;
    
    // Set Wi-Fi to Station mode
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    if (WiFi.SSID().length() > 0) {
        WiFi.begin();  // Reuse credentials saved by the one-time captive portal.
        Serial.println("[OTA] Connecting to the saved Wi-Fi network in background.");
    } else {
        Serial.println("[OTA] No saved Wi-Fi yet; provisioning will start when OTA is pressed.");
    }
    
    Serial.println("[OTA] Wi-Fi & OTA Manager initialized (Station mode).");
    return true;
}

bool ota_is_wifi_connected(void) {
    return (WiFi.status() == WL_CONNECTED);
}

bool ota_wifi_connect(const char *ssid, const char *pass, uint32_t timeout_ms) {
    const char *target_ssid = ssid ? ssid : DEFAULT_WIFI_SSID;
    const char *target_pass = pass ? pass : DEFAULT_WIFI_PASS;

    current_status = OTA_STATUS_CONNECTING_WIFI;
    if (target_ssid[0]) {
        Serial.printf("[OTA] Connecting to Wi-Fi SSID: %s ...\n", target_ssid);
        WiFi.begin(target_ssid, target_pass);
    } else {
        Serial.println("[OTA] Connecting with saved Wi-Fi credentials...");
        WiFi.begin();
    }

    uint32_t start_time = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start_time) < timeout_ms) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED && !target_ssid[0]) {
        Serial.println("[OTA] No saved network. Starting captive portal Ahava-Setup...");
        WiFiManager manager;
        manager.setConfigPortalTimeout(180);
        manager.setConnectTimeout(20);
        if (!manager.autoConnect("Ahava-Setup", "Ahava1234")) {
            Serial.println("[OTA] Wi-Fi provisioning timed out.");
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[OTA] Wi-Fi Connected! IP: %s | RSSI: %d dBm\n", 
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        current_status = OTA_STATUS_IDLE;
        return true;
    } else {
        Serial.println("[OTA] Wi-Fi connection timed out!");
        current_status = OTA_STATUS_FAILED;
        return false;
    }
}

bool ota_perform_update(const char *url) {
    const char *target_url = (url && strlen(url) > 0) ? url : DEFAULT_OTA_FIRMWARE_URL;

    if (!target_url[0]) {
        Serial.println("[OTA] Update URL is not provisioned; refusing OTA.");
        current_status = OTA_STATUS_FAILED;
        return false;
    }

    // 1. Ensure Wi-Fi is connected
    if (!ota_is_wifi_connected()) {
        Serial.println("[OTA] Wi-Fi not connected. Attempting auto-connect...");
        if (!ota_wifi_connect(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASS, 10000)) {
            Serial.println("[OTA] ERROR: Cannot update without Wi-Fi connection.");
            current_status = OTA_STATUS_FAILED;
            return false;
        }
    }

    current_status = OTA_STATUS_DOWNLOADING;
    progress_pct = 0;
    Serial.printf("[OTA] Starting HTTPS OTA update from: %s\n", target_url);

    // 2. Configure HTTPS with the ESP certificate bundle and hostname checks.
    esp_http_client_config_t http_config = {};
    http_config.url = target_url;
    http_config.cert_pem = NULL;
    // Never weaken TLS hostname validation. OTA stays disabled until a trusted
    // endpoint and its CA certificate are provisioned for this device.
    http_config.skip_cert_common_name_check = false;
    http_config.crt_bundle_attach = arduino_esp_crt_bundle_attach;
    http_config.timeout_ms = 20000;
    http_config.keep_alive_enable = true;
    http_config.max_redirection_count = 5;

    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };

    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK) {
        Serial.printf("[OTA] ERROR: esp_https_ota_begin failed (0x%x)\n", err);
        current_status = OTA_STATUS_FAILED;
        return false;
    }

    int image_size = esp_https_ota_get_image_size(https_ota_handle);
    Serial.printf("[OTA] Total firmware binary size: %d bytes\n", image_size);

    // 3. Download & Flash loop
    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }

        int read_len = esp_https_ota_get_image_len_read(https_ota_handle);
        if (image_size > 0) {
            progress_pct = (uint8_t)((read_len * 100) / image_size);
        }
        Serial.printf("[OTA] Flash progress: %d bytes (%u%%)\n", read_len, progress_pct);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (err != ESP_OK) {
        Serial.printf("[OTA] ERROR: esp_https_ota_perform failed (0x%x)\n", err);
        esp_https_ota_abort(https_ota_handle);
        current_status = OTA_STATUS_FAILED;
        return false;
    }

    esp_err_t finish_err = esp_https_ota_finish(https_ota_handle);
    if (finish_err != ESP_OK) {
        Serial.printf("[OTA] ERROR: esp_https_ota_finish failed (0x%x)\n", finish_err);
        current_status = OTA_STATUS_FAILED;
        return false;
    }

    current_status = OTA_STATUS_SUCCESS;
    progress_pct = 100;
    Serial.println("[OTA] ===============================================");
    Serial.println("[OTA] FIRMWARE UPDATE SUCCESSFUL! Restarting MCU...");
    Serial.println("[OTA] ===============================================");
    
    delay(1500);
    esp_restart();
    return true;
}

static void ota_task_worker(void *pvParameters) {
    char *url_copy = (char *)pvParameters;
    ota_perform_update(url_copy);
    if (url_copy) free(url_copy);
    ota_task_handle = NULL;
    vTaskDelete(NULL);
}

void ota_start_async_update(const char *url) {
    if (ota_task_handle != NULL) {
        Serial.println("[OTA] WARN: OTA task already running!");
        return;
    }

    char *url_copy = NULL;
    if (url) {
        url_copy = strdup(url);
    }

    xTaskCreatePinnedToCore(
        ota_task_worker,
        "ota_worker",
        8192,
        url_copy,
        4,
        &ota_task_handle,
        0 // Pinned to Core 0 (leaving Core 1 for GUI)
    );
}

OtaStatus_t ota_get_status(void) {
    return current_status;
}

uint8_t ota_get_progress_pct(void) {
    return progress_pct;
}
