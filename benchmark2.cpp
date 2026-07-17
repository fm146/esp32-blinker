#include <Arduino.h>

// Definisikan tipe data Vector untuk SIMD 16-bit (mengemas 4 integer 16-bit dalam register 64-bit)
typedef int16_t v4i16 __attribute__ ((vector_size (8)));

// Mode Benchmark
enum BenchmarkMode {
  MODE_IDLE,
  MODE_FLOAT,
  MODE_SIMD
};

// Variabel volatile untuk koordinasi task benchmark dual-core
volatile BenchmarkMode currentMode = MODE_IDLE;
volatile uint32_t iterationsFloatCore0 = 0;
volatile uint32_t iterationsFloatCore1 = 0;
volatile uint32_t iterationsSIMDCore0 = 0;
volatile uint32_t iterationsSIMDCore1 = 0;

volatile float dummyFloatCore0 = 0.0f;
volatile float dummyFloatCore1 = 0.0f;
volatile int16_t dummySIMDCore0 = 0;
volatile int16_t dummySIMDCore1 = 0;

// Task Benchmark Utama - Dijalankan secara independen pada Core 0 & Core 1
void benchmarkTask(void* pvParameters) {
  int core = (int)pvParameters;
  
  while (true) {
    // Tunggu sinyal mode aktif dari loop kontrol utama
    while (currentMode == MODE_IDLE) {
      vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    // ==========================================
    // MODE 1: Scalar Float (FP32)
    // ==========================================
    if (currentMode == MODE_FLOAT) {
      uint32_t count = 0;
      float f1 = 1.1f + (float)core * 0.1f;
      float f2 = 1.2f - (float)core * 0.1f;
      float f3 = 1.3f + (float)core * 0.05f;
      float f4 = 1.4f - (float)core * 0.05f;
      
      while (currentMode == MODE_FLOAT) {
        // 40 FLOPs (20 FMA) per iterasi
        f1 = f1 * 0.9f + 0.1f;
        f2 = f2 * 0.9f + 0.15f;
        f3 = f3 * 0.9f + 0.2f;
        f4 = f4 * 0.9f + 0.25f;
        
        f1 = f1 * 0.9f + f2 * 0.1f;
        f2 = f2 * 0.9f + f3 * 0.1f;
        f3 = f3 * 0.9f + f4 * 0.1f;
        f4 = f4 * 0.9f + f1 * 0.1f;
        
        f1 = f1 * 0.9f - f3 * 0.1f;
        f2 = f2 * 0.9f - f4 * 0.1f;
        f3 = f3 * 0.9f - f1 * 0.1f;
        f4 = f4 * 0.9f - f2 * 0.1f;

        f1 = f1 * 0.9f + 0.1f;
        f2 = f2 * 0.9f + 0.15f;
        f3 = f3 * 0.9f + 0.2f;
        f4 = f4 * 0.9f + 0.25f;

        f1 = f1 * 0.9f + f2 * 0.1f;
        f2 = f2 * 0.9f + f3 * 0.1f;
        f3 = f3 * 0.9f + f4 * 0.1f;
        f4 = f4 * 0.9f + f1 * 0.1f;
        
        asm volatile("" : "+r"(f1), "+r"(f2), "+r"(f3), "+r"(f4));
        count++;
      }
      
      if (core == 0) {
        iterationsFloatCore0 = count;
        dummyFloatCore0 = f1 + f2 + f3 + f4;
      } else {
        iterationsFloatCore1 = count;
        dummyFloatCore1 = f1 + f2 + f3 + f4;
      }
    }
    // ==========================================
    // MODE 2: 16-bit Integer SIMD
    // ==========================================
    else if (currentMode == MODE_SIMD) {
      uint32_t count = 0;
      v4i16 v1 = { (int16_t)(10 + core), 20, 30, 40 };
      v4i16 v2 = { 2, 3, 4, (int16_t)(5 - core) };
      
      while (currentMode == MODE_SIMD) {
        // 40 Operasi Integer SIMD (5 baris x 4 elemen x 2 operasi [kali & tambah])
        v1 = v1 * v2 + v1;
        v2 = v2 * v1 - v2;
        v1 = v1 * v2 + v1;
        v2 = v2 * v1 - v2;
        v1 = v1 * v2 + v1;
        
        asm volatile("" : "+r"(v1), "+r"(v2));
        count++;
      }
      
      if (core == 0) {
        iterationsSIMDCore0 = count;
        dummySIMDCore0 = v1[0] + v2[3];
      } else {
        iterationsSIMDCore1 = count;
        dummySIMDCore1 = v1[0] + v2[3];
      }
    }
    
    // Tunggu sampai loop kontrol utama me-reset status ke IDLE
    while (currentMode != MODE_IDLE) {
      vTaskDelay(pdMS_TO_TICKS(1));
    }
  }
}

void printMenu() {
  Serial.println("\n=================================================");
  Serial.println("ESP32 Dual-Core SIMD vs Scalar Float Benchmark");
  Serial.println("=================================================");
  Serial.printf("CPU Frequency : %d MHz\n", ESP.getCpuFreqMHz());
  Serial.println("Commands:");
  Serial.println("  run         : Mulai proses benchmark komparatif (Sequential Dual-Core)");
  Serial.println("  status      : Cek info hardware ESP32");
  Serial.println("=================================================");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Nonaktifkan watchdog pada kedua core agar tidak memicu abort pada kalkulasi ketat
  disableCore0WDT();
  disableCore1WDT();
  
  // Buat Benchmark Task pada Core 0 (Prioritas 1)
  xTaskCreatePinnedToCore(
    benchmarkTask,
    "BenchTaskCore0",
    4096,
    (void*)0,
    1,
    NULL,
    0
  );

  // Buat Benchmark Task pada Core 1 (Prioritas 1)
  xTaskCreatePinnedToCore(
    benchmarkTask,
    "BenchTaskCore1",
    4096,
    (void*)1,
    1,
    NULL,
    1
  );

  printMenu();
}

void runBenchmarkSuite() {
  Serial.println("\n--- MEMULAI BENCHMARK DUAL-CORE KOMPARATIF ---");
  Serial.println("Menyiapkan tes...");
  delay(500);

  // ==========================================
  // TES 1: Scalar Float (FP32) - DUA CORE SEKALIGUS
  // ==========================================
  Serial.println("\n[TES 1] Menjalankan Scalar FP32 pada Dual Core secara paralel...");
  iterationsFloatCore0 = 0;
  iterationsFloatCore1 = 0;
  
  currentMode = MODE_FLOAT;
  delay(1000); // Berjalan tepat 1 detik
  
  currentMode = MODE_IDLE;
  delay(100); // Tunggu task menyimpan nilainya
  
  uint32_t floatCore0 = iterationsFloatCore0;
  uint32_t floatCore1 = iterationsFloatCore1;
  uint32_t totalFloatCount = floatCore0 + floatCore1;
  
  float floatMflopsCore0 = (float)(floatCore0 * 40) / 1000000.0f;
  float floatMflopsCore1 = (float)(floatCore1 * 40) / 1000000.0f;
  float totalFloatMflops = (float)(totalFloatCount * 40) / 1000000.0f;
  
  Serial.printf("-> Core 0 menyelesaikan %d iterasi (%.2f MFLOPS)\n", floatCore0, floatMflopsCore0);
  Serial.printf("-> Core 1 menyelesaikan %d iterasi (%.2f MFLOPS)\n", floatCore1, floatMflopsCore1);
  Serial.printf("-> Total Performa Scalar FP32: %.2f MFLOPS\n", totalFloatMflops);
  
  delay(500);

  // ==========================================
  // TES 2: Integer SIMD (16-bit) - DUA CORE SEKALIGUS
  // ==========================================
  Serial.println("\n[TES 2] Menjalankan Integer SIMD pada Dual Core secara paralel...");
  iterationsSIMDCore0 = 0;
  iterationsSIMDCore1 = 0;
  
  currentMode = MODE_SIMD;
  delay(1000); // Berjalan tepat 1 detik
  
  currentMode = MODE_IDLE;
  delay(100); // Tunggu task menyimpan nilainya
  
  uint32_t simdCore0 = iterationsSIMDCore0;
  uint32_t simdCore1 = iterationsSIMDCore1;
  uint32_t totalSIMDCount = simdCore0 + simdCore1;
  
  float simdMopsCore0 = (float)(simdCore0 * 40) / 1000000.0f;
  float simdMopsCore1 = (float)(simdCore1 * 40) / 1000000.0f;
  float totalSIMDMops = (float)(totalSIMDCount * 40) / 1000000.0f;
  
  Serial.printf("-> Core 0 menyelesaikan %d iterasi (%.2f MOPS)\n", simdCore0, simdMopsCore0);
  Serial.printf("-> Core 1 menyelesaikan %d iterasi (%.2f MOPS)\n", simdCore1, simdMopsCore1);
  Serial.printf("-> Total Performa Integer SIMD: %.2f MOPS\n", totalSIMDMops);
  
  // ==========================================
  // ANALISIS & BANDINGKAN HASIL
  // ==========================================
  Serial.println("\n=================================================");
  Serial.println("HASIL BENCHMARK KOMPARATIF (DUAL-CORE SEQUENTIAL):");
  Serial.println("=================================================");
  Serial.printf("Dual-Core Scalar FP32  : %.2f MFLOPS\n", totalFloatMflops);
  Serial.printf("Dual-Core Integer SIMD : %.2f MOPS (Million Operations Per Second)\n", totalSIMDMops);
  
  float ratio = totalSIMDMops / totalFloatMflops;
  Serial.printf("Rasio Performa SIMD/FPU: %.2fx lebih cepat\n", ratio);
  Serial.println("=================================================");
  Serial.println("Analisis:");
  Serial.println("- Unit FPU pada Xtensa LX6 (ESP32) adalah scalar (1 operasi per cycle).");
  Serial.println("- SIMD menggunakan GCC Vector Extension (v4i16) mengemas 4 data 16-bit");
  Serial.println("  sekaligus, yang dijalankan lewat hardware MAC16 (DSP extension ESP32).");
  Serial.println("=================================================");
  currentMode = MODE_IDLE;
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toLowerCase();

    if (input == "run") {
      runBenchmarkSuite();
    } 
    else if (input == "status") {
      Serial.printf("INFO: CPU Freq: %d MHz\n", ESP.getCpuFreqMHz());
      Serial.printf("INFO: Free Heap: %d Bytes\n", ESP.getFreeHeap());
    } 
    else {
      Serial.println("ERROR: Command tidak dikenal. Gunakan: run | status");
    }
  }
}
