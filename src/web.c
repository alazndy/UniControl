#include "web.h"
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "nvs_flash.h"
#include "settings.h"
#include "hal.h"
#include "logic.h"
#include <string.h>

static const char *TAG = "WEB";

static esp_err_t get_handler(httpd_req_t *req) {
    const char* html_chunk1 = "<html><head><title>Dashboard</title></head><body><h1>RADAR_SISTEM_PRO Dashboard</h1>"
                              "<form action=\"/save\" method=\"POST\">"
                              "Warning Zone: <input type=\"text\" name=\"warning_zone\" value=\"\"><br>"
                              "Danger Zone: <input type=\"text\" name=\"danger_zone\" value=\"\"><br>"
                              "<input type=\"submit\" value=\"Save\"></form><hr>";
    const char* html_chunk2 = "<h2>I/O Control</h2>"
                              "<a href=\"/tog?m=1\">Toggle MOS1</a> | "
                              "<a href=\"/tog?m=2\">Toggle MOS2</a> | "
                              "<a href=\"/tog?m=3\">Toggle MOS3</a><br>"
                              "<a href=\"/tog?r=1\">Toggle REL1</a> | "
                              "<a href=\"/tog?r=2\">Toggle REL2</a> | "
                              "<a href=\"/tog?r=3\">Toggle REL3</a><hr>"
                              "<h2>Logs & OTA</h2>"
                              "<a href=\"/dl\">Download Log</a><br><br>"
                              "<form action=\"/up\" method=\"POST\" enctype=\"multipart/form-data\">"
                              "OTA Update: <input type=\"file\" name=\"firmware\">"
                              "<input type=\"submit\" value=\"Upload\"></form></body></html>";
    
    httpd_resp_send_chunk(req, html_chunk1, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, html_chunk2, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t save_post_handler(httpd_req_t *req) {
    char buf[200];
    int ret, remaining = req->content_len;
    if (remaining > sizeof(buf) - 1) remaining = sizeof(buf) - 1;
    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';
    
    // Gelen verileri ayıkla (Basit demo, URL-encoded parametreleri parse ederiz)
    save_settings();
    update_vehicle_display(10.0f); // Nextion ekranını da güncelle

    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t tog_handler(httpd_req_t *req) {
    char buf[32];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        char val[4];
        if (httpd_query_key_value(buf, "m", val, sizeof(val)) == ESP_OK) {
            int m_idx = atoi(val);
            static bool m_state[3] = {false, false, false};
            if(m_idx >= 1 && m_idx <= 3) {
                m_state[m_idx-1] = !m_state[m_idx-1];
                set_output(m_idx, 0, m_state[m_idx-1]);
            }
        } else if (httpd_query_key_value(buf, "r", val, sizeof(val)) == ESP_OK) {
            int r_idx = atoi(val);
            static bool r_state[3] = {false, false, false};
            if(r_idx >= 1 && r_idx <= 3) {
                r_state[r_idx-1] = !r_state[r_idx-1];
                set_output(0, r_idx, r_state[r_idx-1]);
            }
        }
    }
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t dl_handler(httpd_req_t *req) {
    FILE* f = fopen("/sdcard/log.txt", "r");
    if (!f) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Log dosyasi bulunamadi.");
        return ESP_FAIL;
    }
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=\"log.txt\"");
    char chunk[1024];
    size_t read_bytes;
    while ((read_bytes = fread(chunk, 1, sizeof(chunk), f)) > 0) {
        httpd_resp_send_chunk(req, chunk, read_bytes);
    }
    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t ota_post_handler(httpd_req_t *req) {
    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) {
        ESP_LOGE(TAG, "Guncellenecek OTA bolumu bulunamadi.");
        return ESP_FAIL;
    }

    esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    if (err != ESP_OK) return ESP_FAIL;

    char buf[1024];
    int remaining = req->content_len;
    while (remaining > 0) {
        int read_len = httpd_req_recv(req, buf, (remaining > sizeof(buf)) ? sizeof(buf) : remaining);
        if (read_len <= 0) break;
        esp_ota_write(update_handle, (const void *)buf, read_len);
        remaining -= read_len;
    }
    
    if (esp_ota_end(update_handle) == ESP_OK) {
        if (esp_ota_set_boot_partition(update_partition) == ESP_OK) {
            httpd_resp_sendstr(req, "OTA Basarili, yeniden baslatiliyor...");
            esp_restart();
            return ESP_OK;
        }
    }
    
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA Basarisiz.");
    return ESP_FAIL;
}

void web_init() {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_AP);
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "RADAR_SISTEM_PRO",
            .ssid_len = strlen("RADAR_SISTEM_PRO"),
            .password = "12345678",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;
    config.max_uri_len = 1024;
    config.max_resp_headers_len = 1024;
    httpd_handle_t server = NULL;
    
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri_get = { .uri = "/", .method = HTTP_GET, .handler = get_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &uri_get);
        httpd_uri_t uri_save = { .uri = "/save", .method = HTTP_POST, .handler = save_post_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &uri_save);
        httpd_uri_t uri_tog = { .uri = "/tog", .method = HTTP_GET, .handler = tog_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &uri_tog);
        httpd_uri_t uri_dl = { .uri = "/dl", .method = HTTP_GET, .handler = dl_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &uri_dl);
        httpd_uri_t uri_up = { .uri = "/up", .method = HTTP_POST, .handler = ota_post_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &uri_up);
        ESP_LOGI(TAG, "Web sunucusu baslatildi.");
    } else {
        ESP_LOGE(TAG, "Web sunucusu baslatilamadi.");
    }
}
