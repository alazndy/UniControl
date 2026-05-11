#include "settings.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include "esp_log.h"

static const char* TAG = "SETTINGS";
RadarSettings settings;

void set_default_settings() {
    settings.warning_zone = 5.0f;
    settings.danger_zone = 2.0f;
    settings.vehicle_width = 2.5f;
    settings.side_margin = 1.0f;
    settings.max_width = settings.vehicle_width + (settings.side_margin * 2);
    settings.auto_zoom = true;
    settings.audio_alarm = true;
    settings.max_objects = 6;
    settings.io_link_1 = false;
    settings.sensor_mode = 0;
    settings.bs_id_310_loc = 0;
    settings.bs_id_320_loc = 1;
    settings.uds_s1_loc = 0;
    settings.uds_s2_loc = 0;
    settings.uds_s3_loc = 0;
    settings.uds_s4_loc = 0;
    settings.dbg_can = false;
    settings.dbg_nextion = false;
    settings.dbg_radar = false;
    settings.dbg_sd = false;
    settings.dbg_sys = false;
    settings.sniffer_mode = false;
}

void init_settings() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    load_settings();
}

void load_settings() {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS acilamadi, varsayilan ayarlar yukleniyor.");
        set_default_settings();
        return;
    }

    size_t required_size = sizeof(RadarSettings);
    err = nvs_get_blob(my_handle, "rad_settings", &settings, &required_size);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Ayarlar bulunamadi, varsayilan ayarlar yukleniyor.");
        set_default_settings();
    } else {
        ESP_LOGI(TAG, "Ayarlar basariyla yuklendi.");
    }
    nvs_close(my_handle);
}

void save_settings() {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        err = nvs_set_blob(my_handle, "rad_settings", &settings, sizeof(RadarSettings));
        if (err == ESP_OK) {
            nvs_commit(my_handle);
            ESP_LOGI(TAG, "Ayarlar kaydedildi.");
        } else {
            ESP_LOGE(TAG, "Ayarlar kaydedilemedi.");
        }
        nvs_close(my_handle);
    } else {
        ESP_LOGE(TAG, "NVS kaydetme icin acilamadi.");
    }
}
