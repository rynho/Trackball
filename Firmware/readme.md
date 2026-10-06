## Sensor Communicatoin:
- PAW3805 uses 3-wires SPI, need bit-banging when code in Arduino.
- ESP32-S2's default hardware SPI library expects two separate wires for data: MOSI (Master Out, Slave In) and MISO (Master In, Slave Out).
- The PAW3805 only has one data wire: SDIO. To talk to the sensor, ESP32-S2 must use the exact same pin to send commands (Output mode) and then quickly switch that pin to receive data (Input mode).
- Software SPI (Bit-Banging): Do not use the ESP32-S2's built-in SPI hardware engines at all. Instead, pick any 4 random digital pins (e.g., Pins 5, 6, 7, 8) and manually turn them HIGH and LOW using fast code (digitalWrite or direct register writes).

## Motion Kinematics:
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
1. Global Deck Coordinates:
	- $+\omega_x$ = Rolling the ball to the right (towards 90°E).
	- $+\omega_y$ = Rolling the ball forward (towards 180°E).
	- $+\omega_z$ = Twisting the ball clockwise (looking from above).
2. Sensor Positions on a Unit Sphere $(R=1)$:
	- Latitude $\theta$ = -60° (or 60°S).
	- Sensor 1 Longitude $\phi_1$ = 60°.
	- Sensor 2 Longitude $\phi_2$ = 150°.
3. Sensor Local Axes:
	- Assume the sensor is aligned flat against the ball's surface.
	- Local $\Delta X$ points along the line of latitude (Eastward).
	- Local $\Delta Y$ points along the line of longitude (Northward, toward the equator).

#### 2. The Exact Kinematic Equations
When the ball rotates with an angular velocity $[\omega_x, \omega_y, \omega_z]$, the surface velocity seen by a sensor at latitude $\theta$ and longitude $\phi$ is derived from the cross product of the rotation vector and the sensor's position vector.
For any sensor placed at ($\theta, \phi$), the local raw counts map precisely to the 3D rotation via these two equations:\
$\Delta X=\omega_x(-\sin \phi)+\omega_y(\cos \phi)+\omega_z(\cos \theta)$\
$\Delta Y=\omega_x(-\sin \theta \cdot \cos \phi)+\omega_y(-\sin \theta \cdot \sin \phi)$

If we plug your exact angles into these equations ($\theta$ = -60°, $\phi_1$ = 60°, $\phi_2$ = 150°), we can evaluate the sines and cosines. _Note: cos(-60°)=0.5, sin(-60°)=-0.866._

For Sensor 1 (60°E):
- $\Delta X_1 = -0.866 \omega_x + 0.5 \omega_y + 0.5 \omega_z$
- $\Delta Y_1 = 0.433 \omega_x + 0.75 \omega_y$

For Sensor 2 (150°E):
- $\Delta X_2 = -0.5 \omega_x - 0.866 \omega_y + 0.5 \omega_z$
- $\Delta Y_2 = 0.75 \omega_x - 0.433 \omega_y$

#### 3. The Inverted Matrix (Firmware Code)
To run this on the ESP32-S2, we must invert the system of equations. Since we have 4 inputs ($\Delta X_1, \Delta Y_1, \Delta X_2, \Delta Y_2$) and only 3 unknown outputs ($\omega_x, \omega_y, \omega_z$), the system is overdetermined. We use a least-squares matrix inversion to get the most accurate, mathematically balanced translation.

When solved, the exact quantitative formulas for the firmware are:\
$\omega_x = 0.433\cdot \Delta Y_1 + 0.750\cdot \Delta Y_2$
$\omega_y = 0.750\cdot \Delta Y_1 - 0.433\cdot \Delta Y_2$
$\omega_z = 2.000\cdot (\Delta X_1 + \Delta X2) + 0.732\cdot \Delta Y_1 -2.732\cdot \Delta Y_2$

#### Quantitative Insights from the Matrix:
- **The Radius/Speed Scaling**: Notice the multiplier for $\omega_z$ features a coefficient of 2.000 for the $\Delta X$ inputs. This exactly compensates for the fact that at 60°S, the radius of the latitude circle is exactly half `cos(60°)=0.5` of the ball's actual radius. The firmware scales up the Z-axis inputs by 2x to ensure twisting feels just as fast as rolling.
- **Asymmetric Y-Axis Panning**: Your intuition was entirely accurate. Because Sensor 1 sits at 60°E (closer to the 90°E X-axis), rolling the ball forward $\omega_y$ causes a massive shift in Sensor 1's local longitude line (0.750), whereas Sensor 2 sitting at 150°E is angled further away from that track, yielding a smaller relative response (-0.433).
- **Crosstalk Elimination**: The long equation for $\omega_z$ proves why simple addition isn't enough for an asymmetric layout. If you just added $\Delta X_1 + \Delta X_2$, rolling the ball diagonally would cause the cursor to "drift" or falsely trigger a twist. The trailing $\Delta Y$ subtraction terms act as a mathematical gyroscope, actively stripping out rolling artifacts from your twist calculations.

