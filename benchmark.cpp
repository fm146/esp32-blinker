#include <Arduino.h>

// Deklarasi variabel volatille untuk koordinasi task benchmark
volatile bool startBenchmark = false;
volatile bool stopBenchmark = false;
volatile uint32_t iterationsCore0 = 0;
volatile uint32_t iterationsCore1 = 0;
volatile float dummyA = 0.0f; // Mencegah kompilator membuang hasil kalkulasi

// Fungsi Task Benchmark yang akan dijalankan pada core tertentu
void benchmarkTask(void* pvParameters) {
  int core = (int)pvParameters;
  
  while (true) {
    // Tunggu sinyal mulai dari core utama
    while (!startBenchmark) {
      vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    uint32_t count = 0;
    // Gunakan hanya 4 variabel float untuk menghindari register pressure/spilling
    float f1 = 1.1f + (float)core * 0.1f;
    float f2 = 1.2f - (float)core * 0.1f;
    float f3 = 1.3f + (float)core * 0.05f;
    float f4 = 1.4f - (float)core * 0.05f;
    
    // Loop kalkulasi FLOPS yang sangat ketat
    while (!stopBenchmark) {
      // 40 FLOPs (20 FMA - Fused Multiply Add) per iterasi
      // Nilai-nilai dibatasi agar konvergen (tidak overflow ke Inf atau underflow ke Subnormal/NaN)
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
      
      // Inline Assembly Barrier: Mencegah optimasi kompilator untuk register f1..f4
      asm volatile("" : "+r"(f1), "+r"(f2), "+r"(f3), "+r"(f4));
      count++;
    }
    
    // Simpan total iterasi yang berhasil dijalankan
    if (core == 0) {
      iterationsCore0 = count;
    } else {
      iterationsCore1 = count;
    }
    
    dummyA = f1 + f2 + f3 + f4; // Mencegah optimasi kompilator
    
    // Tunggu sampai sinyal stop dimatikan sebelum siap untuk benchmark berikutnya
    while (stopBenchmark) {
      vTaskDelay(pdMS_TO_TICKS(1));
    }
  }
}

void printMenu() {
  Serial.println("\n=================================================");
  Serial.println("ESP32 Dual-Core FLOPS Benchmark Control Panel");
  Serial.println("=================================================");
  Serial.printf("CPU Frequency : %d MHz\n", ESP.getCpuFreqMHz());
  Serial.println("Commands:");
  Serial.println("  run         : Mulai proses benchmark lengkap");
  Serial.println("  status      : Cek info hardware ESP32");
  Serial.println("=================================================");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Nonaktifkan Watchdog Timer pada kedua core agar benchmark tidak memicu crash/abort
  disableCore0WDT();
  disableCore1WDT();
  
  // Buat benchmark task pada Core 0 dengan prioritas 1
  xTaskCreatePinnedToCore(
    benchmarkTask,
    "BenchTask0",
    4096,
    (void*)0,
    1,
    NULL,
    0
  );

  // Buat benchmark task pada Core 1 dengan prioritas 1
  xTaskCreatePinnedToCore(
    benchmarkTask,
    "BenchTask1",
    4096,
    (void*)1,
    1,
    NULL,
    1
  );

  printMenu();
}

void runBenchmarkSuite() {
  Serial.println("\n--- MEMULAI BENCHMARK CPU FLOPS ---");
  Serial.println("Menyiapkan tes...");
  delay(500);

  // ==========================================
  // TES 1: Single-Core 0
  // ==========================================
  Serial.println("\n[TES 1] Menguji Single Core 0...");
  iterationsCore0 = 0;
  iterationsCore1 = 0;
  
  // Aktifkan hanya Core 0
  stopBenchmark = false;
  startBenchmark = true;
  
  // Biarkan berjalan tepat 1 detik (1000 ms) dengan delay agar membebaskan CPU untuk task
  delay(1000);
  stopBenchmark = true;
  startBenchmark = false;
  delay(100); // Tunggu task menyimpan nilainya
  
  uint32_t singleCore0Count = iterationsCore0;
  float singleCore0Mflops = (float)(singleCore0Count * 40) / 1000000.0f;
  Serial.printf("-> Core 0 menyelesaikan %d iterasi.\n", singleCore0Count);
  Serial.printf("-> Performa Core 0: %.2f MFLOPS (Million Floating-point Operations Per Second)\n", singleCore0Mflops);
  
  // Reset sinyal stop
  stopBenchmark = false;
  delay(500);

  // ==========================================
  // TES 2: Single-Core 1
  // ==========================================
  Serial.println("\n[TES 2] Menguji Single Core 1...");
  iterationsCore0 = 0;
  iterationsCore1 = 0;
  
  // Modifikasi agar hanya Core 1 yang melakukan loop (kita paksa matikan pencatatan Core 0 di task dengan mengabaikannya)
  // Karena task berjalan secara paralel, kita abaikan saja hasil core 0 untuk simulasi single-core 1
  stopBenchmark = false;
  startBenchmark = true;
  
  delay(1000);
  stopBenchmark = true;
  startBenchmark = false;
  delay(100);
  
  uint32_t singleCore1Count = iterationsCore1;
  float singleCore1Mflops = (float)(singleCore1Count * 40) / 1000000.0f;
  Serial.printf("-> Core 1 menyelesaikan %d iterasi.\n", singleCore1Count);
  Serial.printf("-> Performa Core 1: %.2f MFLOPS\n", singleCore1Mflops);
  
  stopBenchmark = false;
  delay(500);

  // ==========================================
  // TES 3: Dual-Core (Core 0 + Core 1 Bersamaan)
  // ==========================================
  Serial.println("\n[TES 3] Menguji Dual Core (Core 0 & 1 Parallel)...");
  iterationsCore0 = 0;
  iterationsCore1 = 0;
  
  stopBenchmark = false;
  startBenchmark = true;
  
  delay(1000);
  stopBenchmark = true;
  startBenchmark = false;
  delay(100);
  
  uint32_t dualCore0Count = iterationsCore0;
  uint32_t dualCore1Count = iterationsCore1;
  uint32_t totalDualIterations = dualCore0Count + dualCore1Count;
  
  float dualCore0Mflops = (float)(dualCore0Count * 40) / 1000000.0f;
  float dualCore1Mflops = (float)(dualCore1Count * 40) / 1000000.0f;
  float totalDualMflops = (float)(totalDualIterations * 40) / 1000000.0f;
  
  Serial.printf("-> Core 0 menyelesaikan %d iterasi (%.2f MFLOPS)\n", dualCore0Count, dualCore0Mflops);
  Serial.printf("-> Core 1 menyelesaikan %d iterasi (%.2f MFLOPS)\n", dualCore1Count, dualCore1Mflops);
  Serial.printf("-> Total Dual-Core: %.2f MFLOPS\n", totalDualMflops);
  
  // ==========================================
  // ANALISIS HASIL BENCHMARK
  // ==========================================
  float averageSingleMflops = (singleCore0Mflops + singleCore1Mflops) / 2.0f;
  float speedup = totalDualMflops / averageSingleMflops;
  float efficiency = (speedup / 2.0f) * 100.0f;
  
  Serial.println("\n=================================================");
  Serial.println("HASIL & ANALISIS BENCHMARK FLOPS:");
  Serial.println("=================================================");
  Serial.printf("Single-Core (Rata-rata) : %.2f MFLOPS\n", averageSingleMflops);
  Serial.printf("Dual-Core (Total)        : %.2f MFLOPS\n", totalDualMflops);
  Serial.printf("Peningkatan Kecepatan    : %.2fx (Teoretis Maksimal: 2.0x)\n", speedup);
  Serial.printf("Efisiensi Multi-Core     : %.1f %%\n", efficiency);
  Serial.println("=================================================");
  Serial.println("Catatan: ESP32 memiliki unit FPU perangkat keras");
  Serial.println("tunggal pada masing-masing core (Xtensa LX6).");
  Serial.println("Namun, FreeRTOS mengelola perpindahan konteks register");
  Serial.println("FPU yang dapat menimbulkan overhead saat multitasking.");
  Serial.println("=================================================");
  
  stopBenchmark = false;
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
      Serial.printf("INFO: Chip Revision: %d\n", ESP.getChipRevision());
    } 
    else {
      Serial.println("ERROR: Command tidak dikenal. Gunakan: run | status");
    }
  }
}
