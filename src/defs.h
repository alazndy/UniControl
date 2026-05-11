#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>
#include <stdbool.h>

// Pin Definitions
#define PIN_CAN_TX      5
#define PIN_CAN_RX      4
#define PIN_NEXTION_TX  17
#define PIN_NEXTION_RX  18
#define PIN_SD_CS       10
#define PIN_SD_MOSI     11
#define PIN_SD_CLK      12
#define PIN_SD_MISO     13
#define PIN_RTC_SDA     6
#define PIN_RTC_SCL     7
#define PIN_BUZZER      8
#define PIN_IN1         1
#define PIN_IN2         2
#define PIN_IN3         3
#define PIN_MOS1        38
#define PIN_MOS2        39
#define PIN_MOS3        40
#define PIN_REL1        14
#define PIN_REL2        21
#define PIN_REL3        47

// Structs
typedef struct {
    float warning_zone;
    float danger_zone;
    float vehicle_width;
    float side_margin;
    float max_width;
    bool auto_zoom;
    bool audio_alarm;
    int max_objects;
    bool io_link_1;
    int sensor_mode; // 0=BS9100, 1=UDS, 2=Hybrid
    int bs_id_310_loc;
    int bs_id_320_loc;
    int uds_s1_loc;
    int uds_s2_loc;
    int uds_s3_loc;
    int uds_s4_loc;
    bool dbg_can;
    bool dbg_nextion;
    bool dbg_radar;
    bool dbg_sd;
    bool dbg_sys;
    bool sniffer_mode;
} RadarSettings;

typedef struct {
    float x;
    float y;
    float distance;
    bool active;
} RadarObject;

// Globals
extern RadarSettings settings;
extern RadarObject frontObjs[6];
extern RadarObject rearObjs[6];
extern bool buzzerActive;
extern int beepInterval;

#endif // DEFS_H
