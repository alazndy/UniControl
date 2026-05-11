#include "settings.h"
#include "hal.h"
#include "logic.h"
#include "web.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"

static const char* TAG = "MAIN";

void app_main() {
    ESP_LOGI(TAG, "UniControl Pro v5.1.0 Baslatiliyor...");

    // Cekirdek Modulleri Baslat
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Sistemi Hazirla
    init_settings();
    hal_init();
    web_init();
    
    // Gorevleri ve is mantigini (FreeRTOS) baslat
    logic_init();

    ESP_LOGI(TAG, "Sistem basariyla hazirlandi. Gorevler calisiyor.");
}
