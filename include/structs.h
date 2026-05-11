#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>
#include <stdbool.h>

/**
 * UniControl Pro v5.1.0 - Data Structures
 */

// Sensor Types
typedef enum {
    SENSOR_RADAR,
    SENSOR_UDS
} sensor_type_t;

// Persistent Settings (NVS Blob)
typedef struct {
    uint32_t magic_word;        // 0xAABBCCDD
    float    warn_zone_m;       // Yellow zone threshold
    float    danger_zone_m;     // Red zone threshold
    float    vehicle_width_m;   // For screen rendering
    uint16_t can_baudrate;      // 250 or 500
    uint8_t  buzzer_vol;        // 0-100
    bool     fusion_mode;       // Radar + UDS combined
} __attribute__((packed)) sys_config_t;

// Tracked Object Structure
typedef struct {
    uint32_t      obj_id;       // ID from CAN
    sensor_type_t source;       // Radar or Ultrasonic
    float         x_pos;        // Lateral distance (meters)
    float         y_pos;        // Vertical distance (meters)
    float         velocity;     // Approach speed (m/s)
    uint32_t      last_seen;    // Tick count for timeout
} tracked_obj_t;

// Log Event Structure
typedef struct {
    uint32_t timestamp;
    char     event_type[16];
    char     message[64];
} log_event_t;

#endif // STRUCTS_H
