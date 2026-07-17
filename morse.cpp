#include <Arduino.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2 // GPIO 2 adalah LED internal pada sebagian besar board ESP32
#endif

// 1 Unit Waktu = 200 milidetik
const int UNIT_TIME = 200;

// Struktur representasi Morse Code untuk huruf A-Z
const char* morseAlphabet[] = {
  ".-",   // A
  "-...", // B
  "-.-.", // C
  "-..",  // D
  ".",    // E
  "..-.", // F
  "--.",  // G
  "....", // H
  "..",   // I
  ".---", // J
  "-.-",  // K
  ".-..", // L
  "--",   // M
  "-.",   // N
  "---",  // O
  ".--.", // P
  "--.-", // Q
  ".-.",  // R
  "...",  // S
  "-",    // T
  "..-",  // U
  "...-", // V
  ".--",  // W
  "-..-", // X
  "-.--", // Y
  "--.."  // Z
};

void blinkSignal(int durationMultiplier) {
  int totalDuration = UNIT_TIME * durationMultiplier;
  int fadeTime = 80; // Durasi fade-in dan fade-out masing-masing 80ms
  int holdTime = totalDuration - (2 * fadeTime);
  if (holdTime < 0) holdTime = 0;

  // 1. Fade In (Eksponensial)
  unsigned long startFadeIn = millis();
  while (millis() - startFadeIn < fadeTime) {
    float progress = (float)(millis() - startFadeIn) / fadeTime;
    // Rumus eksponensial: e^(progress * log(256)) - 1
    int brightness = round(exp(progress * 5.545)) - 1;
    if (brightness < 0) brightness = 0;
    if (brightness > 255) brightness = 255;
    analogWrite(LED_BUILTIN, brightness);
    delay(2);
  }
  analogWrite(LED_BUILTIN, 255); // Pastikan menyala penuh di akhir fade-in

  // 2. Hold (Menahan kecerahan maksimal)
  if (holdTime > 0) {
    delay(holdTime);
  }

  // 3. Fade Out (Eksponensial)
  unsigned long startFadeOut = millis();
  while (millis() - startFadeOut < fadeTime) {
    float progress = 1.0 - ((float)(millis() - startFadeOut) / fadeTime);
    int brightness = round(exp(progress * 5.545)) - 1;
    if (brightness < 0) brightness = 0;
    if (brightness > 255) brightness = 255;
    analogWrite(LED_BUILTIN, brightness);
    delay(2);
  }
  analogWrite(LED_BUILTIN, 0); // Matikan sepenuhnya di akhir fade-out

  // 4. Jeda setelah sinyal selesai (1 unit waktu)
  delay(UNIT_TIME);
}

void playMorseChar(char c) {
  if (c >= 'A' && c <= 'Z') {
    const char* morseStr = morseAlphabet[c - 'A'];
    Serial.printf("%c: %s\n", c, morseStr);
    for (int i = 0; morseStr[i] != '\0'; i++) {
      if (morseStr[i] == '.') {
        blinkSignal(1); // Dot = 1 unit
      } else if (morseStr[i] == '-') {
        blinkSignal(3); // Dash = 3 unit
      }
    }
    delay(UNIT_TIME * 2); // Jeda antar huruf = 3 unit total (2 unit tambahan karena di blinkSignal sudah ada 1 unit)
  } else if (c == ' ') {
    Serial.println("[Space]");
    delay(UNIT_TIME * 6); // Jeda antar kata = 7 unit total (6 unit tambahan)
  }
}

void playMorseMessage(const char* message) {
  for (int i = 0; message[i] != '\0'; i++) {
    char c = toupper(message[i]);
    playMorseChar(c);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  analogWrite(LED_BUILTIN, 0);
  Serial.println("=========================================");
  Serial.println("ESP32 Morse Code Player");
  Serial.println("Pesan: HELLO WORLD");
  Serial.println("=========================================");
}

void loop() {
  playMorseMessage("HELLO WORLD");
  delay(3000); // Tunggu 3 detik sebelum mengulang pesan
}
