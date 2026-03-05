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
- `SNTP Server (Primary)` → NTP server utama (default: `pool.ntp.org`)
- `SNTP Server (Backup)` → NTP server fallback (default: `time.nist.gov`, kosongkan untuk disable)
- `SNTP Sync Timeout` → Timeout SNTP (default: 60 detik, naikin jika network lambat)
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

## Build & Flash

```bash
idf.py build
idf.py -p COMx flash monitor
```
Ganti `COMx` dengan port serial (contoh: `COM3` di Windows)

## Expected Output

Setelah flash & monitor:
```
I (1234) wifi_time: WiFi connected
I (5678) nvs_time: Time saved to NVS: 1772717427
I (6000) totp_app: TOTP ready (digits=6, step=30).
I (6100) totp_app: TOTP: 123456 (valid ~29s)
I (36100) totp_app: TOTP: 234567 (valid ~30s)
```

Atau jika SNTP timeout (fallback NVS):
```
W (18931) wifi_time: Primary SNTP server timeout
I (18931) wifi_time: Trying backup server: time.nist.gov
I (18931) totp_app: ╔══════════════════════════════════════╗
W (18931) totp_app: ║  SNTP SYNC FAILED (OFFLINE MODE)    ║
W (18931) totp_app: ║  Menggunakan waktu terakhir (cache) ║
```

## Troubleshooting

### SNTP Timeout / Tidak Bisa Sync Waktu
**Gejala:** `E (xxx) wifi_time: SNTP sync timeout`

**Kemungkinan Penyebab:**
1. **ISP/Firewall blok port 123 (NTP)** (very common di ISP/WiFi publik)
   - Solusi: Ganti ke backup server atau gunakan VPN
2. **Server primary unreachable**
   - Solusi: Set `TOTP_SNTP_SERVER_BACKUP` ke server lain (time.nist.gov, time.google.com)
3. **Network latency tinggi**
   - Solusi: Naikin `TOTP_SNTP_TIMEOUT_SEC` di menuconfig (default 60s, max 300s)
4. **WiFi tidak konek internet** (hanya connect ke router)
   - Solusi: Cek konektivitas WiFi & DNS resolution

**Debug Steps:**
```bash
idf.py menuconfig
# TOTP Configuration:
# - TOTP_SNTP_TIMEOUT_SEC = 120 (2 menit)
# - TOTP_SNTP_SERVER_BACKUP = "time.google.com"
idf.py build
idf.py -p COMx flash monitor
# Monitor log untuk SNTP progress
```

### TOTP Kode Salah Setelah ESP Mati
**Gejala:** Setelah power off 5+ menit, kode TOTP tidak match authenticator

**Penyebab:** Menggunakan cached time dari NVS, belum re-sync SNTP

**Solusi:**
- Pastikan WiFi tersedia saat startup (untuk SNTP sync)
- Jika perlu offline lama, tambah RTC module (battery backup)
- Monitor warning: `⚠️ Cache age: XXX seconds - TOTP mungkin inaccurate!`

## Konfigurasi Lengkap

Semua opsi di `TOTP Configuration`:
- `WiFi SSID` → SSID jaringan
- `WiFi Password` → Password (boleh kosong)
- `SNTP Server (Primary)` → NTP utama (pool.ntp.org)
- `SNTP Server (Backup)` → NTP fallback (time.nist.gov)
- `SNTP Sync Timeout` → Timeout NTP (default 60s)
- `TOTP Secret (Base32)` → Shared secret
- `Digits` → Jumlah digit (default 6)
- `Time step` → Interval (default 30s)
- `T0` → RFC6238 offset (default 0)

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