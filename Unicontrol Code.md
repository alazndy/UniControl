# 💻 UniControl Pro v5.1.0 - Yazılım Mimarisi ve RTOS Task Listesi
**ESP-IDF & FreeRTOS Kapsamlı Mühendislik Dokümanı**

![ESP-IDF](https://img.shields.io/badge/Framework-ESP--IDF_v5.x-green?style=for-the-badge)
![FreeRTOS](https://img.shields.io/badge/OS-FreeRTOS_SMP-orange?style=for-the-badge)
![Core](https://img.shields.io/badge/Architecture-Dual_Core_Xtensa-blue?style=for-the-badge)

Bu doküman, UniControl Pro cihazının iç yazılım (Firmware) mimarisini, bellek yönetimini, Thread-Safe iletişim protokollerini ve eşzamanlı çalışan FreeRTOS görevlerinin (Tasks) yaşam döngülerini detaylandırır. Sistem "Super-loop" (Arduino tarzı `loop()`) mimarisi yerine, olay güdümlü (Event-Driven) tam asenkron bir RTOS mimarisi kullanır.

> Donanım ve BOM referansı: [[UniControl 5.1]] | HAL modüllerinin kullandığı pin haritası: [[UniControl GPIO]]

---

## 📑 İÇİNDEKİLER
1.[Çift Çekirdek (Dual-Core) Dağılım Stratejisi](#1-çift-çekirdek-dual-core-dağılım-stratejisi)
2.[Modüler Dosya Hiyerarşisi](#2-modüler-dosya-hiyerarşisi)
3.[Kritik Veri Yapıları (Structs)](#3-kritik-veri-yapıları-structs)
4. [FreeRTOS Task Listesi ve Yaşam Döngüleri](#4-freertos-task-listesi-ve-yaşam-döngüleri)
5.[IPC (Görevler Arası İletişim) Senkronizasyonu](#5-ipc-görevler-arası-i̇letişim-senkronizasyonu)
5. [Hata Yönetimi ve Watchdog (WDT)](#6-hata-yönetimi-ve-watchdog-wdt)

---

## 🧠 1. ÇİFT ÇEKİRDEK (DUAL-CORE) DAĞILIM STRATEJİSİ

ESP32-S3'ün iki çekirdeği (Core 0 - PRO_CPU ve Core 1 - APP_CPU), donanımsal kesmelerin (Interrupt) ve Wi-Fi yükünün sistemin gerçek zamanlı hesaplamalarını (Radar Math) geciktirmemesi için kesin sınırlarla ayrılmıştır.

*   **Core 0 (PRO_CPU):** Ağ işlemleri, Web Sunucusu, Wi-Fi SoftAP, Sistem Monitörü ve asenkron Buzzer üretimi. (Ağ trafiği sistemi bloklamamalıdır).
*   **Core 1 (APP_CPU):** Gerçek zamanlı işlemler. CAN Bus (TWAI) okuması, Trigonometrik (FPU) Radar hesaplamaları, I/O mantık devresi, Nextion HMI çizimi ve SD Kart loglama.

---

## 📂 2. MODÜLER DOSYA HİYERARŞİSİ

Kodun yönetilebilirliğini artırmak için iş mantığı (Logic) ile donanım katmanı (HAL) birbirinden ayrılmıştır.

```text
src/
├── main.c           # Uygulama giriş noktası, Init() çağrıları ve Task oluşturulması
├── hal_twai.c       # CAN Bus (SN65HVD230) konfigürasyonu, filtreleme ve okuma/yazma
├── hal_io.c         # Optocoupler girişleri (Debounce) ve MOSFET çıkışları
├── hal_sd_rtc.c     # I2C DS1307 RTC okuma ve SPI SD Kart FAT32 mount işlemleri
├── logic_fusion.c   # Radar + UDS koordinat birleştirme ve tehlike alanı algoritması
├── logic_hmi.c      # Nextion ekran UART haberleşmesi ve Smart Sanitizer parser'ı
├── web_server.c     # SoftAP HTTP Web Sunucusu (Chunked Response), WebSocket, OTA
├── settings_nvs.c   # NVS Blob (Struct) okuma, doğrulama ve kaydetme
└── utils.c          # CRC hesaplamaları, String işlemleri, Log formatlama
include/
├── defs.h           # Global Pin tanımları (Macro) ve Enum listeleri
├── structs.h        # Tüm C Struct yapılarının (NVS, Objeler, Queue) tutulduğu dosya
└── ipc_keys.h       # Mutex, Semaphore ve Queue Handle tanımları
````

---

## 🧬 3. KRİTİK VERİ YAPILARI (STRUCTS)

Sistemdeki veriler global değişkenler yerine düzenli struct yapıları içinde tutulur.

### A. Kalıcı Ayarlar (NVS Blob Struct)

Cihaz her açıldığında settings_nvs.c bu yapıyı okur.

codeC

```
typedef struct {
    uint32_t magic_word;        // Veri bozulmasını önlemek için (Örn: 0xAABBCCDD)
    float    warn_zone_m;       // Sarı bölge eşiği
    float    danger_zone_m;     // Kırmızı bölge eşiği
    float    vehicle_width_m;   // Ekran çizimi için araç genişliği
    uint16_t can_baudrate;      // 250 veya 500
    uint8_t  buzzer_vol;        // %0 - %100 PWM
    bool     fusion_mode;       // Radar + UDS birleşik mi?
} __attribute__((packed)) sys_config_t;
```

### B. Canlı Obje Takip Yapısı (Multi-Object Array)

Sensörlerden okunan tüm veriler burada toplanır. Bu dizi **Mutex** ile korunmak zorundadır!

codeC

```
typedef enum { SENSOR_RADAR, SENSOR_UDS } sensor_type_t;

typedef struct {
    uint32_t      obj_id;       // CAN'den gelen ID
    sensor_type_t source;       // Radar mı Ultrasonik mi?
    float         x_pos;        // Araca göre yanal uzaklık (Metre)
    float         y_pos;        // Araca göre dikey uzaklık (Metre)
    float         velocity;     // Yaklaşma/Uzaklaşma hızı (m/s)
    uint32_t      last_seen;    // Objenin en son görüldüğü zaman (Tick Count - Timeout için)
} tracked_obj_t;

// Sistem aynı anda 12 objeyi takip edebilir.
tracked_obj_t global_obj_list[12];
```

---

## 🔄 4. FREERTOS TASK LİSTESİ VE YAŞAM DÖNGÜLERİ

Sistemin kalbi olan görevlerin öncelik (Priority) ve ayrılan bellek (Stack) haritası.

|   |   |   |   |   |   |
|---|---|---|---|---|---|
|Task Fonksiyon Adı|Core|Priority|Stack (Bytes)|Tetikleyici|Sorumluluk Özeti|
|task_can_parser|1|5 (MAX)|4096|ISR / 5ms|TWAI RX Queue'dan okur, X/Y hesaplar, global_obj_list'i günceller.|
|task_hmi_render|1|4|4096|50ms|Obje listesini okur, Nextion UART komutlarına çevirir (vis, xval).|
|task_io_logic|1|3|2048|100ms|IN pinlerini (Sinyal/Geri Vites) okur, objelere göre OUT pinlerini tetikler.|
|task_sd_logger|1|2|4096|Event Queue|Log kuyruğunda eleman varsa uyanır, SD Karta yazar, tekrar uyur.|
|task_buzzer_pwm|0|3|2048|Event / Dinamik|En yakın nesneye göre bekleme süresini (Delay) ayarlar. Bip...bip...bip üretir.|
|task_sys_monitor|0|1 (MIN)|2048|5000ms|RAM (Heap) sızıntı kontrolü, Log basımı, WDT beslemesi.|
|httpd_task (Gömülü)|0|2|8192|Web Request|ESP-IDF'in kendi yarattığı SoftAP Web sunucusu task'ı.|

### Detaylı Task Algoritmaları:

**1. task_can_parser Algoritması:**

- twai_receive(..., portMAX_DELAY) ile bloklanıp bekler. CPU tüketmez.
    
- Mesaj geldiğinde, ID kontrolü yapar. (11-bit ise Radar, 29-bit ise UDS).
    
- Hex byte'larını kaydırarak (Bitwise Shift) mesafe ve açıyı çıkartır.
    
- xMutexTake(xObjListMutex, portMAX_DELAY) ile Obje dizisini kilitler.
    
- Matematiksel dönüşümü (FPU) yapar, diziyi günceller.
    
- xMutexGive(xObjListMutex) ile kilidi bırakır.
    

**2. task_io_logic Algoritması:**

- Girişlerde Bounce (ark) olmaması için yazılımsal Debounce filtresi uygular.
    
- Örnek Senaryo: Eğer (IN1_SAG_SINYAL == 1) VE (Sağ tarafta danger_zone_m içinde obje var ise) -> OUT2_SAG_UYARI = 1 yap.
    
- Eğer bir OUT pini tetiklendiyse, xQueueSend(xLogQueue, ...) ile Logger taskına bir event fırlatır.
    

**3. task_sd_logger Algoritması (Asenkron Loglama):**

- Eğer Log atma işlemi task_io veya task_can içinde yapılsaydı, SD Kartın 50ms süren yazma gecikmesi radarı dondururdu (Priority Inversion).
    
- Bunun yerine bu task xQueueReceive(xLogQueue, &logData, portMAX_DELAY) ile sonsuz uykudadır.
    
- Kuyruğa "Kör Nokta Uyarısı Verildi" olayı düşer düşmez uyanır, RTC'den saati okur, dosyayı açar, yazar (Append) ve kapatıp tekrar uyur.
    

---

## 🚥 5. IPC (GÖREVLER ARASI İLETİŞİM) SENKRONİZASYONU

ESP32'de aynı değişkene iki task aynı anda yazmaya kalkarsa (Race Condition) veya biri yazarken diğeri okursa sistem çöker (Core Panic - LoadStoreError). Bunu önleyen kilit mekanizmaları main.c içinde ilk başta oluşturulur:

1. **xObjListMutex (Mutex):**
    
    - global_obj_list dizisine erişimi yönetir.
        
    - Kullananlar: task_can_parser (Yazar), task_hmi_render (Okur), task_buzzer_pwm (Okur).
        
2. **xLogQueue (Kuyruk / Queue):**
    
    - Tipi: QueueHandle_t. Uzunluk: 10 eleman. Boyut: sizeof(log_event_t).
        
    - Olayları sıraya dizer. SD kart yazma hızı yetişemese bile veri kaybolmaz, RAM'deki kuyrukta bekler.
        
3. **xSystemEventGroup (Event Group):**
    
    - Sistemin durumlarını bayraklar (Flags) halinde tutar.
        
    - BIT0: Wi-Fi Hazır.
        
    - BIT1: CAN Bus Başarılı.
        
    - BIT2: SD Kart Okundu.
        
    - Kullanım: HMI taskı ekrana "Sistem Hazır" yazmak için bu 3 bitin de "1" olmasını bekler (xEventGroupWaitBits).
        

---

## 🛡️ 6. HATA YÖNETİMİ VE WATCHDOG (WDT)

Otomotiv elektroniğinde sistemin kitlenmesi (Freeze) kabul edilemez.

- **TWAI Bus-Off Kurtarması:** Eğer CAN hattında kısa devre olursa, ESP32 TWAI birimi "Bus-Off" durumuna geçer ve iletişimi keser. task_sys_monitor bu durumu tespit edip, twai_initiate_recovery() çağırarak kendini otomatik resetlemeden donanımı kurtarır.
    
- **Task Watchdog Timer (TWDT):** Kritik tasklar (Örn: task_can_parser) başlatılırken esp_task_wdt_add() ile Watchdog'a eklenir. Eğer task bir Sonsuz Döngüye (While(1)) girer ve vTaskDelay çağırmazsa, donanım WDT'si devreye girip 3 saniye içinde ESP32'yi güvenli bir şekilde yeniden başlatır (Hard Reset).
    
- **Timeout Temizliği:** Sensör koparsa, ekranda eski objeler hayalet (Ghost Object) olarak kalmamalıdır. task_can_parser diziyi tarar; last_seen süresi 200ms'yi geçmiş nesneleri listeden temizler (Sıfırlar).