# Universal Controller v5.1.0 (S3 Edition) - Sistem Dokümantasyonu

  

Bu doküman, Universal Controller v5.0.0 sisteminin mimarisini, donanım bileşenlerini, pin bağlantılarını ve kritik uyarıları içermektedir. Sistem, otomotiv uygulamaları için tasarlanmış ESP32-S3 tabanlı bir kontrolcüdür.

> Tam sistem mimarisi ve BOM için: [[UniControl 5.1]] | Pinlerin firmware'deki kullanımı için: [[Unicontrol Code]]

  

## 1. Sistem Özeti

  

- **İşlemci:** ESP32-S3 (WROOM)

- **Güç Girişi:** 12V - 14.4V (Araç Aküsü)

- **Çalışma Voltajı:** 5.0V (Sistem), 3.3V (Lojik)

- **Ekran:** Nextion HMI (NX4827P043_011 - 4.3 inç)

- **Haberleşme:** CAN Bus (3.3V Lojik)

  

## 2. Sistem Mimarisi ve Katmanlar

  

### Güç Katmanı

- **Regülatör:** LM2596 DC-DC Buck Converter (12V giriş, 5.0V çıkışa ayarlanmalı)

- **Korumalar:** 3A Bıçak Sigorta ve 1N5408 Güç Diyodu (Ters kutup koruması)

  

### İzole Girişler

- Araçtan gelen +12V tetikleme sinyallerini izole etmek için **PC817 Optocoupler** kullanılır.

- Giriş pinleri: GPIO 1, GPIO 2, GPIO 3.

- Optocoupler girişinde 1K / 2.2K direnç kullanılır.

  

### Güç Çıkışları

- Yük kontrolü (Röle, Lamba, Korna vb.) için **IRLZ44Z / IRLZ44N Logic Level MOSFET** kullanılır.

- Çıkış pinleri: GPIO 38, GPIO 39, GPIO 40.

- MOSFET Gate bacağında 100 Ohm seri direnç ve 10k Ohm pull-down direnci bulunur.

  

### Veri Yolu & Ses

- **CAN Bus:** SN65HVD230 modülü (3.3V besleme, 120R sonlandırma direnci).

- **Nextion Ekran:** UART üzerinden haberleşir (5V besleme).

- **Buzzer:** Aktif buzzer, transistör gerektirmeden doğrudan **GPIO 8**'e bağlanır.

- **Data Logger:** Wemos D1 Mini Shield (SD Kart + RTC) SPI ve I2C üzerinden bağlanır.

  

## 3. ESP32-S3 Pin Bağlantı Haritası (Pinout)

  



  

1. **Voltaj Hassasiyeti:** ESP32-S3 pinleri 3.3V lojik seviyesi ile çalışır. 5V sinyal (Nextion hariç) doğrudan pinlere verilmemelidir.

2. **Buzzer Bağlantısı:** Bu sürümde Buzzer'ın artı (+) ucu doğrudan **GPIO 8**'e bağlanır. S3'ün akım kapasitesi yeterli olduğu için ekstra transistör kullanılmasına gerek yoktur.

