#include <Arduino.h>
#include "USB.h"
#include "USBHIDMouse.h"

// Pin Configuration (ESP32-S2 Mini Layout)
#define PIN_SCLK 33
#define PIN_SDIO 18
#define PIN_NCS  16
#define PIN_MFIO 35

// Datasheet Register Mapping
#define REG_PROD_ID1         0x00  // Expected return value: 0x31
#define REG_PROD_ID2         0x01  // Expected return value: 0x61
#define REG_MOTION_STATUS    0x02  // Bit 7 is Active Motion Flag
#define REG_DELTA_X_LO       0x03  
#define REG_DELTA_Y_LO       0x04  
#define REG_DELTA_X_HI       0x11  
#define REG_DELTA_Y_HI       0x12  

#define REG_WRITE_PROTECT    0x09  // Write 0x5A to disable, 0x00 to enable
#define REG_LED_CURRENT      0x51  // RW: Bits [4:0] for current level
#define REG_IQ               0x13  // Read-Only: Image Quality (0-255)
#define REG_SHUTTER          0x15  // Read-Only: Shutter time multiplier
#define REG_FRAME_AVG        0x61  // Read-Only: Frame average brightness

// Instantiate the ESP32 Native USB Mouse Stack
USBHIDMouse Mouse;

// Function Signatures
void writeRegister(uint8_t reg, uint8_t data);
uint8_t readRegister(uint8_t reg);
bool initPAW3805();
void sendRelativeMove(int16_t dx, int16_t dy);

uint8_t currentLedStep = 0x0E;     // Tracks active step (Default 14 = 16.8mA)
unsigned long lastTuningPrint = 0; // Timer tracking variable


void setup() {
  Serial.begin(115200);

  // Initialize line protocols
  pinMode(PIN_SCLK, OUTPUT);
  pinMode(PIN_NCS, OUTPUT);
  pinMode(PIN_MFIO, INPUT_PULLUP);
  
  digitalWrite(PIN_NCS, HIGH);  
  digitalWrite(PIN_SCLK, HIGH); // SPI Mode 3 base standard state

  delay(50); // Power rail stabilization wait 
  
  // Establish USB Peripheral Layer Connection
  Mouse.begin();
  USB.begin();

  if (initPAW3805()) {
    Serial.println("[SUCCESS] PAW3805EK-CJV1 Online. HID Mouse Engine Initialized.");
  } else {
    Serial.println("[CRITICAL] Identity check verification failed. System Halted.");
    while(1);
  }
}

void loop() {
  uint8_t motionState = readRegister(REG_MOTION_STATUS);
  
  if (motionState & 0x80) {
    uint8_t xLo = readRegister(REG_DELTA_X_LO);
    uint8_t yLo = readRegister(REG_DELTA_Y_LO);
    uint8_t xHi = readRegister(REG_DELTA_X_HI);
    uint8_t yHi = readRegister(REG_DELTA_Y_HI);

    int16_t deltaX = (int16_t)((xHi << 8) | xLo);
    int16_t deltaY = (int16_t)((yHi << 8) | yLo);

    if (deltaX != 0 || deltaY != 0) {
      sendRelativeMove(deltaX, deltaY);
    }
  }
  
  // Parse any commands coming from your computer terminal
  handleSerialCommands();

  // Print diagnostic metrics to console every 250 milliseconds
  if (millis() - lastTuningPrint >= 250) {
    lastTuningPrint = millis();
    printDiagnosticDashboard();
  }
  
  delay(1); 
}


// Breaks 16-bit values into sequential 8-bit mouse movements
void sendRelativeMove(int16_t dx, int16_t dy) {
  while (dx != 0 || dy != 0) {
    int8_t moveX = 0;
    int8_t moveY = 0;

    // Segment X channel displacements
    if (dx > 127)       { moveX = 127;  dx -= 127; }
    else if (dx < -127) { moveX = -127; dx += 127; }
    else                { moveX = dx;   dx = 0;    }

    // Segment Y channel displacements
    if (dy > 127)       { moveY = 127;  dy -= 127; }
    else if (dy < -127) { moveY = -127; dy += 127; }
    else                { moveY = dy;   dy = 0;    }

    Mouse.move(moveX, moveY);
  }
}

