#include "USB.h"
#include "USBHID.h" // Switched to generic HID subclass for raw control
#include <Preferences.h>

// --- Profile Presets ---
const uint16_t cpi_presets[] = {800, 1200, 1600, 2400}; 
const uint16_t poll_presets[] = {125, 500, 1000};
const int NUM_CPI = sizeof(cpi_presets) / sizeof(cpi_presets[0]);
const int NUM_POLL = sizeof(poll_presets) / sizeof(poll_presets[0]);

// --- State Variables ---
int saved_cpi_idx = 1;         // default to 1200 CPI.
int saved_poll_idx = 0;        // default to 125Hz polling rate.
bool saved_spacemouse_mode = false;  // default to false (no 6DOF) on first boot.

int active_cpi_idx = 1;  
int active_poll_idx = 0; 
bool is_spacemouse_mode = false;

Preferences prefs;
USBHID HIDInterface; // Unified communication pipeline interface

#define TUD_HID_REPORT_DESC_TRACKBALL(...) \
  HID_USAGE_PAGE_N ( 0xFF00, 2   ), \
  HID_USAGE        ( 0x01        ), \
  HID_COLLECTION   ( HID_COLLECTION_APPLICATION ), \
    /* Input Report: Device -> Browser */ \
    HID_REPORT_ID  ( 1 ) \
    HID_USAGE      ( 0x02        ), \
    HID_REPORT_SIZE( 8           ), \
    HID_REPORT_COUNT( 64         ), \
    HID_INPUT      ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
    /* Output Report: Browser -> Device */ \
    HID_REPORT_ID  ( 2 ) \
    HID_USAGE      ( 0x03        ), \
    HID_REPORT_SIZE( 8           ), \
    HID_REPORT_COUNT( 64         ), \
    HID_OUTPUT     ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ), \
  HID_COLLECTION_END


void apply_hardware_profiles() {
  uint16_t target_cpi = cpi_presets[active_cpi_idx];
  uint16_t target_poll = poll_presets[active_poll_idx];

  // --- TODO: Sensor Write Routines ---
  // 1. Bit-bang target_cpi down your shared-clock SPI bus to PAW3805EK registers
  // 2. Modify USB bInterval configuration mappings to enforce target_poll speeds
}

void commit_settings_to_flash() {
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
}

void handle_webhid_output_report(uint8_t report_id, uint8_t* buffer, size_t len) {
  if (len < 2) return;

  uint8_t command = buffer[0];
  uint8_t value   = buffer[1];

  switch (command) {
    case 0x01: // SET CPI (Live Test)
      if (value < NUM_CPI) {
        active_cpi_idx = value;
        apply_hardware_profiles(); 
      }
      break;

    case 0x02: // TOGGLE 6DOF (Live Test)
      is_spacemouse_mode = (value == 1);
      break;

    case 0x03: // STAGE POLLING RATE (Flash only)
      if (value < NUM_POLL) {
        active_poll_idx = value; 
      }
      break;

    case 0x0F: // COMMIT TO FLASH & REBOOT
      commit_settings_to_flash(); 
      USB.end();
      delay(1000); 
      ESP.restart(); 
      break;
  }
}

void setup() {
  // 1. Initialize NVS storage parameters
  prefs.begin("trackball", false);
  saved_cpi_idx = prefs.getInt("cpi_idx", 1);
  saved_poll_idx = prefs.getInt("poll_idx", 0);
  saved_spacemouse_mode = prefs.getBool("space_mode", false);

  // 2. Synchronize active volatile properties
  active_cpi_idx = saved_cpi_idx;
  active_poll_idx = saved_poll_idx;
  is_spacemouse_mode = saved_spacemouse_mode;

  // 3. Register custom multi-interface WebHID profile
  HIDInterface.begin();
  HIDInterface.appendDescriptor(TUD_HID_REPORT_DESC_TRACKBALL);
  
  // 4. Fire up the native hardware profile pipeline
  apply_hardware_profiles();
  USB.begin();
}

void loop() {
  // High-performance trackball processing pipeline engine runs continuous loops
  run_high_performance_trackball_pipeline();
}

void run_high_performance_trackball_pipeline() {
  // Your 1,000 Hz tracking operations run continuously here across both routing modes:
  // 1. Fetch dual PAW3805EK locations using your shared clock pin (GPIO7)
  // 2. Decode quadrature state structures on Phase A/B lines for your scroll ring
  // 3. Assemble and dispatch HID packages over the native USB pipe every 1 ms
}
