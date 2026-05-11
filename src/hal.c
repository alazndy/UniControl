#include "hal.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "driver/uart.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "HAL";

void hal_init_gpios() {
    /*
     * DONANIM SEMASI NOTLARI:
     * - Girisler (IN1-3): PC817 Optocoupler uzerinden. Arac +12V gelince GPIO 0'a (GND) çekilir.
     * - MOSFET Cikislar (MOS1-3): BC546 -> IRF9540 (High-Side). GPIO High = +12V Cikis Aktif.
     * - Role Cikislar (REL1-3): BC546 -> G6K-2F Sinyal Rolesi. GPIO High = Role Cekili.
     * - Buzzer: GPIO High = Sesli Ikaz Aktif.
     */
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<PIN_BUZZER) | (1ULL<<PIN_MOS1) | (1ULL<<PIN_MOS2) | (1ULL<<PIN_MOS3) |
                           (1ULL<<PIN_REL1) | (1ULL<<PIN_REL2) | (1ULL<<PIN_REL3);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL<<PIN_IN1) | (1ULL<<PIN_IN2) | (1ULL<<PIN_IN3);
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    // Başlangıçta hepsini kapat
    gpio_set_level(PIN_BUZZER, 0);
    gpio_set_level(PIN_MOS1, 0);
    gpio_set_level(PIN_MOS2, 0);
    gpio_set_level(PIN_MOS3, 0);
    gpio_set_level(PIN_REL1, 0);
    gpio_set_level(PIN_REL2, 0);
    gpio_set_level(PIN_REL3, 0);
}

void hal_init_twai() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(PIN_CAN_TX, PIN_CAN_RX, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        if (twai_start() == ESP_OK) {
            ESP_LOGI(TAG, "TWAI baslatildi.");
        }
    } else {
        ESP_LOGE(TAG, "TWAI baslatilamadi!");
    }
}

void hal_init_uart() {
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_NUM_1, 1024 * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, PIN_NEXTION_TX, PIN_NEXTION_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void hal_init_i2c() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = PIN_RTC_SDA,
        .scl_io_num = PIN_RTC_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
}

void hal_init_sd() {
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    const char mount_point[] = "/sdcard";
    
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_SD_MOSI,
        .miso_io_num = PIN_SD_MISO,
        .sclk_io_num = PIN_SD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    if (spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA) != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus baslatilamadi");
        return;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_SD_CS;
    slot_config.host_id = host.slot;

    if (esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card) == ESP_OK) {
        ESP_LOGI(TAG, "SD Kart baglandi");
    } else {
        ESP_LOGE(TAG, "SD Kart baglanamadi");
    }
}

void hal_init() {
    hal_init_gpios();
    hal_init_twai();
    hal_init_uart();
    hal_init_i2c();
    hal_init_sd();
}

void get_rtc_time(char* time_str, int max_len) {
    uint8_t data[7];
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (0x68 << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (0x68 << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 6, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, data + 6, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    if (i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS) == ESP_OK) {
        int s = (data[0] & 0x0F) + ((data[0] >> 4) * 10);
        int m = (data[1] & 0x0F) + ((data[1] >> 4) * 10);
        int h = (data[2] & 0x0F) + ((data[2] >> 4) * 10);
        int D = (data[4] & 0x0F) + ((data[4] >> 4) * 10);
        int M = (data[5] & 0x0F) + ((data[5] >> 4) * 10);
        int Y = (data[6] & 0x0F) + ((data[6] >> 4) * 10) + 2000;
        snprintf(time_str, max_len, "%02d-%02d-%04d %02d:%02d:%02d", D, M, Y, h, m, s);
    } else {
        snprintf(time_str, max_len, "RTC_HATA");
    }
    i2c_cmd_link_delete(cmd);
}

void log_alarm_to_sd() {
    static TickType_t last_log_time = 0;
    TickType_t now = xTaskGetTickCount();
    if ((now - last_log_time) * portTICK_PERIOD_MS < 1000) {
        return; // Saniyede maksimum 1 yazma
    }
    last_log_time = now;

    FILE* f = fopen("/sdcard/log.txt", "a");
    if (f == NULL) return;
    char time_str[32];
    get_rtc_time(time_str, sizeof(time_str));
    fprintf(f, "[%s] ALARM DURUMU\n", time_str);
    fclose(f);
}

void set_output(int mos_idx, int rel_idx, bool state) {
    if (mos_idx == 1) gpio_set_level(PIN_MOS1, state);
    if (mos_idx == 2) gpio_set_level(PIN_MOS2, state);
    if (mos_idx == 3) gpio_set_level(PIN_MOS3, state);
    if (rel_idx == 1) gpio_set_level(PIN_REL1, state);
    if (rel_idx == 2) gpio_set_level(PIN_REL2, state);
    if (rel_idx == 3) gpio_set_level(PIN_REL3, state);
}

int get_input(int in_idx) {
    if (in_idx == 1) return gpio_get_level(PIN_IN1);
    if (in_idx == 2) return gpio_get_level(PIN_IN2);
    if (in_idx == 3) return gpio_get_level(PIN_IN3);
    return 1; // Pull-up varsayilan
}
