## MCU Pin Mapping
1. Dual PAW3805EK Sensors (5 Pins)
The ESP32-S2 features a flexible GPIO matrix, which means you can assign any general-purpose pin to act as your SPI lines.
	- Shared SCLK: GPIO7 (Default hardware SPI clock)
	- Sensor 1 (Trackball Ball): GPIO11 (SDIO_1) + GPIO12 (CS_1)
	- Sensor 2 (Scroll Ring Backing): GPIO9 (SDIO_2) + GPIO10 (CS_2)

2. Primary Mouse Switches (4 Pins)
	- Left Click: GPIO33
	- Right Click: GPIO34
	- Middle/Scroll Click: GPIO35
	- Forward/Back Macro: GPIO36

3. Optical Scroll Ring Interrupters (2 Pins)
	- Phase A (Channel 1): GPIO13
	- Phase B (Channel 2): GPIO14

4. Config Trigger Buttons (1 Pin)
	- Leveraging an extra USB channel to configure instead of hardware buttons.
	- Implement a trigger button (GPIO2) to switch between two config mode: office and home.
 	- Office mode leverages CDC-ACM (virtual serial) to avoid security concern.
  - Home mode implements RNDIS (virtual ethernet) to allow web GUI.
  - Some config options are DPI (400, 800, 1600, 3000), Polling Rate (125, 250, 500, 1000), Multi-Axis (enable 6DOF control), and to "apply" and "save" the config.

5. Caution
	- GPIO0 is kept empty: it's wired directly to physical on-board "BOOT" button.
	- GPIO15 is kept empty: it's tied to onboard blue status LED.

