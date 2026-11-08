// 1. Connecting to Your Trackball
// To open communication with device, call the browser's native HID navigator.
// It must target the exact vendor-defined usage page (0xFF00) put in the 
//  microcontroller descriptor so the browser targets the configuration channel
//  instead of the mouse channel.

let device = null;

async function connectTrackball() {
  try {
    // Request permission from user
    const devices = await navigator.hid.requestDevice({
      filters: [{ usagePage: 0xFF00, usage: 0x01 }]
    });
    
    if (devices.length === 0) return;
    device = devices[0];
    await device.open();
    console.log("Trackball Config Channel Connected!");
  } catch (error) {
    console.error("Connection failed:", error);
  }
}

// Note: Due to security sandbox rules, the browser requires that 
//  connectTrackball() is triggered directly by a real user interaction,
//  like clicking a <button id="connectBtn">Connect Trackball</button>.

// 2. Live Configuration Commands (RAM Only)
// When user interacts with sliders or toggles on your dashboard UI,
//  it will use device.sendReport(). The first number is the Output
//  Report ID (2), followed by a 64-byte array containing your custom
//  command and value mapping.

// To Live Change CPI:
async function sendCpiLive(cpiIndex) {
  if (!device) return;
  // Command 0x01 = CPI Selection
  const payload = new Uint8Array(64);
  payload[0] = 0x01; 
  payload[1] = cpiIndex; 
  
  await device.sendReport(2, payload);
}

// To Live Toggle 6DOF Mode:
async function send6DoFToggle(isEnabled) {
  if (!device) return;
  // Command 0x02 = 6DOF Engine Setup
  const payload = new Uint8Array(64);
  payload[0] = 0x02;
  payload[1] = isEnabled ? 1 : 0;
  
  await device.sendReport(2, payload);
}

// 3. Staging and Saving (The Restart Sequence)
// For properties that can't change mid-air, it stages the setting and
//  then issue a permanent lock request.

// To Stage Polling Rate:
async function stagePollingRate(pollIndex) {
  if (!device) return;
  // Command 0x03 = Stage Polling
  const payload = new Uint8Array(64);
  payload[0] = 0x03;
  payload[1] = pollIndex;
  
  await device.sendReport(2, payload);
}

// To Burn to Flash and Reboot:
async function commitAndReboot() {
  if (!device) return;
  // Command 0x0F = Save & Reset Device
  const payload = new Uint8Array(64);
  payload[0] = 0x0F;
  
  await device.sendReport(2, payload);
  console.log("Save complete. Device is rebooting...");
  device = null; // Clear connection as USB tears down
}

