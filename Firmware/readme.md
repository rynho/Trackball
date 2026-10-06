## Sensor Communicatoin:
- PAW3805 uses 3-wires SPI, need bit-banging when code in Arduino.
- ESP32-S2's default hardware SPI library expects two separate wires for data: MOSI (Master Out, Slave In) and MISO (Master In, Slave Out).
- The PAW3805 only has one data wire: SDIO. To talk to the sensor, ESP32-S2 must use the exact same pin to send commands (Output mode) and then quickly switch that pin to receive data (Input mode).
- Software SPI (Bit-Banging): Do not use the ESP32-S2's built-in SPI hardware engines at all. Instead, pick any 4 random digital pins (e.g., Pins 5, 6, 7, 8) and manually turn them HIGH and LOW using fast code (digitalWrite or direct register writes).

## Motion Kinematics:
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

### The Kinematics Summary
By placing them precisely 90° apart on the same latitude, the ESP32-S2 firmware can compute the 3D navigation using simple addition and subtraction before sending it over USB:
1. Desk Cursor $X = \Delta Y_1 + \Delta Y_2$
2. Desk Cursor $Y = \Delta Y_1 - \Delta Y_2$
3. CAD Twist $Z = \Delta X_1 + \Delta X_2$
