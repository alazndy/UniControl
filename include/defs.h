#ifndef DEFS_H
#define DEFS_H

/**
 * UniControl Pro v5.1.0 - Global Definitions & Pin Map
 * Platform: ESP32-S3
 */

// --- GPIO PIN MAPPING ---

// Isolated Inputs (Optocouplers)
#define PIN_IN1_OPTO        1   // Right Signal Input
#define PIN_IN2_OPTO        2   // Left Signal Input
#define PIN_IN3_OPTO        3   // Reverse Gear Input

// CAN Bus (TWAI)
#define PIN_CAN_RX          4   // SN65HVD230 CRX
#define PIN_CAN_TX          5   // SN65HVD230 CTX

// I2C (RTC DS1307)
#define PIN_I2C_SDA         6
#define PIN_I2C_SCL         7

// Audio
#define PIN_BUZZER          8   // Active Buzzer (+)

// SPI (SD Card)
#define PIN_SPI_CS          10
#define PIN_SPI_MOSI        11
#define PIN_SPI_CLK         12
#define PIN_SPI_MISO        13

// HMI (Nextion Display)
#define PIN_HMI_RX          17  // UART1 RX
#define PIN_HMI_TX          18  // UART1 TX

// High-Side Outputs (MOSFETs)
#define PIN_OUT1_HS         38  // Horn / Main Relay
#define PIN_OUT2_HS         39  // Right Blind Spot Alert
#define PIN_OUT3_HS         40  // Left Blind Spot Alert

// --- SYSTEM CONSTANTS ---
#define CAN_BAUDRATE_DEFAULT 250000
#define HMI_BAUDRATE        115200
#define LOG_QUEUE_LEN       10
#define MAX_TRACKED_OBJECTS 12

// --- RTOS TASK PRIORITIES ---
#define PRIO_TASK_CAN       5
#define PRIO_TASK_HMI       4
#define PRIO_TASK_BUZZER    3
#define PRIO_TASK_IO        2
#define PRIO_TASK_SYS       1

#endif // DEFS_H
