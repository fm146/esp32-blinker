#include <Arduino.h>
#include <I2S.h>
#include <math.h>

// Definisikan Pin default
int pinBCLK = 26; // BCK / BCLK pada PCM5100A
int pinWS   = 25; // LRCK / WS / LCK pada PCM5100A
int pinDOUT = 22; // DIN pada PCM5100A (Data out dari ESP32)

// Nada tes
const int SAMPLE_RATE = 44100;
const float FREQUENCY = 440.0; // Nada A4 (440Hz)
const float AMPLITUDE = 10000.0;
unsigned long sampleIndex = 0;

bool isPlaying = false;

void printMenu() {
  Serial.println("\n=================================================");
  Serial.println("ESP32 I2S DAC (PCM5100A / PCM5102A) Tester");
  Serial.println("=================================================");
  Serial.println("Skema Koneksi PCM5100A ke ESP32:");
  Serial.println("  1. VCC  -> 3.3V / 5V");
  Serial.println("  2. GND  -> GND");
  Serial.printf("  3. LCK  (LRCK/WS) -> GPIO %d\n", pinWS);
  Serial.printf("  4. BCK  (BCLK)    -> GPIO %d\n", pinBCLK);
  Serial.printf("  5. DIN  (Data)    -> GPIO %d\n", pinDOUT);
  Serial.println("  6. SCK  -> Hubungkan ke GND (Solder/Jumper di board)");
  Serial.println("=================================================");
  Serial.println("Commands:");
  Serial.println("  play                : Mulai menghasilkan nada 440Hz");
  Serial.println("  stop                : Hentikan nada");
  Serial.println("  set <bclk> <ws> <din> : Ubah pin");
  Serial.println("  status              : Tampilkan status & konfigurasi pin");
  Serial.println("=================================================");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  printMenu();
}

void startAudio() {
  if (isPlaying) return;
  
  // Konfigurasi pin di library I2S: (sck, fs, sd, outSd, inSd)
  // Set pinDOUT pada sdPin (arg 3) dan outSdPin (arg 4) agar terkonfigurasi dengan benar di mode simplex
  I2S.setAllPins(pinBCLK, pinWS, pinDOUT, pinDOUT, -1);
  
  // Jalankan dalam mode I2S Philips, 44100Hz, 16-bit
  if (!I2S.begin(I2S_PHILIPS_MODE, SAMPLE_RATE, 16)) {
    Serial.println("ERROR: Gagal menginisialisasi I2S!");
    return;
  }
  
  sampleIndex = 0;
  isPlaying = true;
  Serial.println("SUCCESS: I2S Aktif. Memainkan nada 440Hz...");
}

void stopAudio() {
  if (!isPlaying) return;
  I2S.end();
  isPlaying = false;
  Serial.println("SUCCESS: I2S Dihentikan.");
}

void loop() {
  // Baca input serial command
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input.equalsIgnoreCase("play")) {
      startAudio();
    } 
    else if (input.equalsIgnoreCase("stop")) {
      stopAudio();
    } 
    else if (input.startsWith("set ")) {
      // Format: set <bclk> <ws> <din>
      int b, w, d;
      if (sscanf(input.c_str(), "set %d %d %d", &b, &w, &d) == 3) {
        bool tempPlaying = isPlaying;
        if (tempPlaying) stopAudio();
        
        pinBCLK = b;
        pinWS = w;
        pinDOUT = d;
        
        Serial.printf("SUCCESS: Pin diubah ke BCLK=%d, WS=%d, DIN=%d\n", pinBCLK, pinWS, pinDOUT);
        if (tempPlaying) startAudio();
      } else {
        Serial.println("ERROR: Format salah. Gunakan: set <bclk> <ws> <din>");
      }
    } 
    else if (input.equalsIgnoreCase("status")) {
      Serial.printf("INFO: Status -> Playing: %s\n", isPlaying ? "YA" : "TIDAK");
      Serial.printf("INFO: Konfigurasi Pin -> BCLK: %d | WS: %d | DIN: %d\n", pinBCLK, pinWS, pinDOUT);
    } 
    else {
      Serial.println("ERROR: Perintah tidak dikenal. Gunakan play | stop | set | status");
    }
  }

  // Kirim audio buffer jika sedang bermain
  if (isPlaying) {
    int16_t samples[128]; // 64 sample stereo (kiri & kanan bergantian)
    for (int i = 0; i < 64; i++) {
      float t = (float)sampleIndex / SAMPLE_RATE;
      float angle = 2.0 * PI * FREQUENCY * t;
      int16_t val = round(sin(angle) * AMPLITUDE);
      samples[2 * i]     = val; // Kiri
      samples[2 * i + 1] = val; // Kanan
      sampleIndex++;
    }
    // Gunakan write_blocking agar pengiriman data sinkron dengan clock audio DMA
    I2S.write_blocking(samples, sizeof(samples));
  }
}
