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
const uint16_t poll_presets[] = {125, 500, 1000};
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
