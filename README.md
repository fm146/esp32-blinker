# ESP32 PWM Blinker CLI Controller

Proyek ini adalah pengontrol LED internal (built-in LED) pada ESP32 menggunakan **PWM (Pulse Width Modulation)** secara interaktif melalui Serial Monitor (CLI). Program ini dirancang dengan pendekatan **non-blocking** (tanpa fungsi `delay()`), sehingga input dari pengguna tidak akan mengganggu kemulusan kedipan atau pernapasan (breathing) LED.

---

## 🌟 Fitur Utama
1. **Kurva Breathing Eksponensial**: Transisi redup-terang menggunakan pemetaan eksponensial ($y = e^{\sin(x)}$) yang dimodelkan agar sesuai dengan persepsi logaritmik mata manusia terhadap cahaya, menghasilkan efek breathing yang jauh lebih halus dibandingkan dengan metode linear biasa.
2. **Input CLI Non-Blocking**: Pembacaan buffer serial dilakukan karakter demi karakter secara asinkron di dalam loop utama, sehingga transisi LED tidak akan terhenti/tersendat saat Anda sedang mengetik perintah.
3. **Batas Kecerahan Dinamis (Min/Max)**: Parameter `min` dan `max` kecerahan membatasi seluruh mode visual secara dinamis, termasuk mode SOLID.
4. **Multi-Mode Visual**:
   * `BREATH` (Breathing/Fading halus)
   * `BLINK` (Blinking tegas ON/OFF menggunakan batas PWM min & max)
   * `SOLID` (Kecerahan konstan statis)
   * `OFF` (LED padam sepenuhnya)

---

## 🖥️ Command Serial CLI
Buka Serial Monitor dengan baud rate **115200**, lalu kirimkan perintah berikut:

| Perintah | Deskripsi | Contoh Penggunaan |
| :--- | :--- | :--- |
| **`help`** | Menampilkan menu panduan perintah | `help` |
| **`status`** / **`info`** | Menampilkan parameter status blinker saat ini | `status` |
| **`mode <type>`** | Mengubah mode visual: `breath`, `blink`, `solid`, `off` | `mode blink` |
| **`speed <val>`** | Mengatur multiplier kecepatan (angka desimal > 0) | `speed 2.5` atau `speed 0.5` |
| **`period <ms>`** | Mengatur durasi satu siklus kedipan penuh dalam milidetik | `period 1500` |
| **`min <0-255>`** | Mengatur tingkat PWM minimum | `min 10` |
| **`max <0-255>`** | Mengatur tingkat PWM maksimum | `max 180` |
| **`set <0-255>`** | Mengatur kecerahan statis (otomatis beralih ke mode SOLID) | `set 128` |

---

## 🚀 Cara Menjalankan dengan PlatformIO

### 1. Upload ke Board ESP32
Sambungkan ESP32 ke komputer Anda, lalu jalankan perintah berikut pada terminal di direktori proyek:
```bash
pio run -e control -t upload
```

### 2. Membuka Serial Monitor
Untuk berinteraksi dengan CLI, jalankan perintah monitor berikut:
```bash
pio device monitor -e control
```
*(Gunakan kombinasi tombol `Ctrl + C` atau `Ctrl + ]` untuk keluar dari monitor).*