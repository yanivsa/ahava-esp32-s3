/**
 * @file hal_lvgl.cpp
 * @brief LVGL 9.x Core Initialization, Double Buffer Allocation, and FreeRTOS GUI Task
 */

#include "hal_lvgl.h"
#include "hal_display.h"
#include "hal_touch.h"
#include "bsp_config.h"
#include <Arduino.h>
#include "esp_timer.h"
#include "esp_heap_caps.h"

static lv_display_t *lv_disp = NULL;
static lv_indev_t   *lv_indev = NULL;
static SemaphoreHandle_t lvgl_mutex = NULL;
static TaskHandle_t gui_task_handle = NULL;

static uint8_t *draw_buf_1 = NULL;
static uint8_t *draw_buf_2 = NULL;

/**
 * @brief Zero-overhead tick provider callback for LVGL 9.x
 */
static uint32_t lvgl_tick_provider(void) {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

/**
 * @brief FreeRTOS GUI Task - Pinned to Core 1
 */
static void gui_task_entry(void *pvParameters) {
    (void)pvParameters;
    Serial.printf("[GUI_TASK] Started on Core %d with priority %d\n", 
                  xPortGetCoreID(), uxTaskPriorityGet(NULL));

    while (1) {
        uint32_t time_till_next_ms = 10;

        if (hal_lvgl_lock(10)) {
            time_till_next_ms = lv_timer_handler();
            hal_lvgl_unlock();
        }

        /* Prevent task from monopolizing the core or sleeping too long */
        if (time_till_next_ms < 1) {
            time_till_next_ms = 1;
        } else if (time_till_next_ms > 30) {
            time_till_next_ms = 30;
        }

        vTaskDelay(pdMS_TO_TICKS(time_till_next_ms));
    }
}

bool hal_lvgl_init(void) {
    // 1. Create FreeRTOS Recursive Mutex for thread safety
    lvgl_mutex = xSemaphoreCreateRecursiveMutex();
    if (lvgl_mutex == NULL) {
        Serial.println("[HAL_LVGL] ERROR: Failed to create FreeRTOS recursive mutex!");
        return false;
    }

    // 2. Initialize LVGL 9 core engine
    lv_init();

    // 3. Register high-resolution tick callback
    lv_tick_set_cb(lvgl_tick_provider);

    // 4. Allocate Double Draw Buffers
    // Primary strategy: Internal SRAM with DMA capability for maximum SPI bandwidth
    size_t buffer_size_bytes = BSP_LCD_DRAW_BUF_BYTES;
    Serial.printf("[HAL_LVGL] Allocating double draw buffers: 2 x %u bytes (%u lines)\n", 
                  (unsigned int)buffer_size_bytes, BSP_LCD_BUFFER_LINES);

    draw_buf_1 = (uint8_t *)heap_caps_malloc(buffer_size_bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    draw_buf_2 = (uint8_t *)heap_caps_malloc(buffer_size_bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    if (!draw_buf_1 || !draw_buf_2) {
        Serial.println("[HAL_LVGL] WARN: SRAM DMA allocation failed. Falling back to PSRAM.");
        if (draw_buf_1) heap_caps_free(draw_buf_1);
        if (draw_buf_2) heap_caps_free(draw_buf_2);

        draw_buf_1 = (uint8_t *)heap_caps_malloc(buffer_size_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        draw_buf_2 = (uint8_t *)heap_caps_malloc(buffer_size_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

        if (!draw_buf_1 || !draw_buf_2) {
            Serial.println("[HAL_LVGL] FATAL: Out of memory for draw buffers!");
            return false;
        }
    }

    // 5. Initialize Display Hardware
    if (!hal_display_init()) {
        Serial.println("[HAL_LVGL] ERROR: Display hardware init failed!");
        return false;
    }

    // 6. Create & Configure LVGL 9 Display Object
    lv_disp = lv_display_create(BSP_LCD_H_RES, BSP_LCD_V_RES);
    if (!lv_disp) {
        Serial.println("[HAL_LVGL] ERROR: Failed to create LVGL display object!");
        return false;
    }

    lv_display_set_flush_cb(lv_disp, hal_display_flush_cb);
    lv_display_set_buffers(lv_disp, draw_buf_1, draw_buf_2, buffer_size_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);

    // 7. Initialize Touch Controller & Register LVGL Input Device
    if (hal_touch_init()) {
        lv_indev = lv_indev_create();
        if (lv_indev) {
            lv_indev_set_type(lv_indev, LV_INDEV_TYPE_POINTER);
            lv_indev_set_read_cb(lv_indev, hal_touch_read_cb);
            lv_indev_set_display(lv_indev, lv_disp);
            Serial.println("[HAL_LVGL] Touch input device registered with LVGL.");
        }
    } else {
        Serial.println("[HAL_LVGL] WARN: Touch init failed. Running in display-only mode.");
    }

    // 8. Launch GUI FreeRTOS Task pinned to Core 1
    BaseType_t task_status = xTaskCreatePinnedToCore(
        gui_task_entry,
        BSP_GUI_TASK_NAME,
        BSP_GUI_TASK_STACK_SIZE,
        NULL,
        BSP_GUI_TASK_PRIORITY,
        &gui_task_handle,
        BSP_GUI_TASK_CORE_ID
    );

    if (task_status != pdPASS) {
        Serial.println("[HAL_LVGL] ERROR: Failed to launch FreeRTOS GUI task!");
        return false;
    }

    Serial.println("[HAL_LVGL] Core initialization complete.");
    return true;
}

bool hal_lvgl_lock(uint32_t timeout_ms) {
    if (lvgl_mutex == NULL) return false;
    TickType_t timeout_ticks = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return (xSemaphoreTakeRecursive(lvgl_mutex, timeout_ticks) == pdTRUE);
}

void hal_lvgl_unlock(void) {
    if (lvgl_mutex != NULL) {
        xSemaphoreGiveRecursive(lvgl_mutex);
    }
}

lv_display_t* hal_lvgl_get_display(void) {
    return lv_disp;
}

lv_indev_t* hal_lvgl_get_indev(void) {
    return lv_indev;
}
