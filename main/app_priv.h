#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include <esp_rmaker_core.h>

#define DEFAULT_POWER       0
#define DEFAULT_BRIGHTNESS  90

extern esp_rmaker_device_t *light_device;
extern esp_rmaker_device_t *temp_trigger_device;
extern esp_rmaker_device_t *humidity_trigger_device;

extern bool trigger_led_active;

void app_driver_init(void);
esp_err_t app_light_set_power(bool power);
esp_err_t app_light_set_brightness(uint16_t brightness);

void trigger_buzzer_event_once(void);
void turn_off_led_if_triggered(void);