3. **Yük Şasesi (MOSFET Bağlantısı):** Çıkışlarda (Röle/Lamba) yükün eksi (-) ucu mutlaka karta (MOSFET'in Drain bacağına) geri dönmelidir. Yük doğrudan araç şasesine bağlanırsa MOSFET devre dışı kalır ve yük sürekli çalışır.

4. **USB Pinleri:** GPIO 19 ve 20, USB/JTAG pinleridir. Bu pinler donanımsal USB iletişimi için ayrıldığından boş bırakılmalıdır.

5. **Röle Koruması:** MOSFET ile bir röle sürülüyorsa, röle bobininin uçlarına ters paralel olarak mutlaka **1N4007 koruma diyotu** (Flyback diyot) bağlanmalıdır.

# 4 Pinout

### Sol Pin Grubu (Haberleşme ve Girişler)
| GPIO    | Fonksiyon | Donanım Bağlantısı   | Notlar / Kısıtlamalar                                 |
| :------ | :-------- | :------------------- | :---------------------------------------------------- |
| **3V3** | VCC       | CAN Transceiver Gücü | Maksimum 500mA çekilebilir.                           |
| **1**   | IN1_OPTO  | Sinyal Sağ Girişi    | PC817 Opto Collector bacağı (Pull-up gerekir)         |
| **2**   | IN2_OPTO  | Sinyal Sol Girişi    | PC817 Opto Collector bacağı (Pull-up gerekir)         |
| **3**   | IN3_OPTO  | Geri Vites Girişi    | PC817 Opto Collector bacağı (Pull-up gerekir)         |
| **4**   | CAN_RX    | SN65HVD230 (CRX)     | TWAI Sürücüsü RX                                      |
| **5**   | CAN_TX    | SN65HVD230 (CTX)     | TWAI Sürücüsü TX                                      |
| **6**   | I2C_SDA   | DS1307 RTC (SDA)     | Harici 4.7k Pull-up direnci önerilir                  |
| **7**   | I2C_SCL   | DS1307 RTC (SCL)     | Harici 4.7k Pull-up direnci önerilir                  |
| **8**   | BUZZER    | Aktif Buzzer (+)     | Doğrudan pin sürümü. (Maks 40mA çeken buzzerlar için) |
| **10**  | SPI_CS    | SD Kart (CS/D8)      | Wemos shield entegrasyonu                             |
| **11**  | SPI_MOSI  | SD Kart (MOSI/D7)    |                                                       |
| **12**  | SPI_CLK   | SD Kart (CLK/D5)     |                                                       |
| **13**  | SPI_MISO  | SD Kart (MISO/D6)    |                                                       |

### Sağ Pin Grubu (Çıkışlar ve Ekran)
| GPIO   | Fonksiyon | Donanım Bağlantısı   | Notlar / Kısıtlamalar                        |
| :----- | :-------- | :------------------- | :------------------------------------------- |
| **5V** | VIN       | Ana Sistem Beslemesi | LM2596'dan gelen stabil 5.0V                 |
| **17** | UART_RX   | Nextion Sarı Kablo   | UART1. Ekrandan gelen dokunmatik veriler.    |
| **18** | UART_TX   | Nextion Mavi Kablo   | UART1. Ekrana giden çizim verileri.          |
| **38** | OUT1_HS   | Korna / Lamba Rölesi | BC546 NPN Base bacağına (1k seri direnç ile) |
| **39** | OUT2_HS   | Sağ Kör Nokta Uyarı  | BC546 NPN Base bacağına (1k seri direnç ile) |
| **40** | OUT3_HS   | Sol Kör Nokta Uyarı  | BC546 NPN Base bacağına (1k seri direnç ile) |
| **19** | BOŞ       | -                    | **KULLANILMAMALI** (USB D-)                  |
| **20** | BOŞ       | -                    | **KULLANILMAMALI** (USB D+)                  |

## 5. Malzeme Listesi (BOM) 
| Parça Adı              | Açıklama             | Notlar                           |
| :--------------------- | :------------------- | :------------------------------- |
| ESP32-S3 DevKitC-1     | Mikrodenetleyici     | Merkez Kontrol (S3 Sürümü)       |
| LM2596                 | DC-DC Buck Converter | 12V -> 5.0V Ayarlanmalı          |
| SN65HVD230             | CAN Transceiver      | 3.3V Besleme, 120R Direnç        |
| Wemos D1 Mini Shield   | Data Logger (SD+RTC) | Loglama (Jumper ile taşınacak)   |
| Nextion NX4827P043_011 | HMI Dokunmatik Ekran | 4.3 inç Intelligent Series       |
| PC817                  | Optocoupler          | Giriş İzolasyonu (3 Adet)        |
| IRLZ44N / IRLZ44Z      | Logic Level MOSFET   | Çıkış Sürücü (Z serisi önerilir) |
| Aktif Buzzer           | Sesli Uyarı          | Direkt Bağlantı (GPIO 8)         |
| 1N5408                 | Güç Diyodu           | Ana giriş ters koruma            |
| 1N4007                 | Diyot                | Röle bobin koruması              |
| Direnç 120R            | Direnç               | CAN Bus sonlandırma              |
| Direnç 1k / 2.2k       | Direnç               | Opto girişleri                   |
| Direnç 100R            | Direnç               | MOSFET Gate seri                 |
| Direnç 10k             | Direnç               | MOSFET Gate pull-down            |
| Sigorta 3A             | Bıçak Sigorta        | Ana besleme koruması             |
