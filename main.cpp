#include <Arduino.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2 // GPIO 2 is the built-in LED on most ESP32 dev boards
#endif

// Enum for blinking modes
enum Mode {
  MODE_BREATH, // Smooth fade-in and fade-out using exponential sine wave
  MODE_BLINK,  // Sharp ON/OFF transition using PWM bounds
  MODE_SOLID,  // Constant brightness
  MODE_OFF     // LED turned completely off
};

// Configuration variables
Mode currentMode = MODE_BREATH;
float speedMultiplier = 1.0;
unsigned long breathingPeriodMs = 2000; // Base cycle duration in milliseconds
int minBrightness = 0;
int maxBrightness = 255;
int solidBrightness = 128;

// State variables
int lastBrightness = -1;
String inputBuffer = "";

// Forward declarations
void printHelp();
void printStatus();
void parseCommand(String cmd);
void handleSerial();
void updateLED();

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  analogWrite(LED_BUILTIN, 0);

  // Print startup greeting and help menu
  printHelp();
  printStatus();
}

void loop() {
  handleSerial();
  updateLED();
}

void handleSerial() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      inputBuffer.trim();
      if (inputBuffer.length() > 0) {
        parseCommand(inputBuffer);
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;
    }
  }
}

void parseCommand(String cmd) {
  cmd.trim();
  int spaceIndex = cmd.indexOf(' ');
  String action = (spaceIndex > -1) ? cmd.substring(0, spaceIndex) : cmd;
  String arg = (spaceIndex > -1) ? cmd.substring(spaceIndex + 1) : "";
  action.toLowerCase();
  arg.trim();

  if (action == "help") {
    printHelp();
  }
  else if (action == "status" || action == "info") {
    printStatus();
  }
  else if (action == "mode") {
    arg.toLowerCase();
    if (arg == "breath") {
      currentMode = MODE_BREATH;
      Serial.println("SUCCESS: Mode set to BREATHING.");
    } else if (arg == "blink") {
      currentMode = MODE_BLINK;
      Serial.println("SUCCESS: Mode set to BLINKING.");
    } else if (arg == "solid") {
      currentMode = MODE_SOLID;
      Serial.println("SUCCESS: Mode set to SOLID.");
    } else if (arg == "off") {
      currentMode = MODE_OFF;
      Serial.println("SUCCESS: Mode set to OFF.");
    } else {
      Serial.println("ERROR: Invalid mode. Use: breath, blink, solid, off");
    }
  }
  else if (action == "speed") {
    if (arg.length() > 0) {
      float val = arg.toFloat();
      if (val > 0.0) {
        speedMultiplier = val;
        Serial.print("SUCCESS: Speed multiplier set to ");
        Serial.println(speedMultiplier, 2);
      } else {
        Serial.println("ERROR: Speed must be a positive number.");
      }
    } else {
      Serial.print("INFO: Current speed is ");
      Serial.println(speedMultiplier, 2);
    }
  }
  else if (action == "period") {
    if (arg.length() > 0) {
      long val = arg.toInt();
      if (val > 0) {
        breathingPeriodMs = val;
        Serial.print("SUCCESS: Period set to ");
        Serial.print(breathingPeriodMs);
        Serial.println(" ms.");
      } else {
        Serial.println("ERROR: Period must be a positive integer.");
      }
    } else {
      Serial.print("INFO: Current period is ");
      Serial.print(breathingPeriodMs);
      Serial.println(" ms.");
    }
  }
  else if (action == "min") {
    if (arg.length() > 0) {
      int val = arg.toInt();
      if (val >= 0 && val <= 255) {
        minBrightness = val;
        if (minBrightness > maxBrightness) {
          maxBrightness = minBrightness;
        }
        Serial.printf("SUCCESS: Min brightness set to %d (Max: %d).\n", minBrightness, maxBrightness);
      } else {
        Serial.println("ERROR: Min brightness must be between 0 and 255.");
      }
    } else {
      Serial.printf("INFO: Min brightness is %d.\n", minBrightness);
    }
  }
  else if (action == "max") {
    if (arg.length() > 0) {
      int val = arg.toInt();
      if (val >= 0 && val <= 255) {
        maxBrightness = val;
        if (maxBrightness < minBrightness) {
          minBrightness = maxBrightness;
        }
        Serial.printf("SUCCESS: Max brightness set to %d (Min: %d).\n", maxBrightness, minBrightness);
      } else {
        Serial.println("ERROR: Max brightness must be between 0 and 255.");
      }
    } else {
      Serial.printf("INFO: Max brightness is %d.\n", maxBrightness);
    }
  }
  else if (action == "set") {
    if (arg.length() > 0) {
      int val = arg.toInt();
      if (val >= 0 && val <= 255) {
        solidBrightness = val;
        currentMode = MODE_SOLID;
        Serial.printf("SUCCESS: Set solid brightness to %d and switched to SOLID mode.\n", solidBrightness);
      } else {
        Serial.println("ERROR: Brightness must be between 0 and 255.");
      }
    } else {
      Serial.printf("INFO: Solid brightness setting is %d.\n", solidBrightness);
    }
  }
  else {
    Serial.println("ERROR: Unknown command. Type 'help' for a list of commands.");
  }
}

