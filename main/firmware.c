#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "protocol_examples_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

static const char *TAG = "firmware";
static const int RX_BUF_SIZE = 1024;
static esp_mqtt_client_handle_t client = NULL;

#define TXD_PIN (GPIO_NUM_4)
#define RXD_PIN (GPIO_NUM_5)

#define ARDUINO_FAN_PIN (GPIO_NUM_2)
#define STM32_FAN_PIN (GPIO_NUM_4)

void setup_device_fan_gpio(int fan_pin)
{
    gpio_reset_pin(fan_pin);
    gpio_set_direction(fan_pin, GPIO_MODE_OUTPUT);
}

void setup_uart(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void setup_mqtt_client(void)
{
    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
    * Read "Establishing Wi-Fi or Ethernet Connection" section in
    * examples/protocols/README.md for more information about this function.
    */
    ESP_ERROR_CHECK(example_connect());

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
    };
    client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_start(client);
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        esp_mqtt_client_subscribe(client, "/fans/+/status", 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        break;
    case MQTT_EVENT_SUBSCRIBED:
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        break;
    case MQTT_EVENT_PUBLISHED:
        break;
    case MQTT_EVENT_DATA:
        char device_name[64] = {0};
        const char *start = event->topic + 6; // topic format: /fans/<device_name>/status ... skip "/fans/"
        const char *end = memchr(start, '/', event->topic_len - 6);
        if (end && (end - start) < sizeof(device_name)) {
            memcpy(device_name, start, end - start);
            device_name[end - start] = '\0';
        }
        if (strlen(device_name) > 0) {
            enum devices device = -1;
            if (strcmp(device_name, "arduino_uno_r3") == 0) {
                device = arduino_uno_r3;
            }
            if (strcmp(device_name, "stm32l476rg") == 0) {
                device = stm32l476rg;
            }
            char *data = (char *)malloc(event->data_len + 1);
            if (data) {
                memcpy(data, event->data, event->data_len);
                data[event->data_len] = '\0';
            }
            switch (device) {
                case arduino_uno_r3:
                    printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
                    printf("DATA=%s\r\n", data);
                    if(strcmp(data, "on") == 0) {
                        ESP_LOGI(TAG, "Turning on Arduino fan");
                        gpio_set_level(ARDUINO_FAN_PIN, 1);
                    }
                    if(strcmp(data, "off") == 0) {
                        ESP_LOGI(TAG, "Turning off Arduino fan");
                        gpio_set_level(ARDUINO_FAN_PIN, 0);
                    }
                    break;
                case stm32l476rg:
                    printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
                    printf("DATA=%s\r\n", data);
                    if(strcmp(data, "on") == 0) {
                        ESP_LOGI(TAG, "Turning on stm32l476rg fan");
                        gpio_set_level(STM32_FAN_PIN, 1);
                    }
                    if(strcmp(data, "off") == 0) {
                        ESP_LOGI(TAG, "Turning off stm32l476rg fan");
                        gpio_set_level(STM32_FAN_PIN, 0);
                    }
                    break;
                default:
                    ESP_LOGW(TAG, "Unknown device: %s", device_name);
                    break;
            }
            free(data);
        }
        break;
    case MQTT_EVENT_ERROR:
        break;
    default:
        break;
    }
}

void uart_arduino_rx_task(void *arg)
{
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE + 1);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            esp_mqtt_client_publish(client, "/devices/arduino_uno_r3/temperature", (char *)data, 0, 0, 0);
        }
    }
    free(data);
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    setup_mqtt_client();
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    if (client == NULL)
    {
        ESP_LOGE(TAG, "Failed to create MQTT client");
        esp_restart();
    }
    ESP_LOGI(TAG, "MQTT client created successfully");
    ESP_LOGI(TAG, "Set up UART");
    setup_uart();
    ESP_LOGI(TAG, "Creating UART RX task");
    xTaskCreate(uart_arduino_rx_task, "uart_arduino_rx_task", 1024 * 2, NULL, configMAX_PRIORITIES - 1, NULL);

    setup_device_fan_gpio(ARDUINO_FAN_PIN);
    setup_device_fan_gpio(STM32_FAN_PIN);
    while (1)
    {
        
    }
}
