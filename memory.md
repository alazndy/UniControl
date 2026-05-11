# UniControl Memory (v5.1.0 - S3 Edition)

## Son Durum
- Tarih: 2026-05-11
- Sürüm: v5.1.0
- Durum: Active (Development Started)
- Aktif agent: Gemini

## Gemini
### Yaptıkları
- Proje dizini doğrulandı: `C:\Users\turha\Desktop\Dev_Ops_New\10_ADC\Projects\UniControl`.
- Git reposu başlatıldı ve GitHub'a pushlandı: `https://github.com/alazndy/UniControl.git`.
- `gitrepo.md` ve `.gitignore` dosyaları oluşturuldu.
- ESP-IDF (PlatformIO) proje yapısı kuruldu (`src/`, `include/`).
- `include/defs.h` (GPIO haritası) ve `include/structs.h` (Veri yapıları) oluşturuldu.
- `src/main.c` içine FreeRTOS task iskeleti (`task_can_parser`, `task_hmi_render`, `task_io_logic`, `task_sys_monitor`) yazıldı.
- `platformio.ini` ve `partitions_singleapp_large.csv` konfigürasyonları eklendi.
### Yapacakları
- `hal_twai.c` modülünü oluşturup CAN Bus okuma mantığını kurmak.
- `logic_fusion.c` ile Radar ve UDS verilerini birleştirmek.
- Nextion HMI UART protokolünü (`logic_hmi.c`) gerçeklemek.
### Notlar
- `raios health` komutunda UniControl henüz görünmüyor, `entities.json` güncellenmeli.

## Plan
### Tamamlananlar
- [x] Proje konumunun standartlara göre ayarlanması
- [x] BOM Excel dosyasının oluşturulması
- [x] GitHub reposunun oluşturulması ve ilk push
- [x] Proje iskeletinin (Scaffold) kurulması
### Devam Edenler
- [ ] HAL (Hardware Abstraction Layer) modüllerinin yazılması
### Sıradakiler
- [ ] CAN Bus (TWAI) sürücüsünün implementasyonu

## Karar Günlüğü
| Tarih | Agent | Karar | Neden |
|-------|-------|-------|-------|
| 2026-05-11 | Gemini | GitHub Repo oluşturuldu | MASTER kuralları ve memory gereksinimi |
| 2026-05-11 | Gemini | PlatformIO + ESP-IDF seçildi | UniControl 5.1 dökümantasyonuna uygunluk |
| 2026-05-11 | Gemini | Modular C mimarisi kuruldu | "Spaghetti code" engellemek ve RTOS yönetimi için |
