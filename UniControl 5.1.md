# 🛡️ UniControl Pro v5.1.0 - Automotive Radar & I/O Controller
**Kapsamlı Sistem Mimarisi ve Mühendislik Başvuru Dokümanı**

![Platform](https://img.shields.io/badge/Platform-ESP32--S3-blue?style=for-the-badge) ![Framework](https://img.shields.io/badge/Framework-ESP--IDF-green?style=for-the-badge) ![OS](https://img.shields.io/badge/OS-FreeRTOS-orange?style=for-the-badge) ![Architecture](https://img.shields.io/badge/Architecture-Modular_C-red?style=for-the-badge) ![Standard](https://img.shields.io/badge/Standard-Automotive_Grade-yellow?style=for-the-badge)

Bu doküman, ağır vasıtalar ve iş makineleri için geliştirilen **UniControl Pro v5.1.0** projesinin tüm donanımsal, yazılımsal ve elektriksel spesifikasyonlarını içeren nihai referans belgesidir. Sistem; Radar (77GHz) ve Ultrasonik sensörlerin verilerini birleştiren (Sensor Fusion), otonom I/O kararları alan, kara kutu kaydı tutan ve Wi-Fi üzerinden yönetilebilen endüstriyel bir araç bilgisayarıdır.

---

## 📑 İÇİNDEKİLER
1.[Sistem Mimarisi ve Dosya Hiyerarşisi](#1-sistem-mimarisi-ve-dosya-hiyerarşisi)
2. [Detaylı Malzeme Listesi (BOM) ve Komponent Analizi](#2-detaylı-malzeme-listesi-bom-ve-komponent-analizi)
3.[Pinout ve Donanımsal Devre Tasarım Mantığı](#3-pinout-ve-donanımsal-devre-tasarım-mantığı)
3. [FreeRTOS Task Yapısı ve IPC İletişimi](#4-freertos-task-yapısı-ve-ipc-iletişimi)
5.[Sensör Haberleşmesi ve Sensor Fusion (CAN Bus)](#5-sensör-haberleşmesi-ve-sensor-fusion-can-bus)
4. [Hafıza Yönetimi ve NVS Struct Yapısı](#6-hafıza-yönetimi-ve-nvs-struct-yapısı)
7.[Web Sunucusu (SoftAP), OTA ve Live Debug](#7-web-sunucusu-softap-ota-ve-live-debug)
5. [Kara Kutu (Data Logger) Sistemi](#8-kara-kutu-data-logger-sistemi)
6. [PCB Çizim Kuralları ve Otomotiv Korumaları](#9-pcb-çizim-kuralları-ve-otomotiv-korumaları)
10.[Derleme, PlatformIO ve Partition Ayarları](#10-derleme-platformio-ve-partition-ayarları)

---

## 📂 1. SİSTEM MİMARİSİ VE DOSYA HİYERARŞİSİ

Proje "Spaghetti Code" yapısını engellemek için Modüler C (Modular C) mimarisinde tasarlanmıştır. Proje klasörü altındaki `src` dizini aşağıdaki gibi olmalıdır:

*   `main.c`: RTOS Task'larının başlatıldığı (xTaskCreate) ve genel init işlemlerinin yapıldığı giriş noktası.
*   `defs.h`: Tüm pin tanımlamaları, makrolar (Macro), ortak Struct'lar ve Task önceliklerinin tutulduğu global başlık dosyası.
*   `hal.c / hal.h`: Hardware Abstraction Layer. I/O pinleri, TWAI (CAN), I2C, SPI (SD Kart) ve UART khởi tạo (init) fonksiyonları.
*   `logic.c / logic.h`: Çarpışma algoritmalarının, Sensor Fusion matematiğinin ve I/O tetikleme koşullarının bulunduğu karar mekanizması.
*   `web.c / web.h`: Wi-Fi SoftAP kurulumu, HTTP sunucusu (Chunked Response), WebSocket ve OTA işleyicisi.
*   `settings.c / settings.h`: NVS üzerinden ayarların (Blob Struct) güvenli şekilde okunup yazılması.

---

## ⚙️ 2. DETAYLI MALZEME LİSTESİ (BOM) VE KOMPONENT ANALİZİ

| Referans | Kategori | Tam Parça Kodu | Teknik Özellik / Limitler | Açıklama |
| :--- | :--- | :--- | :--- | :--- |
| **U1** | Mikrodenetleyici | ESP32-S3-WROOM-1 | 240MHz, 8MB Flash, 2MB PSRAM | FPU donanım ivmelendirmesi ile sinüs/kosinüs hesaplamaları için seçildi. |
| **U2** | Güç Regülatörü | LM2596 (veya MP1584) | Giriş: 12-24V, Çıkış: 5.0V (3A) | Linear değil Switching yapıdadır, araç içi yüksek ısıda stabil çalışır. |
| **U3** | CAN Transceiver| SN65HVD230DR | VCC: 3.3V, ISO 11898 | ESP32'nin TWAI pinleri ile Brigade sensörün diferansiyel CAN-H/L sinyalleri arasında köprü. |
| **U4..U6**| İzole Giriş | PC817 (Sharp) | İzolasyon: 5000Vrms | Araç tesisatından gelen gürültülü/dalgalı 12V sinyalleri 3.3V'a düşürür ve optik olarak ayırır. |
| **Q1..Q3**| Çıkış Sürme | IRF9540 (P-Ch MOSFET)| Vdss: -100V, Id: -19A | **High-Side Switch.** +12V'u yükün üzerine anahtarlar. |
| **Q4..Q6**| MOSFET Sürücü| BC546 (NPN BJT) | Vce: 65V | 3.3V ESP32 pini ile 12V P-Kanal MOSFET'in Gate bacağını sürmek için aracı transistör. |
| **MOD1** | Data Logger | Wemos D1 SD Shield | SPI MicroSD + I2C DS1307 | RTC pili (CR1220) ile araç aküsü bitse bile saati tutar. |
| **MOD2** | Ekran (HMI) | Nextion NX4827P043_011 | 4.3", 5V TTL UART | ESP32'nin grafik işlemesine gerek bırakmadan RAM'i ve CPU'yu rahatlatır. |

---

## 🔌 3. [[UniControl GPIO]] VE DONANIMSAL DEVRE TASARIM MANTIĞI

ESP32-S3'ün boot/strapping pin çakışmalarını önleyen kesinleşmiş pin haritası:

### A. İletişim ve Modül Pinleri
*   **CAN_RX:** GPIO 4 | **CAN_TX:** GPIO 5 (SN65HVD230 VCC'si 3.3V olmalıdır!)
*   **RTC_SDA:** GPIO 6 | **RTC_SCL:** GPIO 7 (Harici 4.7K Pull-Up gerektirir)
*   **SD_CS:** GPIO 10 | **SD_MOSI:** GPIO 11 | **SD_MISO:** GPIO 13 | **SD_CLK:** GPIO 12
*   **HMI_TX:** GPIO 18 | **HMI_RX:** GPIO 17 (Nextion UART1'e bağlanır. 115200 Baud)
*   **BUZZER:** GPIO 8 (Doğrudan ESP32'den sürülür, buzzer GND'ye bağlanır)

### B. İzole Giriş Tasarım Mantığı (IN1: 1, IN2: 2, IN3: 3)
Araçtan gelen 12V sinyalini okumak için:
1. Araçtan gelen +12V sinyali, seri **2.2K Ohm** direnç üzerinden PC817'nin Anot (Pin 1) bacağına girer.
2. PC817'nin Katot (Pin 2) bacağı aracın **ortak şasesine (GND)** bağlanır.
3. PC817'nin Collector (Pin 4) bacağı ESP32'nin **3.3V** pinine bağlanır.
4. PC817'nin Emitter (Pin 3) bacağı ESP32'nin **GPIO (Örn: 1)** pinine ve **10K Ohm Pull-Down** ile ESP32 şasesine bağlanır. *(Böylece 12V geldiğinde ESP32 pini HIGH olur).*

### C. High-Side Çıkış Tasarım Mantığı (OUT1: 38, OUT2: 39, OUT3: 40)
Otomotiv standardı gereği yükün şasesi sabittir, +12V anahtarlanır.
1. ESP32 pini (**Örn: GPIO 38**) -> **1K Ohm** direnç -> BC546 NPN'in **Base** bacağına.
2. BC546 **Emitter** -> Ortak GND.
3. BC546 **Collector** -> IRF9540 (P-Channel) **Gate** bacağına.
4. IRF9540 **Gate** bacağından kaynaklanan parazitleri önlemek için Gate ile Source (+12V) arasına **10K Ohm Pull-Up** direnç.
5. IRF9540 **Source** -> Araç Ana **+12V** hattına.
6. IRF9540 **Drain** -> Çıkış Yüküne (Korna/Lamba + ucu).

---

## 🧠 4. [[Unicontrol Code]] YAPISI VE IPC İLETİŞİMİ

Projede `vTaskStartScheduler()` altında koşacak 5 ana görev vardır. İletişim **Thread-Safe** olmalı ve Queue (Kuyruk) veya Mutex kullanılmalıdır.

| Task Adı | Çalıştığı Core | Öncelik | Stack Size | Periyot | Görevi ve Sorumlulukları |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `task_radar` | Core 1 | 5 (High) | 4096 | 10ms | `twai_receive` bloğunda bekler. CAN'den mesaj gelir gelmez IRQ tetiklenir, okur, X/Y objelerini hesaplar ve global bir `MultiObj_t` dizisine (Mutex korumalı) yazar. |
| `task_nextion`| Core 1 | 4 (Med) | 4096 | 50ms | Mutex'i alıp `MultiObj_t` dizisini okur. UART üzerinden HMI ekrana çizim komutlarını (Örn: `add 1,0,50,40`) yollar. Nextion'dan gelen dokunma komutlarını `strstr()` ve **Smart Sanitizer** ile temizleyip ayrıştırır. |
| `task_buzzer` | Core 0 | 3 (Med) | 2048 | Event | Hedefin mesafesine göre dinamik bekleme yapar. Bloklanmaz. `vTaskDelay(mesafe_m * 100 / portTICK_PERIOD_MS)`. |
| `task_io` | Core 1 | 2 (Low) | 2048 | 100ms | Opto girişlerini okur (Software Debounce), ayarlanan I/O kurallarına göre `gpio_set_level` ile çıkışları High/Low yapar. |
| `task_sys` | Core 0 | 1 (Idle) | 2048 | 5000ms | `esp_get_free_heap_size()` verisini okur. Watchdog Timer (WDT) besler (Feed). Log/Debug işlerini RAM'i şişirmeden halleder. |

---

## 📡 5. SENSÖR HABERLEŞMESİ VE SENSOR FUSION (CAN BUS)

**ESP-IDF TWAI (Two-Wire Automotive Interface) Konfigürasyonu:**
*   Sistem aynı hatta hem standart hem extended paketleri dinlemek zorundadır. Bu yüzden `twai_filter_config_t` maskesi tüm ID'leri geçirecek şekilde (Accept All) ayarlanır.

**A. Brigade Backsense (BS-9100T) - 77GHz Radar:**
*   **Format:** 11-Bit Standard ID.
*   **Baud Rate:** 250 kbps veya 500 kbps (NVS'ten seçilebilir).
*   **Veri:** Uzun menzil (0-60m). Objelerin radyal mesafesi (Radius) ve açısı (Angle) gelir.
*   **Matematiksel Dönüşüm (FPU ile):** 
    ```c
    float x_coord = radius * cos(angle * M_PI / 180.0);
    float y_coord = radius * sin(angle * M_PI / 180.0);
    ```

**B. Brigade UDS - Ultrasonik Sistem:**
*   **Format:** 29-Bit Extended ID.
*   **Baud Rate:** 250 kbps.
*   **Veri:** Kör nokta, tampon dibi hassas algılama (0.3m - 2.5m).

**C. Sensor Fusion Algoritması:**
Eğer `task_radar` hem UDS'den (tamponda cisim var) hem de Radardan (3 metre ileride cisim var) paket alırsa, ikisini de aynı `MultiObj_t` struct'ına kaydeder. Cihaz ekranda UDS'yi kırmızı bölge (Tehlike), Radarı sarı/yeşil bölge (İzleme) olarak gösterir.

---

## 💾 6. HAFIZA YÖNETİMİ VE NVS STRUCT YAPISI

Cihazın konfigürasyonu, her açılışta okunmak üzere ESP32 NVS (Non-Volatile Storage) partition'ına bir **Blob (Binary Large Object)** olarak yazılır. Bu yöntem, her değişkeni tek tek kaydetmekten çok daha hızlı ve güvenlidir.

**NVS C Struct Yapısı (`defs.h` içinde):**
```c
typedef struct {
    float warn_distance_m;    // Sarı bölge sınırı (Örn: 4.5m)
    float danger_distance_m;  // Kırmızı bölge sınırı (Örn: 2.0m)
    float vehicle_width_m;    // Araç genişliği (Örn: 2.55m)
    int   baud_rate;          // CAN Baud (250 veya 500)
    bool  radar_enabled;      // Radar modülü aktif mi?
    bool  uds_enabled;        // Ultrasonik modülü aktif mi?
    int   buzzer_volume;      // 0-100 arası PWM değeri
} sys_settings_t;

extern sys_settings_t current_settings;
````

---

## 🌐 7. WEB SUNUCUSU (SoftAP), OTA VE LIVE DEBUG

Cihaz sahada kurulduğunda bilgisayar bağlanmasına gerek kalmaz.

- **SoftAP:** SSID UniControl_Pro, Şifre: NVS'te tutulur (Default: 12345678), Gateway: 192.168.4.1
    
- **Web Sunucusu Çökme Koruması:** esp_http_server kütüphanesi kullanılır. HTML dosyası büyük olduğunda belleği (Heap) tüketmemek için, HTML stringi parçalara bölünerek gönderilir:
    
    codeC
    
    ```
    httpd_resp_send_chunk(req, html_part1, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, html_part2, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, NULL, 0); // İletimi bitir
    ```
    
- **OTA (Over-The-Air) Update:** Web arayüzünde /update POST endpoint'i bulunur. Yüklenen firmware.bin dosyası stream halinde (parça parça) okunarak esp_ota_write fonksiyonu ile bir sonraki partition'a (App1) yazılır. Başarılı olursa esp_ota_set_boot_partition çağrılıp cihaz yeniden başlatılır.
    
- **Live Debug:** /debug sayfası üzerinden WebSocket bağlantısı açılarak, CAN Bus hattından gelen ham hex verileri, RAM durumu ve loglar anlık olarak telefon ekranında akıtılabilir.
    

---

## 🗄️ 8. KARA KUTU (DATA LOGGER) SİSTEMİ

Olayların hukuki analizi için log tutulur. SD Kart yazma işlemleri asenkron olmalı, task_radar'ı bloklamamalıdır.

- **Donanım Protokolü:** Wemos Shield ESP32'nin SPI hattına bağlıdır (VSPI veya HSPI). DS1307 RTC, I2C hattına bağlıdır.
    
- **Log Tetikleme Koşulu (Event-Driven):** Her okumayı kaydetmek SD kartı kısa sürede bozar. Log sadece aşağıdaki durumlarda atılır:
    
    1. Hedef danger_distance_m eşiğinin altına girdiğinde (Tehlike başladı).
        
    2. Hedef danger_distance_m sınırından çıktığında (Tehlike bitti).
        
    3. OUT1, OUT2 çıkışları aktif olduğunda veya IN pinlerinden araç sinyalleri/geri vites okunduğunda.
        
- **Rate Limiting (Cooldown):** Sisteme "Aynı saniye içinde maksimum 1 log atılabilir" kuralı eklenmiştir (Yazılımsal timer ile).
    
- **Örnek SD Kart log.txt Formatı:**
    
    codeText[2026-03-18 14:05:12] [EVENT] TEHLIKE_BASLADI: 1.45m (X:0.5, Y:1.3)
    
    ```
    [2026-03-18 14:05:13][IO_TRG] OUT2_AKTIF: Sol Uyari Verildi.[2026-03-18 14:05:22] [EVENT] TEHLIKE_BITTI: 4.20m
    ```
    

---

## ⚡ 9. PCB ÇİZİM KURALLARI VE OTOMOTİV KORUMALARI

Bir araca takılacak devrenin "Automotive Grade" olması için PCB (Printed Circuit Board) tasarımında uyulması zorunlu kurallar:

1. **Güç Girişi Koruması:** Aküden gelen ana +12V hattına; **3A Bıçak Sigorta**, ardından ters akım/ters bağlantı koruması için **1N5408 Güç Diyodu** (veya P-Channel Reverse Polarity devresi) ve yüksek voltaj sıçramaları (Load Dump) için **TVS Diyot (Örn: SMCJ24A)** eklenmelidir.
    
2. **Flyback Koruması:** OUT1, OUT2, OUT3 çıkışları eğer araç üzerinde bobinli bir yük (Röle, Mekanik Korna vb.) sürecekse, dışarıdan bağlanan yükün uçlarına ters **1N4007 Flyback Diyotu** konulmalıdır. Cihaz içindeki MOSFET'in (IRF9540) Drain'den GND'ye TVS konulması da şiddetle önerilir.
    
3. **İzolasyon Bariyeri:** PC817 Optocoupler'ların PCB yerleşiminde, 12V tarafındaki pinler ile 3.3V (ESP32) tarafındaki pinler arasında en az **2mm clearance (boşluk)** bırakılmalı, gerekirse PCB üzerine isolation slot (yarık) açılmalıdır.
    
4. **Toprak Hattı (Ground Plane):** Tüm GND bağlantıları kalın bir Polygon Pour ile birleştirilmeli, CAN Bus modülünün GND'si ile ESP32 GND'si aynı şasede yıldız noktası (Star Ground) mantığı ile birleşmelidir.
    

---

## 🛠️ 10. DERLEME, PLATFORMIO VE PARTITION AYARLARI

Proje Arduino IDE ile değil, profesyonel **VS Code + PlatformIO (ESP-IDF Framework)** ortamında derlenmelidir.

### platformio.ini Konfigürasyonu:

codeIni

```
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = espidf
monitor_speed = 115200
board_build.partitions = partitions_singleapp_large.csv
build_flags = 
    -D CORE_DEBUG_LEVEL=3
    -D CONFIG_FREERTOS_HZ=1000
```

### partitions_singleapp_large.csv (8MB Flash Dağılımı):

ESP32-S3'ün 8MB flaş belleği, OTA destekleyecek şekilde parçalanmalıdır:

codeCSV

```
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
otadata,  data, ota,     0xf000,  0x2000,
phy_init, data, phy,     0x11000, 0x1000,
app0,     app,  ota_0,   0x20000, 0x300000,   # 3MB Fabrika Yazılımı
app1,     app,  ota_1,   0x320000,0x300000,   # 3MB OTA Güncelleme Alanı
spiffs,   data, spiffs,  0x620000,0x1D0000,   # Log ve Web dosyaları (opsiyonel)
```

**Derleme Komutları:**

- Projeyi temizle: pio run -t clean
    
- Derle ve Yükle: pio run -t upload
    
- Seri Portu İzle: pio device monitor
    

---

© 2026 UniControl Systems - Gizli ve Mülki Dokümandır. Sadece yetkili mühendisler tarafından referans alınabilir.