void writeRegister(uint8_t reg, uint8_t data) {
  reg |= 0x80; // Write rule flag enforcement

  pinMode(PIN_SDIO, OUTPUT);
  digitalWrite(PIN_NCS, LOW);
  delayMicroseconds(1);

  for (int i = 7; i >= 0; i--) {
    digitalWrite(PIN_SCLK, LOW);
    digitalWrite(PIN_SDIO, (reg >> i) & 0x01);
    delayMicroseconds(1);
    digitalWrite(PIN_SCLK, HIGH);
    delayMicroseconds(1);
  }

  for (int i = 7; i >= 0; i--) {
    digitalWrite(PIN_SCLK, LOW);
    digitalWrite(PIN_SDIO, (data >> i) & 0x01);
    delayMicroseconds(1);
    digitalWrite(PIN_SCLK, HIGH);
    delayMicroseconds(1);
  }

  delayMicroseconds(5); 
  digitalWrite(PIN_NCS, HIGH);
}

uint8_t readRegister(uint8_t reg) {
  reg &= 0x7F; // Read rule flag enforcement
  uint8_t value = 0;

  pinMode(PIN_SDIO, OUTPUT);
  digitalWrite(PIN_NCS, LOW);
  delayMicroseconds(1);

  for (int i = 7; i >= 0; i--) {
    digitalWrite(PIN_SCLK, LOW);
    digitalWrite(PIN_SDIO, (reg >> i) & 0x01);
    delayMicroseconds(1);
    digitalWrite(PIN_SCLK, HIGH);
    delayMicroseconds(1);
  }

  pinMode(PIN_SDIO, INPUT_PULLUP); 
  delayMicroseconds(35); // Required tSRAD turnaround step

  for (int i = 7; i >= 0; i--) {
    digitalWrite(PIN_SCLK, LOW);
    delayMicroseconds(1); 
    digitalWrite(PIN_SCLK, HIGH); 
    if (digitalRead(PIN_SDIO)) {
      value |= (1 << i);
    }
    delayMicroseconds(1);
  }

  digitalWrite(PIN_NCS, HIGH);
  return value;
}

bool initPAW3805() {
  digitalWrite(PIN_NCS, HIGH); delay(5);
  digitalWrite(PIN_NCS, LOW);  delay(5);
  digitalWrite(PIN_NCS, HIGH); delay(10);

  uint8_t id1 = readRegister(REG_PROD_ID1);
  uint8_t id2 = readRegister(REG_PROD_ID2);
  
  Serial.printf("Bus ID Check -> Received ID1: 0x%02X | ID2: 0x%02X\n", id1, id2);

  return (id1 == 0x31 && id2 == 0x61);
}

void handleSerialCommands() {
  if (Serial.available() > 0) {
    String inputStr = Serial.readStringUntil('\n');
    inputStr.trim();
    if (inputStr.length() == 0) return;

    int targetStep = currentLedStep;
    if (inputStr == "+")      targetStep++;
    else if (inputStr == "-") targetStep--;
    else {
      int parsedValue = inputStr.toInt();
      if (parsedValue == 0 && inputStr != "0") return; // Filter out bad text inputs
      targetStep = parsedValue;
    }

    if (targetStep >= 0 && targetStep <= 25) {
      currentLedStep = (uint8_t)targetStep;
      
      // Execute the Write-Protect bypass chain
      writeRegister(REG_WRITE_PROTECT, 0x5A); // 1. Unlock registers
      writeRegister(REG_LED_CURRENT, currentLedStep); // 2. Modify LED driver
      writeRegister(REG_WRITE_PROTECT, 0x00); // 3. Re-lock protection
      
      Serial.printf("--> [UPDATED] Step: %d | Current: %.1f mA\n", currentLedStep, currentLedStep * 1.2);
    } else {
      Serial.println("--> [BLOCKED] Target step exceeds safe testing ceiling (Max Step 25 / 30mA).");
    }
  }
}

void printDiagnosticDashboard() {
  uint8_t rawFrameAvg = readRegister(REG_FRAME_AVG);
  uint8_t rawIQ       = readRegister(REG_IQ);
  uint8_t rawShutter  = readRegister(REG_SHUTTER);

  // Convert raw shutter ticks to real-world microseconds
  float exposureUs = (rawShutter * 0.512) + 2.6;

  Serial.printf("LED: %.1fmA (Step %d) | FrameAvg: %d/255 | IQ: %d/255 | Shutter: %.2fus\n", 
                currentLedStep * 1.2, currentLedStep, rawFrameAvg, rawIQ, exposureUs);
}
