/**
 * @file hal_touch.cpp
 * @brief Capacitive Touch driver implementation with hardware probe
 */

#include "hal_touch.h"
#include "bsp_config.h"
#include <Arduino.h>
#include <Wire.h>
static bool touch_initialized = false;
static uint8_t touch_addr = 0;
static uint16_t controller_max_x = BSP_LCD_H_RES;
static uint16_t controller_max_y = BSP_LCD_V_RES;
static uint8_t touch_log_budget = 8;

static bool gt911_read(uint16_t reg, uint8_t *data, size_t len) {
    Wire.beginTransmission(touch_addr);
    Wire.write((uint8_t)(reg >> 8));
    Wire.write((uint8_t)reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(touch_addr, len) != len) return false;
    for (size_t i = 0; i < len; ++i) data[i] = Wire.read();
    return true;
}

static void gt911_clear_status(void) {
    Wire.beginTransmission(touch_addr);
    Wire.write(0x81);
    Wire.write(0x4E);
    Wire.write(0);
    Wire.endTransmission();
}

bool hal_touch_init(void) {
    Wire.begin(BSP_TOUCH_I2C_SDA_PIN, BSP_TOUCH_I2C_SCL_PIN, (uint32_t)BSP_TOUCH_I2C_FREQ_HZ);
    
    // Probe I2C bus to see if a touch controller responds
    bool touch_found = false;
    uint8_t touch_addrs[] = {BSP_TOUCH_GT911_ADDR_1, BSP_TOUCH_GT911_ADDR_2, 0x38, 0x15};
    for (uint8_t addr : touch_addrs) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[HAL_TOUCH] Found touch device at I2C address 0x%02X\n", addr);
            touch_found = true;
            touch_addr = addr;
            break;
        }
    }

    if (touch_found) {
        // The controller is already hardware-reset by the board. Poll it directly;
        // the generic TAMC driver assumes dedicated INT/RST GPIOs that MAX35 does not expose.
        touch_initialized = true;
        gt911_clear_status();
        uint8_t product_id[4] = {};
        uint8_t resolution[4] = {};
        gt911_read(0x8140, product_id, sizeof(product_id));
        if (gt911_read(0x8048, resolution, sizeof(resolution))) {
            controller_max_x = resolution[0] | (resolution[1] << 8);
            controller_max_y = resolution[2] | (resolution[3] << 8);
        }
        Serial.printf("[HAL_TOUCH] GT911 polling enabled at 0x%02X on SDA=%d, SCL=%d\n",
                      touch_addr, BSP_TOUCH_I2C_SDA_PIN, BSP_TOUCH_I2C_SCL_PIN);
        Serial.printf("[HAL_TOUCH] Product %.4s, controller resolution %ux%u\n",
                      product_id, controller_max_x, controller_max_y);
    } else {
        Serial.println("[HAL_TOUCH] No capacitive touch detected on I2C. Touch input disabled.");
    }
    return touch_initialized;
}

void hal_touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    LV_UNUSED(indev);

    if (!touch_initialized) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    uint8_t status = 0;
    if (!gt911_read(0x814E, &status, 1) || !(status & 0x80) || !(status & 0x0F)) {
        data->state = LV_INDEV_STATE_RELEASED;
        if (status & 0x80) gt911_clear_status();
        return;
    }

    uint8_t point[8] = {};
    if (gt911_read(0x814F, point, sizeof(point))) {
        int32_t raw_x = point[1] | (point[2] << 8);
        int32_t raw_y = point[3] | (point[4] << 8);
        int32_t touch_x;
        int32_t touch_y;
        // MAX35's GT911 reports coordinates in the same portrait orientation
        // as the ST7796 panel. Inverting both axes made top-left touches land
        // at bottom-right and reversed vertical scrolling.
        if (controller_max_x == BSP_LCD_V_RES && controller_max_y == BSP_LCD_H_RES) {
            touch_x = raw_y;
            touch_y = raw_x;
        } else {
            touch_x = raw_x;
            touch_y = raw_y;
        }

        // Clamp coordinates within screen bounds
        if (touch_x < 0) touch_x = 0;
        if (touch_x >= BSP_LCD_H_RES) touch_x = BSP_LCD_H_RES - 1;
        if (touch_y < 0) touch_y = 0;
        if (touch_y >= BSP_LCD_V_RES) touch_y = BSP_LCD_V_RES - 1;

        data->point.x = touch_x;
        data->point.y = touch_y;
        data->state = LV_INDEV_STATE_PRESSED;
        if (touch_log_budget) {
            Serial.printf("[HAL_TOUCH] raw=(%ld,%ld) mapped=(%ld,%ld)\n",
                          (long)raw_x, (long)raw_y, (long)touch_x, (long)touch_y);
            --touch_log_budget;
        }
        gt911_clear_status();
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
