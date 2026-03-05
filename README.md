# ESP-TOTP (ESP32-S3)

Project ini menghasilkan kode TOTP (RFC6238, HMAC-SHA1) di ESP32-S3.
Waktu disinkronisasi lewat SNTP, lalu kode TOTP dicetak ke serial (ESP_LOGI).

## Sinkronisasi Waktu

- **Startup dengan WiFi:** Sync via SNTP, simpan waktu ke NVS (flash memory)
- **Startup tanpa WiFi/SNTP gagal:** Gunakan waktu terakhir dari NVS sebagai fallback
- **Keuntungan:** TOTP tetap akurat meski ESP mati lama (asal sinkron setidaknya sekali)

## Quick Start

### 1. Clone & Setup
```bash
git clone <repo>
cd esp_totp
```

### 2. Konfigurasi (PENTING!)
```bash
idf.py menuconfig
```
Masuk ke **TOTP Configuration** dan isi:
- `WiFi SSID` → Nama jaringan WiFi
- `WiFi Password` → Password WiFi (boleh kosong untuk open)
- `TOTP Secret (Base32)` → Shared secret (contoh: `JBSWY3DPEHPK3PXP`)
  - Ambil dari Google Authenticator, Microsoft Authenticator, atau service 2FA lainnya
  - Format: Base32 (A-Z, 2-7, boleh dengan spasi)
- (Opsional) `Digits` (default 6) dan `Time step` (default 30 detik)

**⚠️ Catatan:** Nilai yang kamu isi akan tersimpan di `sdkconfig` (ignored git, jadi aman).

### 3. Build & Flash
```bash
idf.py build
idf.py -p COMx flash monitor
```
Ganti `COMx` dengan port serial device (contoh: `COM3` di Windows, `/dev/ttyUSB0` di Linux).

### 4. Monitor Output
Di serial monitor akan muncul:
```
I (1234) totp_app: TOTP: 123456 (valid ~28s)
I (5678) totp_app: TOTP: 234567 (valid ~27s)
```

## Konfigurasi Detail

Buka konfigurasi:

- `idf.py menuconfig`
- Masuk ke: **TOTP Configuration**

Opsi yang tersedia:

- `WiFi SSID` → SSID jaringan WiFi
- `WiFi Password` → Password WiFi
- `SNTP Server` → Server NTP (default: `pool.ntp.org`)
- `TOTP Secret (Base32)` → Shared secret dalam format Base32
- `Digits` → Jumlah digit (default: 6)
- `Time step` → Interval (default: 30 detik)
- `T0` → RFC6238 offset (default: 0)

## Build / Flash / Monitor

Jalankan:

- `idf.py build`
- `idf.py -p COMx flash monitor`

Output contoh di monitor:

- `TOTP: 123456 (valid ~28s)`

## Keamanan & Best Practices

- ✅ Secret disimpan di `sdkconfig` (local only, tidak di-commit ke git)
- ✅ NVS tidak mengenkripsi → Gunakan NVS encryption untuk produksi
- ✅ WiFi password disimpan di flash → Pertimbangkan secure storage untuk produk
- ⚠️ Jangan bagikan `sdkconfig` yang sudah isi rahasia
- ⚠️ Untuk production, implementasikan storage terenkripsi (NVS encryption / sealed device)

## Catatan Teknis

- Pastikan board punya akses internet untuk sinkronisasi awal (SNTP).
- Waktu disimpan ke NVS, jadi TOTP tetap kerja meski WiFi mati (asal tidak mati berbulan-bulan tanpa sinkron ulang).
- RTC internal ESP32-S3 tidak memiliki baterai backup, jadi bergantung pada NVS untuk fallback.
- Jika ESP mati sangat lama (>6 bulan) tanpa sinkron, akurasi waktu mungkin ±1 detik/hari → TOTP bisa off by one.

## File yang Penting

- `main/security_key_s3.c` → Main app logic
- `main/totp.c/h` → RFC6238 HMAC-SHA1 implementation
- `main/base32.c/h` → Base32 decoder
- `main/wifi_time.c/h` → WiFi & SNTP sync
- `main/nvs_time.c/h` → NVS time backup/restore
- `.gitignore` → Protect `sdkconfig` (secret) & build artifacts