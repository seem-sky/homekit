#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include "wifi.h"
#include "queue.h"


#define TRIGGER_LATCH_MS 1000
#define TRIGGER_GPIO 2
#define SENSE_GPIO 3

typedef enum _current_door_state current_door_state_t;
enum _current_door_state {
    CURRENT_DOOR_STATE_OPEN = 0,
    CURRENT_DOOR_STATE_CLOSED = 1,
    CURRENT_DOOR_STATE_OPENING = 2,
    CURRENT_DOOR_STATE_CLOSING = 3,
    CURRENT_DOOR_STATE_STOPPED = 4
};

static void wifi_init() {
    struct sdk_station_config wifi_config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASSWORD,
    };

    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&wifi_config);
    sdk_wifi_station_connect();
}

homekit_characteristic_t current_door_state;

void identify_task(void *_args) {
    for (int i=0; i<6; i++) {
        // Should light up the GPIO1 Pin (Connected to blue LED)
        printf("Identifying...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void led_identify(homekit_value_t _value) {
    printf("LED identify\n");
    xTaskCreate(identify_task, "LED identify\n", 256, NULL, 2, NULL);
}


void trigger_init(void) {
    gpio_enable(TRIGGER_GPIO, GPIO_OUTPUT);
    gpio_write(TRIGGER_GPIO, 1);
}

void trigger_task(void *_args) {
    gpio_write(TRIGGER_GPIO, 0);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    gpio_write(TRIGGER_GPIO, 1);
    vTaskDelete(NULL);
}

static QueueHandle_t tsqueue;

current_door_state_t get_door_state() {
    // A high value means the door is open.
    uint8_t sense_val = gpio_read(SENSE_GPIO);
    if (sense_val == 0) {
        return CURRENT_DOOR_STATE_CLOSED;
    } else {
        return CURRENT_DOOR_STATE_OPEN;
    }
}

void sense_intr_handler(uint8_t gpio_num) {
    uint32_t now = xTaskGetTickCountFromISR();
    xQueueSendToBackFromISR(tsqueue, &now, NULL);
}

void sense_intr_task(void *pvParameters) {
    QueueHandle_t *tsqueue = (QueueHandle_t *)pvParameters;
    gpio_set_interrupt(SENSE_GPIO, GPIO_INTTYPE_EDGE_ANY, sense_intr_handler);

    uint32_t last = 0;
    while(1) {
        uint32_t button_ts;
        xQueueReceive(*tsqueue, &button_ts, portMAX_DELAY);
        button_ts *= portTICK_PERIOD_MS;
        if(last < button_ts-200) {
            current_door_state_t door_state = get_door_state();
            homekit_characteristic_notify(&current_door_state, HOMEKIT_UINT8(door_state));
            last = button_ts;
        }
    }
}

void sense_init() {
    tsqueue = xQueueCreate(2, sizeof(uint32_t));
    gpio_enable(SENSE_GPIO, GPIO_INPUT);
    xTaskCreate(sense_intr_task, "senseTask", 256, &tsqueue, 2, NULL);
}



homekit_value_t get_current_door_state() {
    return HOMEKIT_UINT8(get_door_state());
}


void set_target_door_state(homekit_value_t value) {
    current_door_state_t current_state = get_door_state();

    printf("Setting target door state to %d\n", value.int_value);
    printf("Current state is %d\n", current_state);

    if (current_state == CURRENT_DOOR_STATE_OPEN) {
        homekit_characteristic_notify(&current_door_state,
            HOMEKIT_UINT8(CURRENT_DOOR_STATE_CLOSING));
    } else {
        homekit_characteristic_notify(&current_door_state,
            HOMEKIT_UINT8(CURRENT_DOOR_STATE_OPENING));
    }

    xTaskCreate(trigger_task, "trigger", 128, NULL, 2, NULL);
}

homekit_characteristic_t current_door_state = HOMEKIT_CHARACTERISTIC_(CURRENT_DOOR_STATE, 0,
    .getter=get_current_door_state
);
homekit_characteristic_t target_door_state = HOMEKIT_CHARACTERISTIC_(TARGET_DOOR_STATE, 0,
    .setter=set_target_door_state
);

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_garage, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Garage Door"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "nickw"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "1B2770724F43"),
            HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266-01-door"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, led_identify),
            NULL
        }),
        HOMEKIT_SERVICE(GARAGE_DOOR_OPENER, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(OBSTRUCTION_DETECTED, false),
            &target_door_state,
            &current_door_state,
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111"
};

void user_init(void) {
    uart_set_baud(0, 115200);

    trigger_init();
    sense_init();
    wifi_init();

    homekit_server_init(&config);
}
