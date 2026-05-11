# 🛡️ UniControl Pro v5.1.0

Automotive ADAS (Advanced Driver Assistance System) Controller based on ESP32-S3 and ESP-IDF.

## 🎯 Projenin Amacı
Bu proje; ağır vasıtalar, iş makineleri ve lüks karavanlar için tasarlanmış bir ADAS beynidir. Kör noktalardaki engelleri tespit eder, sürücüyü uyarır ve olayları SD karta loglar.

## 🛠️ Teknik Özellikler
- **MCU:** ESP32-S3 DevKitC-1 (Dual Core, 240MHz)
- **Framework:** ESP-IDF (Strict C, FreeRTOS)
- **CAN Bus:** SN65HVD230 (Brigade Radar & UDS Sensör Desteği)
- **HMI:** Nextion 4.3" Intelligent Serisi (UART1)
- **Kara Kutu:** MicroSD Kart + DS1307 RTC (SPI2/I2C0)
- **I/O:** 3x Opto-İzole Giriş, 3x High-Side MOSFET (+12V), 3x Sinyal Rölesi

## 📂 Dosya Yapısı
- `src/main.c`: Sistem giriş noktası ve görev başlatıcı.
- `src/hal.c`: Donanım Soyutlama Katmanı (GPIO, CAN, UART, I2C, SPI).
- `src/logic.c`: Radar algoritmaları ve FreeRTOS görevleri.
- `src/web.c`: SoftAP, Web Dashboard ve OTA sunucusu.
- `src/settings.c`: NVS üzerinden kalıcı ayarlar yönetimi.
- `src/defs.h`: Pin tanımları ve veri yapıları.

## 🌐 Web Arayüzü
- **SSID:** `RADAR_SISTEM_PRO`
- **Şifre:** `12345678`
- **IP:** `192.168.4.1`

## 🚀 Kurulum (PlatformIO)
1. PlatformIO Core yüklü olduğundan emin olun.
2. Proje dizininde terminali açın.
3. Derleme için: `pio run`
4. Yükleme için: `pio run --target upload`

## ⚖️ Lisans
Bu proje Göktuğ için özel olarak geliştirilmiştir.
