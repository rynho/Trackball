## MCU Pin Mapping
1. Dual PAW3805EK Sensors (7 Pins)
The ESP32-S2 features a flexible GPIO matrix, which means it can assign any general-purpose pin to act as SPI lines.
	- Shared SCLK: GPIO 3
 	- Shared QB (MFIO): GPIO 5
	- CS Sensor A: GPIO 7
	- CS Sensor B: GPIO 9
	- SDIO Sensor A: GPIO 11
	- SDIO Sensor B: GPIO 12

2. Optical Scroll Ring Interrupters (2 Pins)
	- Phase A (Channel 1): GPIO 16
	- Phase B (Channel 2): GPIO 18

3. Primary Mouse Switches (4 Pins)
	- Left Click: GPIO 33
	- Right Click: GPIO 35
	- Middle/Scroll Click: GPIO 37
	- Forward/Back Macro: GPIO 39

4. Config
	- Leveraging an extra USB interface to configure.
	- Implement WebHID for config.

5. Caution
	- GPIO 0 is kept empty: it's wired directly to physical on-board "BOOT" button.
	- GPIO 15 is kept empty: it's tied to onboard blue status LED.
	- GPIO 18, 19, 20, 26: These are typically tied to the internal SPI flash/PSRAM.
 	- GPIO 21-32 (PSRAM/Flash): Occupied by the S2 Mini 2MB PSRAM and 4MB Flash.
	- GPIO 39: JTAG pin, avoid if doing JTAG debugging.
 	- GPIO 45, 46: Strapping pins for VDD_SPI voltage and boot log printing. Avoid using these for critical sensor lines.


## USB Interface and Configuration
Implement the trackball as a 3-channel composite device:
- Interface 0 (HID Mouse): Sends standard mouse movements (X/Y deltas) and standard clicks.
- Interface 1 (HID SpaceMouse/Joystick): A separate HID endpoint simulating a multi-axis 6DOF controller for CAD or 3D navigation.
- Interface 2 (WebHID): Custom WebHID Configurator Endpoint.

- Some config options include DPI (800, 1200, 1600, 2400), Polling Rate (125, 500, 1000), Multi-Axis (enable 6DOF control), and to "apply" and "save" the config.
- Compare to most office mouse, 1200 DPI and 125Hz polling rate would be default. Under the hood, 125Hz polling is the ceiling of Low-Speed USB data rate (USB 1.1, 1.5 Mbps), and 1,000Hz is for Full-Speed USB (USB 1.1, 12 Mbps). ESP32-S2 features the built-in USB 2.0 Full-Speed OTG.

Browser app will safely ignore Interface 0 and 1, explicitly locking onto and opening Interface 2 using its dedicated usagePage and usage. Implement "apply" to test in RAM, and include <Preferences.h> in Arduino IDE to save the config in flash storage.

### 1. Define HID Report Descriptor
To make a generic HID interface talk to WebHID, we must define a custom collection in USB report descriptor. On the ESP32-S2 (using TinyUSB underneath the Arduino core), it will need a descriptor that looks like this:
```
// Example Custom Config HID Report Descriptor
#define TUD_HID_REPORT_DESC_CONFIG(...) \
  HID_USAGE_PAGE_N ( 0xFF00, 2   ), /* Vendor-Defined Usage Page 0xFF00 */ \
  HID_USAGE        ( 0x01        ), /* Vendor-Defined Usage 0x01 */ \
  HID_COLLECTION   ( HID_COLLECTION_APPLICATION ), \
    /* Input Report: Device -> Browser */ \
    HID_REPORT_ID  ( 1 ) \
    HID_USAGE      ( 0x02        ), \
    HID_REPORT_SIZE( 8           ), \
    HID_REPORT_COUNT( 64         ), /* 64-byte chunks are standard for Full-Speed */ \
    HID_INPUT      ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
    /* Output Report: Browser -> Device */ \
    HID_REPORT_ID  ( 2 ) \
    HID_USAGE      ( 0x03        ), \
    HID_REPORT_SIZE( 8           ), \
    HID_REPORT_COUNT( 64         ), \
    HID_OUTPUT     ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
  HID_COLLECTION_END

```
Using a dedicated Vendor-Defined Usage Page (0xFF00) ensures the operating system doesn't accidentally lock the interface for exclusive OS-mouse use, leaving it completely open for the WebHID browser context.