## USB Interface
Implement the trackball as a 3-channel composite device:
- Channel 1 (HID Mouse): Sends standard mouse movements (X/Y deltas) and standard clicks.
- Channel 2 (HID SpaceMouse/Joystick): A separate HID endpoint simulating a multi-axis 6DOF controller for CAD or 3D navigation.
- Channel 3 Configuration.
	- In Office mode (USB CDC-ACM / Virtual COM Port): A standard serial communications port used to host the text-based configuration menu.
	- In Home mode (USB RNDIS): The virtual network interface initializes instantly over the wire. You open your browser, navigate to your crisp local dashboard page (e.g., http://192.168.7.1), click your settings, and save.
	- In both case, implement "apply" to test in RAM, and include <Preferences.h> in Arduino IDE to save the config in flash storage.

```
#include "USB.h"
#include "USBHIDMouse.h" // Replace with your compound HID/SpaceMouse stack down the line
#include "USBCDC.h"
#include "USBNetwork.h"
#include <WebServer.h>
#include <Preferences.h>

// --- Pin Assignments ---
const int CONFIG_BTN_PIN = 2; // Switched to GPIO2 as requested

// --- Profile Presets ---
const uint16_t cpi_presets[] = {500, 1000, 1500, 3000}; // Added 1000 CPI
const uint16_t poll_presets[] = {125, 250, 500, 1000};      // Added 250 Hz
const int NUM_CPI = sizeof(cpi_presets) / sizeof(cpi_presets[0]);
const int NUM_POLL = sizeof(poll_presets) / sizeof(poll_presets[0]);

// --- Profile State Vectors ---
// Stored = what's in Flash | Active = what the hardware is running right now
int saved_cpi_idx = 1;         // Default to index 1 (1000 CPI) on first boot
int saved_poll_idx = 0;        // Default to index 0 (125 Hz) on first boot
bool saved_spacemouse_mode = false; // Default to false (no 6DOF) on first boot

int active_cpi_idx = 1;  
int active_poll_idx = 0; 
bool is_spacemouse_mode = false;

// --- State Machine & Debounce ---
enum USBProfile { PROFILE_CLI, PROFILE_GUI }; // Renamed to CLI and GUI
USBProfile current_usb_profile = PROFILE_CLI;

const unsigned long HOLD_TIME_MS = 2000; 
unsigned long btn_press_start_time = 0;
bool btn_was_pressed = false;

Preferences prefs;
WebServer server(80); 

// Global composite USB class interfaces
// Channel 1 & 2: Mounted via the HID Subsystem (Standard Mouse + Multi-Axis SpaceMouse)
USBHIDMouse MouseInterface; 
// Note: Down the line, you will combine this with a custom report descriptor 
// to expose Interface 0 (Mouse) and Interface 1 (SpaceMouse/Joystick) simultaneously.

// Channel 3 (Profile 1 Option): Virtual Serial COM Line
USBCDC SerialInterface; 

// Channel 3 (Profile 2 Option): Virtual Network Card
USBNetwork NetworkInterface; 

// Local IP endpoints for over-the-wire browser routing
IPAddress local_IP(192, 168, 7, 1);
IPAddress gateway(192, 168, 7, 1);
IPAddress subnet(255, 255, 255, 0);

void apply_hardware_profiles() {
  uint16_t target_cpi = cpi_presets[active_cpi_idx];
  uint16_t target_poll = poll_presets[active_poll_idx];

  // --- TODO: Sensor Write Routines ---
  // 1. Bit-bang target_cpi down your shared-clock SPI bus to PAW3805EK registers
  // 2. Modify USB bInterval configuration mappings to enforce target_poll speeds
}

void commit_settings_to_flash() {
  // Software Value Filtering: Only issue writes if current runtime attributes diverge from flash records
  bool data_changed = false;

  if (active_cpi_idx != saved_cpi_idx) {
    saved_cpi_idx = active_cpi_idx;
    prefs.putInt("cpi_idx", saved_cpi_idx);
    data_changed = true;
  }
  if (active_poll_idx != saved_poll_idx) {
    saved_poll_idx = active_poll_idx;
    prefs.putInt("poll_idx", saved_poll_idx);
    data_changed = true;
  }
  if (is_spacemouse_mode != saved_spacemouse_mode) {
    saved_spacemouse_mode = is_spacemouse_mode;
    prefs.putBool("space_mode", saved_spacemouse_mode);
    data_changed = true;
  }

  if (data_changed) {
    Serial.println("[NVS] Configuration state modified. Changes successfully written to Flash.");
  } else {
    Serial.println("[NVS] Run states match storage bounds. Flash write bypassed to preserve sectors.");
  }
}

// --- HTML Configuration Webpage ---
void handle_root_route() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>body{font-family:sans-serif; background:#121212; color:#fff; text-align:center; padding:20px;}";
  html += "select, button{padding:12px; margin:10px; width:80%; max-width:300px; border-radius:6px; border:none; font-size:16px;}";
  html += ".btn-apply{background:#17a2b8; color:#fff; font-weight:bold; cursor:pointer;}";
  html += ".btn-save{background:#28a745; color:#fff; font-weight:bold; cursor:pointer;}</style>";
  html += "<title>Trackball Profile GUI</title></head><body>";
  html += "<h2>Trackball Profile GUI Portal</h2>";
  html += "<form method='POST'>";
  
  // CPI Selector
  html += "<label>Resolution (CPI):</label><br><select name='cpi'>";
  for(int i=0; i<NUM_CPI; i++) {
    html += "<option value='" + String(i) + "'" + (i == active_cpi_idx ? " selected" : "") + ">" + String(cpi_presets[i]) + " CPI</option>";
  }
  html += "</select><br><br>";

  // Polling Selector
  html += "<label>Polling Rate:</label><br><select name='poll'>";
  for(int i=0; i<NUM_POLL; i++) {
    html += "<option value='" + String(i) + "'" + (i == active_poll_idx ? " selected" : "") + ">" + String(poll_presets[i]) + " Hz</option>";
  }
  html += "</select><br><br>";

  // SpaceMouse Selector
  html += "<label>Multi-Axis Mode:</label><br><select name='spacemouse'>";
  html += "<option value='0'" + String(!is_spacemouse_mode ? " selected" : "") + ">Standard Mouse</option>";
  html += "<option value='1'" + String(is_spacemouse_mode ? " selected" : "") + ">SpaceMouse Simulation (6DOF)</option>";
  html += "</select><br><br>";

  html += "<button type='submit' formaction='/apply' class='btn-apply'>Apply & Test (RAM Only)</button><br>";
  html += "<button type='submit' formaction='/save' class='btn-save'>Save Permanently (Flash)</button>";
  html += "</form></body></html>";
  server.send(200, "text/html", html);
}

void handle_web_apply() {
  if (server.hasArg("cpi") && server.hasArg("poll") && server.hasArg("spacemouse")) {
    active_cpi_idx = server.arg("cpi").toInt();
    active_poll_idx = server.arg("poll").toInt();
    is_spacemouse_mode = server.arg("spacemouse").toInt() == 1;

    apply_hardware_profiles(); // Execution adjustments in RAM without writing to Flash
    handle_root_route();       // Reload screen with active properties
  }
}

void handle_web_save() {
  if (server.hasArg("cpi") && server.hasArg("poll") && server.hasArg("spacemouse")) {
    active_cpi_idx = server.arg("cpi").toInt();
    active_poll_idx = server.arg("poll").toInt();
    is_spacemouse_mode = server.arg("spacemouse").toInt() == 1;

    apply_hardware_profiles();
    commit_settings_to_flash(); // Run matching optimization checks and store variables

    String response = "<html><body><h2>Settings Saved Successfully!</h2><p>Safe in NVS flash. Switch profile button when ready.</p></body></html>";
    server.send(200, "text/html", response);
  }
}

void handle_serial_cli() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('
');
    command.trim();

    if (command.startsWith("SET_CPI ")) {
      int val = command.substring(8).toInt();
      for (int i = 0; i < NUM_CPI; i++) {
        if (cpi_presets[i] == val) {
          active_cpi_idx = i;
          Serial.println(">> Target CPI prepared in RAM.");
          return;
        }
      }
      Serial.println(">> Invalid CPI value.");
    } 
    else if (command.startsWith("SET_POLL ")) {
      int val = command.substring(9).toInt();
      for (int i = 0; i < NUM_POLL; i++) {
        if (poll_presets[i] == val) {
          active_poll_idx = i;
          Serial.println(">> Target Polling prepared in RAM.");
          return;
        }
      }
      Serial.println(">> Invalid Polling value.");
    }
    else if (command.startsWith("SET_6DOF ")) {
      int val = command.substring(9).toInt();
      is_spacemouse_mode = (val == 1);
      Serial.println(">> Target 6DOF mode prepared in RAM.");
    }
    else if (command.equals("APPLY")) {
      apply_hardware_profiles();
      Serial.println(">> Profile applied to hardware layers (RAM only).");
    } 
    else if (command.equals("SAVE")) {
      apply_hardware_profiles();
      commit_settings_to_flash();
      Serial.println(">> Parameters locked securely to NVS Flash storage.");
    }
  }
}

void switch_usb_stack_runtime(USBProfile target_mode) {
  USB.end();
  delay(500); // Disconnect settling pause

  if (target_mode == PROFILE_GUI) {
    current_usb_profile = PROFILE_GUI;
    
    // --- BINDING PROFILE GUI (Mouse + SpaceMouse + RNDIS Network Portal) ---
    USB.PID(0x4002); 
    USB.productName("Trackball GUI");

    NetworkInterface.begin(local_IP, gateway, subnet);
    server.begin();
    
    USB.begin(); 
  } else {
    current_usb_profile = PROFILE_CLI;
    
    // --- BINDING PROFILE CLI (Mouse + SpaceMouse + CDC-ACM Serial) ---
    server.stop();
    
    USB.PID(0x4001); 
    USB.productName("Trackball CLI");

    SerialInterface.begin(115200);
    MouseInterface.begin();
    
    USB.begin();
  }
}

void setup() {
  pinMode(CONFIG_BTN_PIN, INPUT_PULLUP);

  // Pull configuration attributes from NVS memory pools
  prefs.begin("trackball", false);
  
  // Note: Defaults specify index 1 (1000 CPI), index 0 (125 Hz), and false (no 6DOF) for first boot
  saved_cpi_idx = prefs.getInt("cpi_idx", 1);    
  saved_poll_idx = prefs.getInt("poll_idx", 0);  
  saved_spacemouse_mode = prefs.getBool("space_mode", false);

  // Initialize runtime variables using values pulled from storage
  active_cpi_idx = saved_cpi_idx;
  active_poll_idx = saved_poll_idx;
  is_spacemouse_mode = saved_spacemouse_mode;

  // Configure Profile 1 (Default Whitelisted Identity: Profile CLI)
  USB.VID(0x303A); // Standard Espressif VID (or use a generic whitelisted one)
  USB.PID(0x4001); 
  USB.manufacturer("Custom Trackball");
  USB.productName("Profile CLI");

  // Mount composite elements
  SerialInterface.begin(115200); 
  MouseInterface.begin();

  apply_hardware_profiles();

  // Always force standard Profile 1 (Profile CLI) on hardware power-up/replug
  current_usb_profile = PROFILE_CLI;
  USB.begin();
}

void loop() {
  unsigned long current_time = millis();

  // --- Dynamic Switch Monitoring Loop (GPIO2) ---
  if (digitalRead(CONFIG_BTN_PIN) == LOW) {
    if (!btn_was_pressed) {
      btn_press_start_time = current_time;
      btn_was_pressed = true;
    } else if (current_time - btn_press_start_time > HOLD_TIME_MS) {
      btn_was_pressed = false; 
      
      if (current_usb_profile == PROFILE_CLI) {
        switch_usb_stack_runtime(PROFILE_GUI);
      } else {
        switch_usb_stack_runtime(PROFILE_CLI);
      }
    }
  } else {
    btn_was_pressed = false;
  }

  // --- Execution Routine Selection Mapping ---
  if (current_usb_profile == PROFILE_GUI) {
    server.handleClient();
  } else {
    handle_serial_cli();
  }

  // Pure High-Performance Loop Pipeline (1,000 Hz structural block target)
  run_high_performance_trackball_pipeline();
}

void run_high_performance_trackball_pipeline() {
  // Your 1,000 Hz tracking operations run continuously here across both routing modes:
  // 1. Fetch dual PAW3805EK locations using your shared clock pin (GPIO7)
  // 2. Decode quadrature state structures on Phase A/B lines for your scroll ring
  // 3. Assemble and dispatch HID packages over the native USB pipe every 1 ms
}
```

## Sensor Communicatoin
- PAW3805 uses 3-wires SPI, need bit-banging when code in Arduino.
- ESP32-S2's default hardware SPI library expects two separate wires for data: MOSI (Master Out, Slave In) and MISO (Master In, Slave Out).
- The PAW3805 only has one data wire: SDIO. To talk to the sensor, ESP32-S2 must use the exact same pin to send commands (Output mode) and then quickly switch that pin to receive data (Input mode).
- Software SPI (Bit-Banging): Do not use the ESP32-S2's built-in SPI hardware engines at all. Instead, pick any 4 random digital pins (e.g., Pins 5, 6, 7, 8) and manually turn them HIGH and LOW using fast code (digitalWrite or direct register writes).

## Geometry Kinematics
### A. Qualitative Analysis
#### Movement 1: Pure Twist (Z-Axis Rotation)
Imagine twisting the ball clockwise (spinning it like a top).
- Because both sensors are sitting at the exact same distance from the equator (60°S), the ball's surface slides sideways over both windows at the exact same speed.
- The surface moves left-to-right past Sensor 1, and left-to-right past Sensor 2.
- The Math: Both sensors register a pure horizontal shift in their local X-axes:\
  $\Delta X_1=\Delta X_2$
  
  Therefore, to isolate Twist (Z), you look for when their X-axes move in perfect unison:\
  $Z=\Delta X_1+\Delta X_2$

#### Movement 2: Pure Roll Forward/Backward (Desk Y-Axis)
Imagine rolling the top of the ball away from your wrist (toward 180°E).
- The ball is rotating around an axis that runs from 90°E to 270°E.
- Sensor 1 (at 60°E) is sitting very close to this rolling path. The surface moves almost vertically straight across its lens. It registers a large movement in its local Y-axis $\Delta Y_1$.
- Sensor 2 (at 150°E) is sitting on the other side of the ball. It also sees the surface moving vertically across its lens, but in the opposite direction relative to its orientation. It registers a large negative movement $\Delta Y_2$.
- The Math: To calculate pure forward/backward desk movement, you subtract their local Y deltas (which filters out any uniform tilting):\
  $Y_{desk}=\Delta Y_1-\Delta Y_2$

#### Movement 3: Pure Tilt Left/Right (Desk X-Axis)
Imagine rolling the top of the ball to the right (toward 90°E).
- Now, the surface behavior flips. Because the sensors are 90 degrees apart in longitude, the lateral and vertical components swap traits.
- The rotation causes Sensor 1's local X-axis and Sensor 2's local X-axis to fight each other (one moves left, one moves right), while their Y-axes move together.
- The Math: To extract the true left/right panning of your desk mouse, the formula combines the remaining differences:\
  $X_{desk}=\Delta Y_1+\Delta Y_2$

#### Summary
By placing them precisely 90° apart on the same latitude, the ESP32-S2 firmware can compute the 3D navigation using simple addition and subtraction before sending it over USB:
1. Desk Cursor $X = \Delta Y_1 + \Delta Y_2$
2. Desk Cursor $Y = \Delta Y_1 - \Delta Y_2$
3. CAD Twist $Z = \Delta X_1 + \Delta X_2$


### B. Quantitative Calculation
To make the trackball accurate in a quantitative sense, it needs to map the raw sensor readings to the true 3D angular velocity vector of the ball $\vec{\omega} = [\omega_x, \omega_y, \omega_z]^T$ using a precise kinematic transformation matrix.
Let's break down the exact mathematics for your specific layout.
#### 1. Establishing the Coordinate Systems
1. Global Deck Coordinates (Cartesian coordinate, right handed):
	- X axis: pointing and increasing out towards viewer on horizontal plane.
	- Y axis: pointing and increasing to right on horizontal plane.
	- Z axis: pointing and increasing up. 
	- $+\omega_x$ = Rolling the ball to the left around X-axis (towards 270°E).
	- $+\omega_y$ = Rolling the ball backward around Y-axis (towards 0°E).
	- $+\omega_z$ = Twisting the ball counter-clockwise around Z-axis (looking from above).
	- Equator is latitude 0°, increasing upward, North pole is 90°, South pole is -90°.
	- X axis direction is longitude 0° (Prime Meridian), towards wrist.
3. Sensor Positions on a Unit Sphere $(R=1)$:
	- Latitude $\theta$ = -60° (or 60°S).
	- Sensor 1 Longitude $\phi_1$ = 60°.
	- Sensor 2 Longitude $\phi_2$ = 150°.
4. Sensor Local Axes:
	- Assume the sensor is aligned flat against the ball's surface.
	- Local $\Delta X$ points along the line of latitude (Eastward).
	- Local $\Delta Y$ points along the line of longitude (Northward, toward the equator).

#### 2. The Exact Kinematic Equations
When the ball rotates with an angular velocity $[\omega_x$, $\omega_y$, $\omega_z]$, the surface velocity seen by a sensor at latitude $\theta$ and longitude $\phi$ is derived from the cross product of the rotation vector and the sensor's position vector.
For any sensor placed at ($\theta, \phi$), the local raw counts map precisely to the 3D rotation via these two equations:\
$\Delta X = R [\omega_x(\sin\theta \cdot \cos\phi) + \omega_y(\sin\theta \cdot \sin\phi) + \omega_z(-\cos\theta)]$\
$\Delta Y = R [\omega_x(-\sin\phi) + \omega_y(\cos\phi)]$

If we plug your exact angles into these equations ($\theta$=-60°, $\phi_1$=60°, $\phi_2$=150°), we can evaluate the sines and cosines.

For Sensor 1 (-60°S, 60°E):
- $\Delta X_1 = -\frac{\sqrt{3}}{4}\omega_x - \frac{3}{4}\omega_y - \frac{1}{2}\omega_z$
- $\Delta Y_1 = -\frac{\sqrt{3}}{2}\omega_x + \frac{1}{2}\omega_y$

For Sensor 2 (-60°S, 150°E):
- $\Delta X_2 = \frac{3}{4}\omega_x - \frac{\sqrt{3}}{4}\omega_y - \frac{1}{2}\omega_z$
- $\Delta Y_2 = -\frac{1}{2}\omega_x - \frac{\sqrt{3}}{2}\omega_y$

#### 3. The Inverted Matrix (Firmware Code)
To run this on the ESP32-S2, we must invert the system of equations. Since we have 4 inputs ($\Delta X_1$, $\Delta Y_1$, $\Delta X_2$, $\Delta Y_2$) and only 3 unknown outputs ($\omega_x, \omega_y, \omega_z$), the system is overdetermined. We use a least-squares matrix inversion to get the most accurate, mathematically balanced translation.

When solved, the exact quantitative formulas for the firmware are:\
$\omega _x=-\frac{\sqrt{3}}{2} \Delta Y_1 - \frac{1}{2} \Delta Y_2$\
$\omega _y=\frac{1}{2} \Delta Y_1 - \frac{\sqrt{3}}{2} \Delta Y_2$\
$\omega _z=-\Delta X_1 - \Delta X_2 - \frac{\sqrt{3}}{2} \Delta Y_1 + \frac{\sqrt{3}}{2} \Delta Y_2$

#### Quantitative Insights from the Matrix:
- **The Radius/Speed Scaling**: Notice the multiplier for $\omega_z$ features a coefficient of 1 for the $\Delta X$ inputs. This compensates for the fact that at 60°S, the radius of the latitude circle is exactly half `cos(60°)=0.5` of the ball's actual radius. The firmware scales up the Z-axis inputs by 2x to ensure twisting feels just as fast as rolling.
- **Crosstalk Elimination**: The long equation for $\omega_z$ proves why simple addition isn't enough for an asymmetric layout. If you just added $\Delta X_1 + \Delta X_2$, rolling the ball diagonally would cause the cursor to "drift" or falsely trigger a twist. The trailing $\Delta Y$ subtraction terms act as a mathematical gyroscope, actively stripping out rolling artifacts from your twist calculations.

```
// Define the geometric transformation constant
const float SQRT3_DIV2 = 0.8660254038f; 

void calculate_global_movement(float dx1, float dy1, float dx2, float dy2, 
                                float *out_wx, float *out_wy, float *out_wz) {
    // 1. Pre-calculate common scaled variables to save clock cycles
    float scale_dy1 = SQRT3_DIV2 * dy1;
    float scale_dy2 = SQRT3_DIV2 * dy2;

    // 2. Solve for X and Y angular velocities (decoupled dy method)
    *out_wx = - scale_dy1 - (0.5f * dy2);
    *out_wy = (0.5f * dy1) - scale_dy2;

    // 3. Solve for Z twist using a balanced average from both sensors
    *out_wz = - dx1 - dx2 - scale_dy1 + scale_dy2;
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
