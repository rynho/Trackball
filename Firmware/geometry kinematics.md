# Geometry Kinematics
## A. Qualitative Analysis
### Movement 1: Pure Twist (Z-Axis Rotation)
Imagine twisting the ball counter-clockwise (spinning it like a top).
- Because both sensors are sitting at the exact same distance from the equator (60°S), the ball's surface slides sideways over both windows at the exact same speed.
- The surface moves right-to-left past Sensor 1 (90°W) and Sensor 2 (90°E).
- The Math: Both sensors register a pure horizontal shift in their local X-axes:\
  $\Delta X_1=\Delta X_2$
  
  Therefore, to isolate Twist (Z), you look for when their X-axes move in perfect unison:\
  $Z=\Delta X_1+\Delta X_2$

### Movement 2: Pure Roll Forward/Backward (Desk X-Axis)
Imagine rolling the top of the ball away from your wrist (toward 180°E).
- The ball is rotating around an axis that runs from 90°W to 90°E.
- At Sensor 1 (90°W), the surface moves right-to-left, registers movement in its local X-axis $-\Delta X_1$.
- At Sensor 2 (90°E), the surface moves left-to-right, registers movement in its local X-axis $\Delta X_2$.
- The Math: To calculate pure forward/backward desk movement, subtract their local X deltas to combine the motion:\
  $X_{desk}=\Delta X_2-\Delta X_1$

### Movement 3: Pure Tilt Left/Right (Desk Y-Axis)
Imagine rolling the top of the ball to the right (toward 90°E).
- The sensors are 180 degrees apart in longitude.
- The surface moves bottom-up on Sensor 1 (90°W) while top-down on Sensor 2 (90°E). There's no lateral movement since the moving path going through same meridian both sensors are on.
- The Sensor 1 registers movement in its local Y-axis $\Delta Y_1$.
- The Sensor 2 registers movement in its local Y-axis $-\Delta Y_2$.
- The Math: To calculate pure Left/Right desk movement, subtract their local Y deltas to combine the motion:\
  $Y_{desk}=\Delta Y_1-\Delta Y_2$

### Summary
By placing them precisely 180° apart on the same latitude, the ESP32-S2 firmware can compute the 3D navigation using simple addition and subtraction before sending it over USB:
1. Desk Cursor $X = \Delta X_2 - \Delta X_1$
2. Desk Cursor $Y = \Delta Y_1 - \Delta Y_2$
3. CAD Twist $Z = \Delta X_1 + \Delta X_2$

## B. Quantitative Calculation
### Configuration One:
The two sensors are 180° apart, reside on the same latitude circle 45° South.  They are globally orthogonal on two perpendicular global radius line.  Here is the mathematical breakdown of how the trackball's rotation translates into sensor readings.

To analyze the kinematics of a dual-sensor trackball, we model the ball as a perfect sphere of radius $R$ rotating about its center. Optical sensors on a trackball read surface movement ($dx, dy$) along their local tangential axes, which directly correspond to the linear velocity vectors of the sphere's surface at the sensor locations.

#### 1. Geometric Setup & Sensor Positions
Let the trackball center be at the origin $(0,0,0)$ in a standard right-handed 3D Cartesian coordinate system:
- $z$-axis: Points towards the North Pole ($+z$).
- $x$-axis: Points towards the Prime Meridian (0° Longitude, $+x$).
- $y$-axis: Points towards 90° East Longitude ($+y$).

The positions of the two sensors located at Latitude $\phi$ = -45° (45° South) and Longitudes $\theta_1$ = 90° and $\theta_2$ = -90° are given by conversion from spherical to Cartesian coordinates:\
$`\mathbf{P}=\begin{bmatrix}R \cdot cos\phi \cdot cos\theta \\ R \cdot cos\phi \cdot sin\theta \\ R \cdot sin\phi \end{bmatrix}`$

Since cos(-45°) = $\frac{1}{\sqrt{2}}$ and sin(-45°) = $-\frac{1}{\sqrt{2}}$:
- Sensor 1 ($\theta_1$ = 90°):\
$`\mathbf{P}_1 = \begin{bmatrix} 0 \\ \frac{R}{\sqrt{2}} \\ -\frac{R}{\sqrt{2}} \end{bmatrix}`$
- Sensor 2 ($\theta_2$ = -90°):\
$`\mathbf{P}_2 = \begin{bmatrix} 0 \\ -\frac{R}{\sqrt{2}} \\ -\frac{R}{\sqrt{2}} \end{bmatrix}`$

#### 2. Kinematic Relationships
When the ball rotates with an angular velocity vector $\boldsymbol{\omega} = [\omega_x, \omega_y, \omega_z]^T$, the linear velocity $\mathbf{v}$ at any point $\mathbf{P}$ on its surface is determined by the cross product: $\mathbf{v}=\mathbfit{\omega}\times \mathbf{P}$

