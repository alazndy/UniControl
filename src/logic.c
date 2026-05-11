#include "logic.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"
#include "driver/uart.h"
#include "hal.h"
#include "settings.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char* TAG = "LOGIC";

RadarObject frontObjs[6];
RadarObject rearObjs[6];
bool buzzerActive = false;
int beepInterval = 0;

void update_vehicle_display(float value) {
    int jSolVal = (int)value; // Yüzdelik değer hesaplama
    int jSagVal = 100 - jSolVal;
    
    char buf[64];
    int len = snprintf(buf, sizeof(buf), "jSol.val=%d", jSolVal);
    buf[len] = 0xFF; buf[len+1] = 0xFF; buf[len+2] = 0xFF;
    uart_write_bytes(UART_NUM_1, buf, len+3);
    
    len = snprintf(buf, sizeof(buf), "jSag.val=%d", jSagVal);
    buf[len] = 0xFF; buf[len+1] = 0xFF; buf[len+2] = 0xFF;
    uart_write_bytes(UART_NUM_1, buf, len+3);
}

void task_radar(void *pvParameters) {
    TickType_t last_keepalive = xTaskGetTickCount();
    while (1) {
        if (settings.sensor_mode == 1 || settings.sensor_mode == 2) {
            TickType_t now = xTaskGetTickCount();
            if ((now - last_keepalive) * portTICK_PERIOD_MS >= 100) {
                twai_message_t msg;
                msg.identifier = 0x18FF5A54;
                msg.extd = 1;
                msg.rtr = 0;
                msg.data_length_code = 8;
                memset(msg.data, 0, 8);
                msg.data[0] = 0x01;
                twai_transmit(&msg, 0);
                last_keepalive = now;
            }
        }

        twai_message_t rx_msg;
        if (twai_receive(&rx_msg, pdMS_TO_TICKS(10)) == ESP_OK) {
            float min_dist = 999.0f;
            if (!rx_msg.extd) {
                if (rx_msg.identifier == 0x310 || rx_msg.identifier == 0x320) {
                    float x = (float)rx_msg.data[2] / 10.0f;
                    float y = ((float)rx_msg.data[3] / 10.0f) - 8.0f;
                    float dist = y; // Basitleştirilmiş mesafe
                    if (dist < min_dist) min_dist = dist;
                    if (settings.dbg_can) {
                        ESP_LOGI(TAG, "CAN Std ID: 0x%lx, X: %.1f, Y: %.1f", rx_msg.identifier, x, y);
                    }
                }
            } else {
                if ((rx_msg.identifier & 0x1FFFFFFF) == 0x18FF5B80 || 
                    (rx_msg.identifier & 0x1FFFFFFF) == 0x18FF5B81) {
                    float d1 = rx_msg.data[0] * 0.1f;
                    float d2 = rx_msg.data[1] * 0.1f;
                    float d3 = rx_msg.data[2] * 0.1f;
                    float d4 = rx_msg.data[3] * 0.1f;
                    if (d1 < min_dist) min_dist = d1;
                    if (d2 < min_dist) min_dist = d2;
                    if (d3 < min_dist) min_dist = d3;
                    if (d4 < min_dist) min_dist = d4;
                    if (settings.dbg_can) {
                        ESP_LOGI(TAG, "CAN Ext UDS Dists: %.1f %.1f %.1f %.1f", d1, d2, d3, d4);
                    }
                }
            }

            if (min_dist < settings.danger_zone) {
                buzzerActive = true;
                beepInterval = 100;
                log_alarm_to_sd();
            } else if (min_dist < settings.warning_zone) {
                buzzerActive = true;
                beepInterval = 500;
            } else {
                buzzerActive = false;
            }
        }
    }
}

void task_nextion(void *pvParameters) {
    uint8_t data[128];
    char str_data[128];
    while (1) {
        int rxBytes = uart_read_bytes(UART_NUM_1, data, sizeof(data) - 1, pdMS_TO_TICKS(100));
        if (rxBytes > 0) {
            for (int i = 0; i < rxBytes; i++) {
                if (data[i] == 0x00 || data[i] == 0xFF) {
                    str_data[i] = ' '; // Temizle (Sanitize)
                } else {
                    str_data[i] = data[i];
                }
            }
            str_data[rxBytes] = '\0';
            
            if (settings.dbg_nextion) {
                ESP_LOGI(TAG, "Nextion RX: %s", str_data);
            }

            if (strstr(str_data, "SAVE1") != NULL) {
                save_settings();
            } else if (strstr(str_data, "SAVE2") != NULL || strstr(str_data, "REFRESH") != NULL) {
                update_vehicle_display(10.0f);
            } else if (strstr(str_data, "SAVE3") != NULL) {
                save_settings();
            } else if (strstr(str_data, "RESETALL") != NULL) {
                set_default_settings();
                save_settings();
            }
        }
    }
}

void task_buzzer(void *pvParameters) {
    bool state = false;
    while (1) {
        if (buzzerActive && settings.audio_alarm) {
            state = !state;
            gpio_set_level(PIN_BUZZER, state);
            vTaskDelay(pdMS_TO_TICKS(beepInterval));
        } else {
            gpio_set_level(PIN_BUZZER, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void task_io(void *pvParameters) {
    while (1) {
        if (settings.io_link_1) {
            int in1 = get_input(1); // Pull-up olduğu için aktif = 0
            if (in1 == 0) {
                set_output(1, 0, true);
            } else {
                set_output(1, 0, false);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void task_sys(void *pvParameters) {
    while (1) {
        ESP_LOGI(TAG, "Free Heap: %lu bytes", (unsigned long)esp_get_free_heap_size());
        ESP_LOGI(TAG, "Uptime: %lu ms", (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS));
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void logic_init() {
    xTaskCreate(task_radar, "radar", 4096, NULL, 5, NULL);
    xTaskCreate(task_nextion, "nextion", 4096, NULL, 4, NULL);
    xTaskCreate(task_buzzer, "buzzer", 2048, NULL, 3, NULL);
    xTaskCreate(task_io, "io", 2048, NULL, 3, NULL);
    xTaskCreate(task_sys, "sys", 2048, NULL, 1, NULL);
}