void printHelp() {
  Serial.println("\n=================================================");
  Serial.println("      ESP32 PWM Blinker CLI Controller");
  Serial.println("=================================================");
  Serial.println("Commands:");
  Serial.println("  help           : Show this help menu");
  Serial.println("  status / info  : Show current blinker parameters");
  Serial.println("  mode <type>    : Set mode: breath | blink | solid | off");
  Serial.println("  speed <val>    : Set speed multiplier (e.g., 0.5, 2.0)");
  Serial.println("  period <ms>    : Set cycle period in ms (e.g., 2000)");
  Serial.println("  min <0-255>    : Set minimum PWM brightness");
  Serial.println("  max <0-255>    : Set maximum PWM brightness");
  Serial.println("  set <0-255>    : Set solid brightness (auto-switches to solid mode)");
  Serial.println("=================================================");
}

void printStatus() {
  Serial.println("\n--- Current Blinker Status ---");
  Serial.print("  Mode            : ");
  switch (currentMode) {
    case MODE_BREATH: Serial.println("BREATHING (Smooth)"); break;
    case MODE_BLINK:  Serial.println("BLINKING (Sharp)"); break;
    case MODE_SOLID:  Serial.println("SOLID (Constant)"); break;
    case MODE_OFF:    Serial.println("OFF"); break;
  }
  Serial.printf("  Speed Multiplier: %.2f\n", speedMultiplier);
  Serial.printf("  Base Period     : %lu ms (Effective: %.0f ms)\n", breathingPeriodMs, breathingPeriodMs / speedMultiplier);
  Serial.printf("  Min Brightness  : %d\n", minBrightness);
  Serial.printf("  Max Brightness  : %d\n", maxBrightness);
  Serial.printf("  Solid Brightness: %d\n", solidBrightness);
  Serial.println("------------------------------");
}

void updateLED() {
  int targetBrightness = 0;
  float effectivePeriod = (float)breathingPeriodMs / speedMultiplier;

  switch (currentMode) {
    case MODE_BREATH: {
      // Calculate current phase of the cycle [0.0, 2*PI]
      unsigned long timeInCycle = millis() % (unsigned long)effectivePeriod;
      float phase = (float)timeInCycle / effectivePeriod;
      float angle = phase * 2.0 * PI;
      
      // Calculate sine value from -1.0 to 1.0 (starting at minimum at phase 0)
      float sinVal = sin(angle - PI / 2.0);
      
      // Map to 0.0 - 1.0 using an exponential curve for smooth human eye perception
      float normalizedVal = (exp(sinVal) - 0.36787944) / 2.35040238;
      
      targetBrightness = minBrightness + (int)(normalizedVal * (maxBrightness - minBrightness));
      break;
    }
    
    case MODE_BLINK: {
      unsigned long timeInCycle = millis() % (unsigned long)effectivePeriod;
      targetBrightness = (timeInCycle < (effectivePeriod / 2.0)) ? maxBrightness : minBrightness;
      break;
    }
    
    case MODE_SOLID:
      targetBrightness = constrain(solidBrightness, minBrightness, maxBrightness);
      break;
      
    case MODE_OFF:
      targetBrightness = 0;
      break;
  }

  // Only update the duty cycle if it has changed to minimize CPU/register write overhead
  if (targetBrightness != lastBrightness) {
    analogWrite(LED_BUILTIN, targetBrightness);
    lastBrightness = targetBrightness;
  }
}
