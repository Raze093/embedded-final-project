#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_system.h>
#include <esp_random.h>

#include <esp_rmaker_console.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>

#include <app_network.h>
#include <app_insights.h>

#include "app_priv.h"
#include "app_driver.c"

static const char *TAG = "app_main";

esp_rmaker_device_t *light_device;
esp_rmaker_device_t *temp_trigger_device;
esp_rmaker_device_t *humidity_trigger_device;

bool trigger_led_active = false;

void turn_off_led_if_triggered()
{
    if (trigger_led_active) {
        app_light_set_power(false);
        esp_rmaker_param_t *led_param = esp_rmaker_device_get_param_by_type(light_device, ESP_RMAKER_PARAM_POWER);
        if (led_param) {
            esp_rmaker_param_update_and_report(led_param, esp_rmaker_bool(false));
        }
        trigger_led_active = false;
    }
}

static void turn_on_led_via_trigger()
{
    app_light_set_power(true);
    esp_rmaker_param_t *led_param = esp_rmaker_device_get_param_by_type(light_device, ESP_RMAKER_PARAM_POWER);
    if (led_param) {
        esp_rmaker_param_update_and_report(led_param, esp_rmaker_bool(true));
    }
    trigger_led_active = true;
}

static void print_warning_and_alert(const char *type)
{
    if (strcmp(type, "Temperature") == 0) {
        int low = 30 + esp_random() % 2;
        int high = low + 1;
        ESP_LOGW(TAG, "Warning: High Temperature level!\n%d-%dC !", low, high);
        esp_rmaker_raise_alert("⚠️ High Temperature Detected!");
    } else {
        int low = 85 + esp_random() % 3;
        int high = low + 5;
        ESP_LOGW(TAG, "Warning: High Humidity level!\n%d-%d%% !", low, high);
        esp_rmaker_raise_alert("⚠️ High Humidity Detected!");
    }
}

static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
                          const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    const char *param_name = esp_rmaker_param_get_name(param);

    if (device == light_device && strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
        app_light_set_power(val.val.b);
        trigger_led_active = false;
    }

    if (device == temp_trigger_device && strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0 && val.val.b) {
        turn_on_led_via_trigger();
        print_warning_and_alert("Temperature");
        trigger_buzzer_event_temp();
    }

    if (device == humidity_trigger_device && strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0 && val.val.b) {
        turn_on_led_via_trigger();
        print_warning_and_alert("Humidity");
        trigger_buzzer_event_humidity();
    }

    esp_rmaker_param_update(param, val);
    return ESP_OK;
}

void physical_button_trigger_task(void *param)
{
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
        if (gpio_get_level(4) == 0) {
            esp_rmaker_param_t *param = esp_rmaker_device_get_param_by_type(temp_trigger_device, ESP_RMAKER_PARAM_POWER);
            if (param) {
                esp_rmaker_param_update_and_report(param, esp_rmaker_bool(true));
                turn_on_led_via_trigger();
                print_warning_and_alert("Temperature");
                trigger_buzzer_event_temp();
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        if (gpio_get_level(5) == 0) {
            esp_rmaker_param_t *param = esp_rmaker_device_get_param_by_type(humidity_trigger_device, ESP_RMAKER_PARAM_POWER);
            if (param) {
                esp_rmaker_param_update_and_report(param, esp_rmaker_bool(true));
                turn_on_led_via_trigger();
                print_warning_and_alert("Humidity");
                trigger_buzzer_event_humidity();
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

void app_main()
{
    esp_rmaker_console_init();
    app_driver_init();

    ESP_ERROR_CHECK(nvs_flash_init());
    app_network_init();

    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Device", "LED + Triggers");
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        abort();
    }

    light_device = esp_rmaker_lightbulb_device_create("Light", NULL, DEFAULT_POWER);
    esp_rmaker_device_add_cb(light_device, write_cb, NULL);
    esp_rmaker_device_add_param(light_device, esp_rmaker_brightness_param_create(ESP_RMAKER_DEF_BRIGHTNESS_NAME, DEFAULT_BRIGHTNESS));
    esp_rmaker_node_add_device(node, light_device);

    temp_trigger_device = esp_rmaker_switch_device_create("Temperature Trigger", NULL, false);
    esp_rmaker_device_add_cb(temp_trigger_device, write_cb, NULL);
    esp_rmaker_node_add_device(node, temp_trigger_device);

    humidity_trigger_device = esp_rmaker_switch_device_create("Humidity Trigger", NULL, false);
    esp_rmaker_device_add_cb(humidity_trigger_device, write_cb, NULL);
    esp_rmaker_node_add_device(node, humidity_trigger_device);

    esp_rmaker_ota_enable_default();
    esp_rmaker_start();

    ESP_ERROR_CHECK(app_network_start(POP_TYPE_RANDOM));

    xTaskCreate(physical_button_trigger_task, "physical_button_trigger_task", 4096, NULL, 5, NULL);
}
