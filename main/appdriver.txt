#include <sdkconfig.h>
#include <esp_log.h>
#include <iot_button.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <app_reset.h>
#include "app_priv.h"

#define LED_GPIO                2
#define BUZZER_GPIO             3
#define SWITCH_GPIO             4
#define HUMIDITY_SWITCH_GPIO    5

static uint16_t g_led_brightness = DEFAULT_BRIGHTNESS;
static bool g_led_power = DEFAULT_POWER;

typedef enum {
    TRIGGER_TEMP,
    TRIGGER_HUMIDITY
} trigger_event_t;

static QueueHandle_t trigger_queue;

void buzzer_event_task_temp(void *pvParameters);
void buzzer_event_task_humidity(void *pvParameters);

void trigger_buzzer_event_temp()
{
    xTaskCreate(buzzer_event_task_temp, "buzzer_event_task_temp", 2048, NULL, 5, NULL);
}

void trigger_buzzer_event_humidity()
{
    xTaskCreate(buzzer_event_task_humidity, "buzzer_event_task_humidity", 2048, NULL, 5, NULL);
}

esp_err_t app_light_set_power(bool power)
{
    g_led_power = power;
    gpio_set_level(LED_GPIO, power ? 1 : 0);
    return ESP_OK;
}

esp_err_t app_light_set_brightness(uint16_t brightness)
{
    g_led_brightness = brightness;
    return app_light_set_power(g_led_power);
}

void IRAM_ATTR gpio_switch_isr_handler(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    trigger_event_t evt = TRIGGER_TEMP;
    xQueueSendFromISR(trigger_queue, &evt, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void IRAM_ATTR gpio_humidity_switch_isr_handler(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    trigger_event_t evt = TRIGGER_HUMIDITY;
    xQueueSendFromISR(trigger_queue, &evt, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void trigger_handler_task(void *pvParameters)
{
    trigger_event_t event;
    while (xQueueReceive(trigger_queue, &event, portMAX_DELAY)) {
        if (event == TRIGGER_TEMP) {
            esp_rmaker_param_t *param = esp_rmaker_device_get_param_by_type(temp_trigger_device, ESP_RMAKER_PARAM_POWER);
            if (param) esp_rmaker_param_update_and_report(param, esp_rmaker_bool(true));
            trigger_buzzer_event_temp();
        } else if (event == TRIGGER_HUMIDITY) {
            esp_rmaker_param_t *param = esp_rmaker_device_get_param_by_type(humidity_trigger_device, ESP_RMAKER_PARAM_POWER);
            if (param) esp_rmaker_param_update_and_report(param, esp_rmaker_bool(true));
            trigger_buzzer_event_humidity();
        }

        app_light_set_power(true);
        esp_rmaker_param_t *led_param = esp_rmaker_device_get_param_by_type(light_device, ESP_RMAKER_PARAM_POWER);
        if (led_param) esp_rmaker_param_update_and_report(led_param, esp_rmaker_bool(true));
        trigger_led_active = true;
    }
}

void buzzer_event_task_temp(void *pvParameters)
{
    for (int i = 0; i < 20; i++) {
        gpio_set_level(BUZZER_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(125));
        gpio_set_level(BUZZER_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(125));
    }

    esp_rmaker_param_t *param = esp_rmaker_device_get_param_by_type(temp_trigger_device, ESP_RMAKER_PARAM_POWER);
    if (param) esp_rmaker_param_update_and_report(param, esp_rmaker_bool(false));

    turn_off_led_if_triggered();
    vTaskDelete(NULL);
}

void buzzer_event_task_humidity(void *pvParameters)
{
    for (int i = 0; i < 5; i++) {
        gpio_set_level(BUZZER_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(BUZZER_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    esp_rmaker_param_t *param = esp_rmaker_device_get_param_by_type(humidity_trigger_device, ESP_RMAKER_PARAM_POWER);
    if (param) esp_rmaker_param_update_and_report(param, esp_rmaker_bool(false));

    turn_off_led_if_triggered();
    vTaskDelete(NULL);
}

esp_err_t app_light_init(void)
{
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, g_led_power);

    gpio_reset_pin(BUZZER_GPIO);
    gpio_set_direction(BUZZER_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER_GPIO, 0);

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << SWITCH_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&io_conf);

    gpio_config_t io_conf2 = {
        .pin_bit_mask = (1ULL << HUMIDITY_SWITCH_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&io_conf2);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(SWITCH_GPIO, gpio_switch_isr_handler, NULL);
    gpio_isr_handler_add(HUMIDITY_SWITCH_GPIO, gpio_humidity_switch_isr_handler, NULL);

    trigger_queue = xQueueCreate(5, sizeof(trigger_event_t));
    xTaskCreate(trigger_handler_task, "trigger_handler_task", 4096, NULL, 10, NULL);

    return ESP_OK;
}

void app_driver_init()
{
    app_light_init();
}