Expanding this cross product for both sensors yields their absolute velocity vectors:\
$`\mathbf{v}_1=\mathbfit{\omega}\times \mathbf{P}_1=\begin{bmatrix}\omega_x\\ \omega_y\\ \omega_z \end{bmatrix} \times \begin{bmatrix}0\\ \frac{R}{\sqrt{2}}\\ -\frac{R}{\sqrt{2}} \end{bmatrix} =\frac{R}{\sqrt{2}} \begin{bmatrix} -\omega_y-\omega_z\\ \omega_x\\ \omega_x \end{bmatrix}`$

$`\mathbf{v}_2=\mathbfit{\omega}\times \mathbf{P}_2=\begin{bmatrix}\omega_x\\ \omega_y\\ \omega_z \end{bmatrix} \times \begin{bmatrix}0\\ -\frac{R}{\sqrt{2}}\\ -\frac{R}{\sqrt{2}} \end{bmatrix} =\frac{R}{\sqrt{2}} \begin{bmatrix} -\omega_y+\omega_z\\ \omega_x\\ -\omega_x \end{bmatrix}`$

#### 3. Local Sensor Frames & Measurements
An optical sensor cannot measure movement along the normal vector (radial direction into the sphere). It only captures motion along its local tangential surface coordinates ($X_{local}$ aligned along East/West longitude, $Y_{local}$ aligned along North/South latitude).

We define the local unit tangent vectors ($`\hat{\mathbf{u}}_\theta, \hat{\mathbf{u}}_\phi`$) for both positions:\
$`\hat{\mathbf{u}}_\theta=\begin{bmatrix}-sin\theta \\ cos\theta \\ 0 \end{bmatrix},\quad \hat{\mathbf{u}}_\phi=\begin{bmatrix}-sin\phi \cdot cos\theta \\ -sin\phi \cdot sin\theta \\ cos\phi \end{bmatrix}`$

For Sensor 1 ($\theta = 90°, \phi = -45°$):\
$`\mathbf{u}_{\theta 1}=\begin{bmatrix} -1\\ 0\\ 0 \end{bmatrix},\quad \mathbf{u}_{\phi 1}=\begin{bmatrix} 0\\ \frac{1}{\sqrt{2}}\\ \frac{1}{\sqrt{2}} \end{bmatrix}`$

The sensor reads outputs $`S_{1x} = \mathbf{v}_1 \cdot \hat{\mathbf{u}}_{\theta 1}`$ and $`S_{1y} = \mathbf{v}_1 \cdot \hat{\mathbf{u}}_{\phi 1}`$:
- $S_{1x}=\frac{R}{\sqrt{2}}(\omega_y+\omega_z)$
- $S_{1y}=0+\frac{R}{2}\omega_x+\frac{R}{2}\omega_x=R\omega_x$

For Sensor 2 ($\theta = -90°, \phi = -45°$):\
$`\mathbf{u}_{\theta 2}=\begin{bmatrix} 1\\ 0\\ 0 \end{bmatrix},\quad \mathbf{u}_{\phi 2}=\begin{bmatrix} 0\\ -\frac{1}{\sqrt{2}}\\ \frac{1}{\sqrt{2}} \end{bmatrix}`$

The sensor reads outputs $`S_{2x} = \mathbf{v}_2 \cdot \hat{\mathbf{u}}_{\theta 2}`$ and $`S_{2y} = \mathbf{v}_2 \cdot \hat{\mathbf{u}}_{\phi 2}`$:
- $S_{2x}=\frac{R}{\sqrt{2}}(-\omega_y+\omega_z)$
- $S_{2y}=0-\frac{R}{2}\omega_x-\frac{R}{2}\omega_x=-R\omega_x$

The forward kinematic transformation matrix mapping ball velocity $\mathbfit{\omega}$ to sensor readings $\mathbf{S}$ is:
$`\begin{bmatrix}S_{1x}\\ S_{1y}\\ S_{2x}\\ S_{2y}\end{bmatrix}=R \begin{bmatrix}0& \frac{1}{\sqrt{2}}& \frac{1}{\sqrt{2}}\\ 1& 0& 0\\ 0& -\frac{1}{\sqrt{2}}& \frac{1}{\sqrt{2}}\\ -1& 0& 0\end{bmatrix} \begin{bmatrix}\omega_x\\ \omega_y\\ \omega_z\end{bmatrix}`$

#### 5. Solving for Angular Velocities
By taking the sum and difference of the two active horizontal streams, we isolate every axis of rotation:
- Rotation about the X-axis ($\omega_x$): Measured redundantly by both vertical sensor axes with flipped signs.\
$\omega_x=\frac{S_{1y}}{R}=-\frac{S_{2y}}{R}$
- Rotation about the Y-axis ($\omega_y$): Isolated by taking the difference of the horizontal channels.\
$\omega_y=\frac{1}{\sqrt{2}R}(S_{1x}-S_{2x})$
- Rotation about the Z-axis ($\omega_z$): Isolated by taking the sum of the horizontal channels.\
$\omega_z=\frac{1}{\sqrt{2}R}(S_{1x}+S_{2x})$