### 2. Light Communication Protocol Design
Since you want to tweak multiple parameters (CPI presets, bInterval polling rates, and toggling the 6DOF calculation engine), structure your 64-byte payload with a simple Command/Value pairing.
Example Browser-to-Device Packets (Output Reports):
- Change CPI: [0x01, 0x02, ...rest zeros] → Command 0x01 (Set CPI), Value 0x02 (Index 2: 1500 CPI).
- Toggle 6DOF: [0x02, 0x01, ...rest zeros] → Command 0x02 (6DOF Mode), Value 0x01 (Enabled).
- Save to Flash: [0x0F, 0x00, ...rest zeros] → Command 0x0F (Commit to NVS).

### 3. Handling Runtime USB Changes (Polling Rate & 6DOF)
- CPI/DPI Tweaks: This is easy. When your WebHID thread parses a CPI command, immediately bit-bang the updated register offsets to twin PAW3805EK sensors over SPI. No USB reset required.
- Toggling 6DOF: You can keep the SpaceMouse HID interface alive globally. When `is_spacemouse_mode == false`, firmware simply stops sending reports on Interface 1. This is cleaner than tearing down the whole USB stack.
- Polling Rate (Dynamic bInterval Change): This is the tricky part. The USB polling rate is defined by the bInterval field inside the Endpoint Descriptor, which the PC reads only during enumeration. If a user alters the polling rate via WebHID, firmware must write the configuration index to <Preferences.h>, close the USB stack via USB.end(), pause, and call USB.begin() to re-enumerate the device with the new bInterval value.

### 4. Seamless Web App Architecture
Because your configurations fit perfectly into RAM-backed testing routines, you can make an incredible web UI experience:
1. Provide real-time sliders in your web GUI.
2. As the user drags a slider, fire a WebHID device.sendReport() immediately.
3. The trackball updates its active CPI variables in volatile RAM instantly. The user can test the new tracking speed while the web dashboard is open.
4. Add a clean "Save to Device Profile" button that sends the flash commit command to trigger prefs.putInt() only when they are satisfied.

---
## Sensor Communicatoin
- PAW3805 uses 3-wires SPI, need bit-banging when code in Arduino.
- ESP32-S2's default hardware SPI library expects two separate wires for data: MOSI (Master Out, Slave In) and MISO (Master In, Slave Out).
- The PAW3805 only has one data wire: SDIO. To talk to the sensor, ESP32-S2 must use the exact same pin to send commands (Output mode) and then quickly switch that pin to receive data (Input mode).
- Software SPI (Bit-Banging): Do not use the ESP32-S2's built-in SPI hardware engines at all. Instead, pick any 4 random digital pins (e.g., Pins 5, 6, 7, 8) and manually turn them HIGH and LOW using fast code (digitalWrite or direct register writes).

## PAW3805 Register
| Address | Register (Read/Write) | Defult Value | Notes |
| -- | -- | -- | -- |
| 0x00 | Prod ID1 (R) | 0x31 | PID [11:4] |
| 0x01 | Prod ID2 (R) | 0x61 | [7:4] PID [3:0], [3:0] VID [3:0] |
| 0x02 | Motion Status (R) | - | [7] Motion  1=movement registers ready.<br/> [5] DYOVF 1=Y buffer overflowed. <br/> [4] DXOVF 1=X buffer overflowed.|
| 0x0D | CPI X (RW) | - | About 27 per step.<br /> 800->0x1F, **1200**->0x2E<br /> 1600->0x3C, 2400->0x5B |
| 0x0E | CPI Y (RW) | - | About 24.5 per step.<br />800->0x22, **1200**->0x32<br /> 1600->0x43, 2400->0x64 |
| 0x03 | X Lo (RW) | - | Low byte of X movement. |
| 0x04 | Y Lo (RW) | - | Low byte of Y movement. |
| 0x11 | X Hi (RW) | - | High byte of 16-bit X movement. |
| 0x12 | Y Hi (RW) | - | High byte of 16-bit Y movement. |
| 0x05 | Operation Mode (RW) | 0xB8 | [4] Sleep1 enable=1; [3] Sleep2 enable=1 => 0xB8. |
| 0x06 | Config (RW) | 0x10 | [5] Sleep3 enable=1 => 0x30. |
| 0x09 | Write Protect (RW) | 0x00 | **Enabled** (registers after 0x09 are read-only)=0x00;<br /> Disabled=0x5A. |
| 0x19 | Orientation (RW) | 0x14 | Sensor A (90°E)= B (90°W)=0x14. |
| 0x26 | MFIO Config (RW) | 0x34 | Quick-Burst=0x1C. |
| 0x51 | LED Current (RW) | 0x0E | [4:0] * 1.2mA, 0-31 (37.2mA). Default is 16.8mA. |
| 0x61 | Frame Avg (R) | - | Average brightness within a frame (35x35), 0-255. |
| 0x13 | IQ (R) | -  | Image Quality, number of valid feature in current frame. 0-255. |
| 0x15 | Shutter (R) | - | LED shutter time, shutter*0.512+2.6us. 0-255. |

