#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "defs.h"
#include "structs.h"

static const char *TAG = "UniControl_Main";

// Global Handles
SemaphoreHandle_t xObjListMutex = NULL;
QueueHandle_t xLogQueue = NULL;
tracked_obj_t global_obj_list[MAX_TRACKED_OBJECTS];

// Task Prototypes
void task_can_parser(void *pvParameters);
void task_hmi_render(void *pvParameters);
void task_io_logic(void *pvParameters);
void task_sys_monitor(void *pvParameters);

void app_main(void) {
    ESP_LOGI(TAG, "UniControl Pro v5.1.0 Starting...");

    // 1. Initialize Mutexes & Queues
    xObjListMutex = xSemaphoreCreateMutex();
    xLogQueue = xQueueCreate(LOG_QUEUE_LEN, sizeof(log_event_t));

    if (xObjListMutex == NULL || xLogQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create IPC objects!");
        return;
    }

    // 2. Initialize Hardware (Placeholder for HAL calls)
    ESP_LOGI(TAG, "Initializing HAL...");

    // 3. Create Tasks
    xTaskCreatePinnedToCore(task_can_parser, "CAN_Parser", 4096, NULL, PRIO_TASK_CAN, NULL, 1);
    xTaskCreatePinnedToCore(task_hmi_render, "HMI_Render", 4096, NULL, PRIO_TASK_HMI, NULL, 1);
    xTaskCreatePinnedToCore(task_io_logic,  "IO_Logic",   2048, NULL, PRIO_TASK_IO,  NULL, 1);
    xTaskCreatePinnedToCore(task_sys_monitor, "SYS_Mon",  2048, NULL, PRIO_TASK_SYS, NULL, 0);

    ESP_LOGI(TAG, "Scheduler started.");
}

// Task Stubs
void task_can_parser(void *pvParameters) {
    while(1) {
        // TODO: Implement TWAI receive
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void task_hmi_render(void *pvParameters) {
    while(1) {
        // TODO: Implement Nextion UART send
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void task_io_logic(void *pvParameters) {
    while(1) {
        // TODO: Implement IO read/write
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void task_sys_monitor(void *pvParameters) {
    while(1) {
        ESP_LOGD(TAG, "System Health Check...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