#### 6. The Optimal Least-Squares Estimation
$`\begin{bmatrix}\omega_x\\ \omega_y\\ \omega_z\end{bmatrix}_{opt}=\frac{1}{R}\begin{bmatrix}\frac{1}{2}(S_{1y}-S_{2y})\\ \frac{1}{\sqrt{2}}(S_{1x}-S_{2x})\\ \frac{1}{\sqrt{2}}(S_{1x}+S_{2x})\end{bmatrix}`$

Why this is mathematically powerful:
- Noise Averaging on X: Instead of relying entirely on one sensor for $\omega_x$, the calculation takes the arithmetic mean of both channels: $\omega_x = \frac{S_{1y} + (-S_{2y})}{2R}$. If one sensor undergoes a micro-slip or hits a dusty patch on the ball, the error is cut in half.

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

---
### Configuration two:
The two sensors are 180° apart, reside on the same latitude circle 60° South. Here is the mathematical breakdown of how the trackball's rotation translates into sensor readings.

#### 1. Geometric Setup & Sensor Coordinates
Let the trackball be a sphere of radius R centered at the origin (0,0,0).
- Latitude $\phi$: -60° (or $\phi$ = -c, where c = $\frac{\pi}{3}$ radians).
- Longitude $\lambda$: Sensor 1 is at $\lambda_1$ = -90° ($-\frac{\pi}{2}$), and Sensor 2 is at $\lambda_2$ = +90° ($+\frac{\pi}{2}$).

Using standard spherical-to-Cartesian coordinates where $+z$ is North, $+x$ is 0° Longitude, $+y$ is 90° Longitude:\
$`\mathbf{P} = \begin{bmatrix} R \cdot \cos\phi \cdot \cos\lambda \\ R \cdot \cos\phi \cdot \sin\lambda \\ R \cdot \sin\phi \end{bmatrix}`$

Substituting $\phi$ = -60° ($\cos$ -60° = 0.5, $\sin$ -60° = $-\frac{\sqrt{3}}{2}$):
- Sensor 1 Position $\mathbf{P}_1$: At\
$`\lambda = -90^\circ \implies \mathbf{P}_1 = \begin{bmatrix} 0 \\ -0.5R \\ -\frac{\sqrt{3}}{2}R \end{bmatrix}`$
- Sensor 2 Position $\mathbf{P}_2$: At\
$`\lambda = +90^\circ \implies \mathbf{P}_2 = \begin{bmatrix} 0 \\ 0.5R \\ -\frac{\sqrt{3}}{2}R \end{bmatrix}`$

#### 2. Local Sensor Coordinate Frames (Surface Kinematics)
An optical sensor tracks local surface displacement $\Delta u, \Delta v$, which corresponds to orthogonal tangent vectors on the sphere's surface.
- Let $\mathbf{u}$ be the local East vector direction of increasing longitude $\lambda$.
- Let $\mathbf{v}$ be the local North vector direction of increasing latitude $\phi$.
The unit tangent vectors at any point are derived by differentiating $\mathbf{P}$:\
$`\mathbf{u}=\begin{bmatrix}-\sin \lambda \\ \cos \lambda \\ 0\end{bmatrix},\quad \mathbf{v}=\begin{bmatrix} -\sin\phi \cdot \cos\lambda \\ -\sin\phi \cdot \sin\lambda \\ \cos\phi \end{bmatrix}`$

Evaluating these at each sensor location yields their local tracking axes:
| Axis | Sensor 1 $\lambda$ = -90°, $\phi$ = -60°	| Sensor 2 $\lambda$ = +90°, $\phi$ = -60° |
|--|--|--|
| $\mathbf{u}$ Horizontal/East | $`\begin{bmatrix}1\\ 0\\ 0\end{bmatrix}`$ | $`\begin{bmatrix}-1\\ 0\\ 0\end{bmatrix}`$ |
| $\mathbf{v}$ Vertical/North | $`\begin{bmatrix}0\\ -\frac{\sqrt{3}}{2}\\ 0.5\end{bmatrix}`$ | $`\begin{bmatrix}0\\ \frac{\sqrt{3}}{2}\\ 0.5\end{bmatrix}`$ |

#### 3. Kinematic Equations (Velocity Mapping)
When the user rolls the ball, it rotates with an angular velocity vector $\boldsymbol{\omega} = [\omega_x, \omega_y, \omega_z]^T$.