## Motion Sensor
The Optimal Least-Squares Estimation
- $\omega_x = \frac{1}{2}(S_{1y}-S_{2y}) \propto S_{1y}-S_{2y}$
- $\omega_y = \frac{\sqrt{2}}{2}(S_{1x}-S_{2x}) \propto \sqrt{2}(S_{1x}-S_{2x})$
- $\omega_z = \frac{\sqrt{2}}{2}(S_{1x}+S_{2x}) \propto \sqrt{2}(S_{1x}+S_{2x})$

```
// Define the geometric transformation constant
const float SQRT2 = 1.4142135624f; 
void calculate_global_movement(float dx1, float dy1, float dx2, float dy2,
                               float &out_wx, float &out_wy, float &out_wz) {
    out_wx = dy1 - dy2;
    out_wy = SQRT2 * (dx1 - dx2);
    out_wz = SQRT2 * (dx1 + dx2);
}

void setup() {
    Serial.begin(115200);
}

void loop() {
    // 2. Read your raw sensor delta accumulations
    float s1_dx = 12.0f; // Example values from your sensor polling logic
    float s1_dy = -4.0f;
    float s2_dx = -8.0f;
    float s2_dy = 6.0f;

    // 3. Declare standard variables to hold the outputs
    float wx = 0.0f;
    float wy = 0.0f;
    float wz = 0.0f;

    // 4. Call the function naturally! 
    // Behind the scenes, the function writes directly to wx, wy, and wz.
    calculate_global_movement(s1_dx, s1_dy, s2_dx, s2_dy, wx, wy, wz);

    // 5. Use your calculated vectors
    Serial.print("Global WX: "); Serial.println(wx);
    Serial.print("Global WY: "); Serial.println(wy);
    Serial.print("Global WZ: "); Serial.println(wz);

    delay(10); // Standard polling delay
}
```


## Dedicated Scroll Wheel
There will be an extra dedicated scroll wheel similar to Kensington Orbit or Expert. It will leverage two optical interrupter to encode the rotation, based on quadrature encoding principle.  While the two receiver channels' waveform should be digitally 90° phase apart ideally, it works as far as channel B's rising edge dropped in channel A's high state.
```
// Interrupt Service Routine (ISR) stored in RAM for speed
void IRAM_ATTR readEncoder() {
  int aState = digitalRead(ENCODER_A);
  int bState = digitalRead(ENCODER_B);
  
  // If states are equal, encoder is spinning clockwise
  if (aState == bState) {
    encoderTicks++;
  } else {
    encoderTicks--;
  }
}

void setup() {
  Serial.begin(115200);
  
  // Configure pins with internal pull-up resistors 
  // (Change to INPUT if your encoder has external pull-ups or active outputs)
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  
  // Trigger interrupt on any logic change (Rising or Falling) on Channel A
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), readEncoder, CHANGE);
}

void loop() {
  static long lastTicks = 0;
  
  // Only print when the position actually changes
  if (encoderTicks != lastTicks) {
    lastTicks = encoderTicks;
    Serial.print("Position (Ticks): ");
    Serial.println(lastTicks);
  }
  
  delay(10); // Small delay to avoid flooding the serial monitor
}
```