The linear velocity $\mathbf{V}$ of the sphere's surface underneath a fixed sensor at position $\mathbf{P}$ is given by:\
$\mathbf{V}=\mathbfit{\omega} \times \mathbf{P}$

The 2D output of each sensor $U, V$ matches the projection of this linear surface velocity onto the sensor's local $\mathbf{u}$ and $\mathbf{v}$ axes:\
$U=\mathbf{V}\cdot \mathbf{u},\quad V=\mathbf{V}\cdot \mathbf{v}$

By calculating the cross products $\boldsymbol{\omega} \times \mathbf{P}_i$ and projecting them onto each sensor's coordinate system, we establish the forward kinematics:

Sensor 1 Outputs:
- $U_1 = -0.5 R \cdot \omega_z + \frac{\sqrt{3}}{2} R \cdot \omega_y$
- $V_1 = 0.5 R \cdot \omega_x$

Sensor 2 Outputs:
- $U_2 = -0.5 R \cdot \omega_z - \frac{\sqrt{3}}{2} R \cdot \omega_y$
- $V_2 = -0.5 R \cdot \omega_x$

#### 4. Key Kinematic Insights & Trade-offs
- The Redundancy/Symmetry Constraint: Note that $V_1 = -V_2$. Because both sensors sit on the exact same latitude line symmetrically opposite each other, both vertical sensor tracks measure the exact same physical rotation channel $\omega_x$.
- The "Blind" Rotation Component: Notice that neither $V_1$ nor $V_2$ contains terms for $\omega_y$ or $\omega_z$. If a user tries to isolate $\omega_y$ or $\omega_z$, it can only be extracted by mathematically decoupling the $U_1$ and $U_2$ signals.
- Solving for 3D Angular Velocity: If you are trying to reconstruct full 3D rotation from these 4 output channels, you can invert the system using the $U$ channels and an average of the $V$ channels:\
$\omega_x=\frac{V_1-V_2}{2R \cdot 0.5}=\frac{V_1-V_2}{R}$\
$\omega_y=\frac{U_1-U_2}{\sqrt{3}R}$\
$\omega_z=-\frac{U_1+U_2}{R}$

#### 5. Singularity Warning
Because $V_1$ and $V_2$ are perfectly linearly dependent $V_1 + V_2 = 0$, your actual matrix rank for tracking is 3, not 4. While this allows you to solve for full 3D rotation $\omega_x, \omega_y, \omega_z$, you have zero tracking redundancy for $\omega_y$ and $\omega_z$. If Sensor 1's horizontal tracker slips, $\omega_y$ and $\omega_z$ calculations will fail completely.

---
### Configuration Three: 
The two sensors are orthogonal in terms of longitude (90° apart), reside on the same latitude circle 60° South. Here is the mathematical breakdown of how the trackball's rotation translates into sensor readings.

To make the trackball accurate in a quantitative sense, it needs to map the raw sensor readings to the true 3D angular velocity vector of the ball $\vec{\omega} = [\omega_x, \omega_y, \omega_z]^T$ using a precise kinematic transformation matrix.
Let's break down the exact mathematics for your specific layout.
#### 1. Establishing the Coordinate Systems
1. Global Deck Coordinates (Cartesian coordinate, right handed):
	- X axis: pointing and increasing out towards viewer on horizontal plane.
	- Y axis: pointing and increasing to right on horizontal plane.
	- Z axis: pointing and increasing up. 
	- $+\omega_x$ = Rolling the ball to the left around X-axis (towards 90°W).
	- $+\omega_y$ = Rolling the ball backward around Y-axis (towards 0°E).
	- $+\omega_z$ = Twisting the ball counter-clockwise around Z-axis (looking from above).
	- Equator is latitude 0°, increasing upward, North pole is 90°, South pole is -90°.
	- X axis direction is longitude 0° (Prime Meridian), towards wrist.
3. Sensor Positions on a Unit Sphere $(R=1)$:
	- Latitude $\theta$ = -60° (or 60°S).
	- Sensor 1 Longitude $\phi_1$ = -90°.
	- Sensor 2 Longitude $\phi_2$ = 90°.
4. Sensor Local Axes:
	- Assume the sensor is aligned flat against the ball's surface.
	- Local $\Delta X$ points along the line of latitude, increasing right-to-left (i.e., $+\omega_z$ direction).
	- Local $\Delta Y$ points along the line of longitude, increasing bottom-up.

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
$\omega_x=-\frac{\sqrt{3}}{2} \Delta Y_1 - \frac{1}{2} \Delta Y_2$\
$\omega_y=\frac{1}{2} \Delta Y_1 - \frac{\sqrt{3}}{2} \Delta Y_2$\
$\omega_z=-\Delta X_1 - \Delta X_2 - \frac{\sqrt{3}}{2} \Delta Y_1 + \frac{\sqrt{3}}{2} \Delta Y_2$

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
