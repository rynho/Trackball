# Geometry Kinematics
## A. Qualitative Analysis

### Global Deck Coordinates (Cartesian coordinate, right handed):
Let the Trackball center be at the origin $(0,0,0)$ in a standard right-handed 3D Cartesian coordinate system 
- X axis: pointing and increasing out towards viewer (the Prime Meridian, 0° Longitude, $+x$) on horizontal plane.
- Y axis: pointing and increasing to right (90° East Longitude, $+y$) on horizontal plane.
- Z axis: pointing and increasing up towards the North Pole ($+z$). 
- $+\omega_x$ = Rolling the ball to the left around X-axis (towards 90°W).
- $+\omega_y$ = Rolling the ball backward around Y-axis (towards 0°E).
- $+\omega_z$ = Twisting the ball counter-clockwise around Z-axis (looking from above).
- Equator is latitude 0°, increasing upward, North pole is 90°, South pole is -90°.
- X axis direction is longitude 0° (Prime Meridian), towards wrist.

### Movement 1: Pure Tilt Left/Right (Desk Y-Axis, $\omega_x$+)
Imagine rolling the top of the ball to the left (toward 90°W).
- The sensors are 180 degrees apart in longitude.
- The surface moves bottom-up on Sensor 1 (90°E) while top-down on Sensor 2 (90°W). There's no lateral movement since the moving path going through same meridian both sensors are on.
- The Sensor 1 registers movement in its local Y-axis $\Delta Y_1$.
- The Sensor 2 registers movement in its local Y-axis $-\Delta Y_2$.
- The Math: To calculate pure Left/Right desk movement, subtract their local Y deltas to combine the motion:\
  $\omega_x=\Delta Y_1-\Delta Y_2$
  
### Movement 2: Pure Roll Forward/Backward (Desk X-Axis, $\omega_y$+)
Imagine rolling the top of the ball away from your wrist (toward 180°E).
- The ball is rotating around an axis that runs from 90°W to 90°E.
- At Sensor 1 (90°E), the surface moves left-to-right, registers movement in its local X-axis $\Delta X_1$.
- At Sensor 2 (90°W), the surface moves right-to-left, registers movement in its local X-axis $-\Delta X_2$.
- The Math: To calculate pure forward/backward desk movement, subtract their local X deltas to combine the motion:\
  $\omega_y=\Delta X_1-\Delta X_2$

### Movement 3: Pure Twist (Z-Axis Rotation, $\omega_z$+)
Imagine twisting the ball counter-clockwise (spinning it like a top but reversely).
- Because both sensors are sitting at the exact same distance from the equator (60°S), the ball's surface slides sideways over both windows at the exact same speed.
- The surface moves right-to-left past Sensor 1 (90°E) and Sensor 2 (90°W).
- The Math: Both sensors register a pure horizontal shift in their local X-axes:\
  $\Delta X_1=\Delta X_2$

  Therefore, to isolate Twist (Z), you look for when their X-axes move in perfect unison:\
  $\omega_z=\Delta X_1+\Delta X_2$

### Summary
By placing them precisely 180° apart on the same latitude, the ESP32-S2 firmware can compute the 3D navigation using simple addition and subtraction before sending it over USB:
1. Desk Cursor Y $\implies \omega_x = \Delta Y_1 - \Delta Y_2$
2. Desk Cursor X $\implies \omega_y = \Delta X_1 - \Delta X_2$
3. CAD Twist Z $\implies \omega_z = \Delta X_1 + \Delta X_2$

## B. Quantitative Calculation

### Configuration A:
The two sensors are 180° apart, reside on the same latitude circle 45° South.  They are globally orthogonal on two perpendicular global radius line.  Here is the mathematical breakdown of how the trackball's rotation translates into sensor readings.

To analyze the kinematics of a dual-sensor trackball, we model the ball as a perfect sphere of radius $R$ rotating about its center. Optical sensors on a trackball read surface movement ($dx, dy$) along their local tangential axes, which directly correspond to the linear velocity vectors of the sphere's surface at the sensor locations.

The positions of the two sensors located at Latitude $\phi$ = -45° (45° South) and Longitudes $\theta_1$ = 90° and $\theta_2$ = -90° are given by conversion from spherical to Cartesian coordinates:\
$`P=\begin{bmatrix}R \cdot cos\phi \cdot cos\theta \\ R \cdot cos\phi \cdot sin\theta \\ R \cdot sin\phi \end{bmatrix}`$

Since cos(-45°) = $\frac{\sqrt{2}}{2}$ and sin(-45°) = $-\frac{\sqrt{2}}{2}$:
- Sensor 1 ($\theta_1$ = 90°):\
$`P_1 = \begin{bmatrix} 0 \\ \frac{\sqrt{2}R}{2} \\ -\frac{\sqrt{2}R}{2} \end{bmatrix}`$
- Sensor 2 ($\theta_2$ = -90°):\
$`P_2 = \begin{bmatrix} 0 \\ -\frac{\sqrt{2}R}{2} \\ -\frac{\sqrt{2}R}{2} \end{bmatrix}`$

#### 2. Kinematic Relationships
When the ball rotates with an angular velocity vector $\omega = [\omega_x, \omega_y, \omega_z]^T$, the linear velocity $\hat{V}$ at any point $P$ on its surface is determined by the cross product: $\hat{V}=\omega\times P$

Expanding this cross product for both sensors yields their absolute velocity vectors:\
$`\hat{V}_1=\omega\times P_1=\begin{bmatrix}\omega_x\\ \omega_y\\ \omega_z \end{bmatrix} \times \begin{bmatrix}0\\ \frac{\sqrt{2}R}{2}\\ -\frac{\sqrt{2}R}{2} \end{bmatrix} =\frac{\sqrt{2}R}{2} \begin{bmatrix} -\omega_y-\omega_z\\ \omega_x\\ \omega_x \end{bmatrix}`$

$`\hat{V}_2=\omega\times P_2=\begin{bmatrix}\omega_x\\ \omega_y\\ \omega_z \end{bmatrix} \times \begin{bmatrix}0\\ -\frac{\sqrt{2}R}{2}\\ -\frac{\sqrt{2}R}{2} \end{bmatrix} =\frac{\sqrt{2}R}{2} \begin{bmatrix} -\omega_y+\omega_z\\ \omega_x\\ -\omega_x \end{bmatrix}`$

#### 3. Local Sensor Frames & Measurements
An optical sensor cannot measure movement along the normal vector (radial direction into the sphere). It only captures motion along its local tangential surface coordinates ($X_{local}$ aligned along East/West longitude, $Y_{local}$ aligned along North/South latitude).

We define the local unit tangent vectors (u, v) for both positions:\
$`u=\begin{bmatrix}-sin\theta \\ cos\theta \\ 0 \end{bmatrix},\quad v=\begin{bmatrix}-sin\phi \cdot cos\theta \\ -sin\phi \cdot sin\theta \\ cos\phi \end{bmatrix}`$

For Sensor 1 ($\theta_1 = 90°, \phi = -45°$):\
$`u_1=\begin{bmatrix} -1\\ 0\\ 0 \end{bmatrix},\quad v_1=\begin{bmatrix} 0\\ \frac{\sqrt{2}}{2}\\ \frac{\sqrt{2}}{2} \end{bmatrix}`$

The sensor reads outputs $`U_1 = \hat{V}_1 \cdot u_1`$ and $`V_1 = \hat{V}_1 \cdot v_1`$:
- $U_1=\frac{\sqrt{2}R}{2}(\omega_y+\omega_z)$
- $V_1=0+\frac{R}{2}\omega_x+\frac{R}{2}\omega_x=R\omega_x$

For Sensor 2 ($\theta_2 = -90°, \phi = -45°$):\
$`u_2=\begin{bmatrix} 1\\ 0\\ 0 \end{bmatrix},\quad v_2=\begin{bmatrix} 0\\ -\frac{\sqrt{2}}{2}\\ \frac{\sqrt{2}}{2} \end{bmatrix}`$

The sensor reads outputs $`U_2 = \hat{V}_2 \cdot u_2`$ and $`V_2 = \hat{V}_2 \cdot v_2`$:
- $U_2=\frac{\sqrt{2}R}{2}(-\omega_y+\omega_z)$
- $V_2=0-\frac{R}{2}\omega_x-\frac{R}{2}\omega_x=-R\omega_x$

The forward kinematic transformation matrix mapping ball velocity $\omega$ to sensor readings $S$ is:
$`\begin{bmatrix}U_1\\ V_1\\ U_2\\ V_2\end{bmatrix}=R \begin{bmatrix}0& \frac{\sqrt{2}}{2}& \frac{\sqrt{2}}{2}\\ 1& 0& 0\\ 0& -\frac{\sqrt{2}}{2}& \frac{\sqrt{2}}{2}\\ -1& 0& 0\end{bmatrix} \begin{bmatrix}\omega_x\\ \omega_y\\ \omega_z\end{bmatrix}`$

#### 5. Solving for Angular Velocities
By taking the sum and difference of the two active horizontal streams, we isolate every axis of rotation:
- Rotation about the X-axis ($\omega_x$): Measured redundantly by both vertical sensor axes with flipped signs.\
$\omega_x=\frac{V_1}{R}=-\frac{V_2}{R}$
- Rotation about the Y-axis ($\omega_y$): Isolated by taking the difference of the horizontal channels.\
$\omega_y=\frac{\sqrt{2}}{2R}(U_1-U_2)$
- Rotation about the Z-axis ($\omega_z$): Isolated by taking the sum of the horizontal channels.\
$\omega_z=\frac{\sqrt{2}}{2R}(U_1+U_2)$

#### 6. The Optimal Least-Squares Estimation
$`\begin{bmatrix}\omega_x\\ \omega_y\\ \omega_z\end{bmatrix}_{opt}=\frac{1}{2R}\begin{bmatrix}V_1-V_2\\ \sqrt{2}(U_1-U_2)\\ \sqrt{2}(U_1+U_2)\end{bmatrix}`$

Why this is mathematically powerful:
- Noise Averaging on X: Instead of relying entirely on one sensor for $\omega_x$, the calculation takes the arithmetic mean of both channels: $\omega_x = \frac{1}{2}[\frac{V_1}{R} + (-\frac{V_2}{R})]$. If one sensor undergoes a micro-slip or hits a dusty patch on the ball, the error is cut in half.

```
// Define the geometric transformation constant
const float SQRT2 = 1.4142136f; 
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
### Configuration B:
The two sensors are 180° apart, reside on the same latitude circle 60° South. Here is the mathematical breakdown of how the trackball's rotation translates into sensor readings.

#### 1. Geometric Setup & Sensor Coordinates
Let the trackball be a sphere of radius R centered at the origin (0,0,0).
- Latitude $\phi$: -60° (South).
- Longitude $\theta$: Sensor 1 is at $\theta_1$ = 90° (East), and Sensor 2 is at $\theta_2$ = -90° (West).

Using standard spherical-to-Cartesian coordinates where $+z$ is North, $+x$ is 0° Longitude, $+y$ is 90° Longitude:\
$`\mathbf{P} = \begin{bmatrix} R \cdot cos\phi \cdot cos\theta \\ R \cdot cos\phi \cdot sin\theta \\ R \cdot sin\phi \end{bmatrix}`$

Substituting $\phi$ = -60° (cos-60° = $\frac{1}{2}$, sin-60° = $-\frac{\sqrt{3}}{2}$):
- Sensor 1 Position $\mathbf{P}_1$: At $\theta_1$ = +90° (cos90°=0, sin90°=1)\
$`\mathbf{P}_1 = \begin{bmatrix} R\cdot \frac{1}{2}\cdot0 \\R\cdot \frac{1}{2}\cdot1 \\ R\cdot -\frac{\sqrt{3}}{2} \end{bmatrix} \implies \mathbf{P}_1 = \begin{bmatrix} 0 \\ \frac{1}{2}R \\ -\frac{\sqrt{3}}{2}R \end{bmatrix}`$
- Sensor 2 Position $\mathbf{P}_2$: At $\theta_2$ = -90° (cos-90°=0, sin-90°=-1)\
$`\mathbf{P}_2 = \begin{bmatrix} R\cdot \frac{1}{2}\cdot0\\ R\cdot \frac{1}{2}\cdot-1\\ R\cdot -\frac{\sqrt{3}}{2} \end{bmatrix} \implies \mathbf{P}_2 = \begin{bmatrix} 0 \\ -\frac{1}{2}R \\ -\frac{\sqrt{3}}{2}R \end{bmatrix}`$

#### 2. Local Sensor Coordinate Frames (Surface Kinematics)
An optical sensor tracks local surface displacement $\Delta u, \Delta v$, which corresponds to orthogonal tangent vectors on the sphere's surface.
- Let $u$ be the local East vector direction of increasing longitude $\theta$.
- Let $v$ be the local North vector direction of increasing latitude $\phi$.
The unit tangent vectors at any point are derived by differentiating $\mathbf{P}$:\
$`u=\begin{bmatrix}-sin\theta \\ cos\theta \\ 0\end{bmatrix},\quad v=\begin{bmatrix} -sin\phi \cdot cos\theta \\ -sin\phi \cdot sin\theta \\ cos\phi \end{bmatrix}`$

Evaluating these at each sensor location yields their local tracking axes:
| Axis | Sensor 1 $\theta_1$ = +90°, $\phi$ = -60°	| Sensor 2 $\theta_2$ = -90°, $\phi$ = -60° |
|--|--|--|
| Horizontal/East | $`u_1=\begin{bmatrix} -1\\ 0\\ 0\end{bmatrix}`$ | $`u_2=\begin{bmatrix} 1\\ 0\\ 0\end{bmatrix}`$ |
| Vertical/North | $`v_1=\begin{bmatrix} 0\\ \frac{\sqrt{3}}{2}\\ \frac{1}{2} \end{bmatrix}`$ | $`v_2=\begin{bmatrix} 0\\ -\frac{\sqrt{3}}{2}\\ \frac{1}{2} \end{bmatrix}`$ |

#### 3. Kinematic Equations (Velocity Mapping)
When the user rolls the ball, it rotates with an angular velocity vector $\boldsymbol{\omega} = [\omega_x, \omega_y, \omega_z]^T$.

The linear velocity $\hat{V}$ of the sphere's surface underneath a fixed sensor at position $P$ is given by:\
$\hat{V}=\omega \times P$\
$`\hat{V}_1 = \omega \times P_1 = \begin{bmatrix}\omega_x\\ \omega_y\\ \omega_z\end{bmatrix} \times \begin{bmatrix} 0 \\ \frac{1}{2}R \\ -\frac{\sqrt{3}}{2}R \end{bmatrix} = R\begin{bmatrix} -\frac{\sqrt{3}}{2}\omega_y -\frac{1}{2}\omega_z\\ \frac{\sqrt{3}}{2}\omega_x\\ \frac{1}{2}\omega_x\end{bmatrix}`$\
$`\hat{V}_2 = \omega \times P_2 = \begin{bmatrix}\omega_x\\ \omega_y\\ \omega_z\end{bmatrix} \times \begin{bmatrix} 0 \\ -\frac{1}{2}R \\ -\frac{\sqrt{3}}{2}R \end{bmatrix} = R\begin{bmatrix}-\frac{\sqrt{3}}{2}\omega_y+\frac{1}{2}\omega_z\\ \frac{\sqrt{3}}{2}\omega_x\\ -\frac{1}{2}\omega_x\end{bmatrix} `$

The 2D output of each sensor $U, V$ matches the projection of this linear surface velocity onto the sensor's local $u$ and $v$ axes:\
$U=\hat{V}\cdot u,\quad V=\hat{V}\cdot v$

By calculating the cross products $\omega \times P_i$ and projecting them onto each sensor's coordinate system, we establish the forward kinematics:

Sensor 1 Outputs:\
$`U_1 = \hat{V}_1 \cdot u_1 = R\begin{bmatrix} -\frac{\sqrt{3}}{2}\omega_y -\frac{1}{2}\omega_z\\ \frac{\sqrt{3}}{2}\omega_x\\ \frac{1}{2}\omega_x\end{bmatrix} \cdot \begin{bmatrix} -1\\ 0\\ 0\end{bmatrix} = \frac{\sqrt{3}R}{2}\omega_y +\frac{R}{2}\omega_z`$\
$`V_1 = \hat{V}_1 \cdot v_1 = R\begin{bmatrix} -\frac{\sqrt{3}}{2}\omega_y -\frac{1}{2}\omega_z\\ \frac{\sqrt{3}}{2}\omega_x\\ \frac{1}{2}\omega_x\end{bmatrix} \cdot \begin{bmatrix} 0\\ \frac{\sqrt{3}}{2}\\ \frac{1}{2} \end{bmatrix} = R\omega_x`$

Sensor 2 Outputs:\
$`U_2 = \hat{V}_2 \cdot u_2 = R\begin{bmatrix}-\frac{\sqrt{3}}{2}\omega_y+\frac{1}{2}\omega_z\\ \frac{\sqrt{3}}{2}\omega_x\\ -\frac{1}{2}\omega_x\end{bmatrix} \cdot \begin{bmatrix} 1\\ 0\\ 0\end{bmatrix} = -\frac{\sqrt{3}R}{2}\omega_y +\frac{R}{2}\omega_z`$\
$`V_2 = \hat{V}_2 \cdot v_2 = R\begin{bmatrix}-\frac{\sqrt{3}}{2}\omega_y+\frac{1}{2}\omega_z\\ \frac{\sqrt{3}}{2}\omega_x\\ -\frac{1}{2}\omega_x\end{bmatrix} \cdot \begin{bmatrix} 0\\ -\frac{\sqrt{3}}{2}\\ \frac{1}{2} \end{bmatrix} = -R\omega_x`$

#### 4. Key Kinematic Insights & Trade-offs
- The Redundancy/Symmetry Constraint: Note that $V_1 = -V_2$. Because both sensors sit on the exact same latitude line symmetrically opposite each other, both vertical sensor tracks measure the exact same physical rotation channel $\omega_x$.
- The "Blind" Rotation Component: Notice that neither $V_1$ nor $V_2$ contains terms for $\omega_y$ or $\omega_z$. If a user tries to isolate $\omega_y$ or $\omega_z$, it can only be extracted by mathematically decoupling the $U_1$ and $U_2$ signals.
- Solving for 3D Angular Velocity: If you are trying to reconstruct full 3D rotation from these 4 output channels, you can invert the system using the $U$ channels and an average of the $V$ channels:\
$\omega_x=\frac{1}{2}(\frac{V_1}{R}-\frac{V_2}{R})=\frac{V_1-V_2}{2R}$\
$\omega_y=\frac{U_1-U_2}{\sqrt{3}R}$\
$\omega_z=\frac{U_1+U_2}{R}$

#### 5. Singularity Warning
Because $V_1$ and $V_2$ are perfectly linearly dependent $V_1 + V_2 = 0$, your actual matrix rank for tracking is 3, not 4. While this allows you to solve for full 3D rotation $\omega_x, \omega_y, \omega_z$, you have zero tracking redundancy for $\omega_y$ and $\omega_z$. If Sensor 1's horizontal tracker slips, $\omega_y$ and $\omega_z$ calculations will fail completely.

---
### Configuration C: 
The two sensors are orthogonal in terms of longitude (90° apart), reside on the same latitude circle 60° South. Here is the mathematical breakdown of how the trackball's rotation translates into sensor readings.

To make the trackball accurate in a quantitative sense, it needs to map the raw sensor readings to the true 3D angular velocity vector of the ball $\vec{\omega} = [\omega_x, \omega_y, \omega_z]^T$ using a precise kinematic transformation matrix.
Let's break down the exact mathematics for your specific layout.
#### 1. Establishing the Coordinate Systems
1. Sensor Positions on a Unit Sphere $(R=1)$:
	- Latitude $\phi$ = -60° (or 60°S).
	- Sensor 1 Longitude $\theta_1$ = 60°E.
	- Sensor 2 Longitude $\theta_2$ = 150°E.
2. Sensor Local Axes:
	- Assume the sensor is aligned flat against the ball's surface.
	- Local $\Delta X$ points along the line of latitude, increasing longitude $\theta$, i.e., $+\omega_z$.
	- Local $\Delta Y$ points along the line of longitude, increasing latitude $\phi$, bottom-up.

#### 2. The Exact Kinematic Equations
When the ball rotates with an angular velocity $[\omega_x$, $\omega_y$, $\omega_z]$, the surface velocity seen by a sensor at latitude $\phi$ and longitude $\theta$ is derived from the cross product of the rotation vector and the sensor's position vector.
For any sensor placed at ($\phi, \theta$), the local raw counts map precisely to the 3D rotation via these two equations:\
$\Delta X = R [\omega_x(-sin\theta) + \omega_y(cos\theta)]$\
$\Delta Y = R [\omega_x(-sin\phi \cdot cos\theta) + \omega_y(-sin\phi \cdot sin\theta) + \omega_z(cos\phi)]$

If we plug exact angles into these equations ($\phi$=-60°, $\theta_1$=60°, $\theta_2$=150°), we can evaluate the sines and cosines.

For Sensor 1 (-60°S, 60°E):
- $\Delta X_1 = -\frac{\sqrt{3}}{2}\omega_x + \frac{1}{2}\omega_y$
- $\Delta Y_1 = -(-\frac{\sqrt{3}}{2})\frac{1}{2}\omega_x - (-\frac{\sqrt{3}}{2})\frac{\sqrt{3}}{2}\omega_y + \frac{1}{2}\omega_z = \frac{\sqrt{3}}{4}\omega_x + \frac{3}{4}\omega_y + \frac{1}{2}\omega_z$


For Sensor 2 (-60°S, 150°E):
- $\Delta X_2 = -\frac{1}{2}\omega_x - \frac{\sqrt{3}}{2}\omega_y$
- $\Delta Y_2 = -(-\frac{\sqrt{3}}{2})(-\frac{\sqrt{3}}{2})\omega_x - (-\frac{\sqrt{3}}{2})\frac{1}{2}\omega_y + \frac{1}{2}\omega_z = -\frac{3}{4}\omega_x + \frac{\sqrt{3}}{4}\omega_y + \frac{1}{2}\omega_z$

#### 3. The Inverted Matrix (Firmware Code)
To run this on the ESP32-S2, we must invert the system of equations. Since we have 4 inputs ($\Delta X_1$, $\Delta Y_1$, $\Delta X_2$, $\Delta Y_2$) and only 3 unknown outputs ($\omega_x, \omega_y, \omega_z$), the system is overdetermined. We use a least-squares matrix inversion to get the most accurate, mathematically balanced translation.

When solved, the exact quantitative formulas for the firmware are:\
$\omega_x=-\frac{\sqrt{3}}{2} \Delta X_1 - \frac{1}{2} \Delta X_2$\
$\omega_y=\frac{1}{2} \Delta X_1 - \frac{\sqrt{3}}{2} \Delta X_2$\
$\omega_z=-\frac{\sqrt{3}}{2}\Delta X_1 +\frac{\sqrt{3}}{2}\Delta X_2 +\Delta Y_1 +\Delta Y_2$

There's also a set of full system least-square resolution:\
$\omega_x=\frac{3-11\sqrt{3}}{28}X_1+\frac{3\sqrt{3}-11}{28}X_2+\frac{3+\sqrt{3}}{14}Y_1-\frac{3+\sqrt{3}}{14}Y_2$\
$\omega_y=\frac{11+3\sqrt{3}}{28}X_1-\frac{11\sqrt{3}+3}{28}X_2+\frac{3-\sqrt{3}}{14}Y_1-\frac{3-\sqrt{3}}{14}Y_2$\
$\omega_z=-\frac{\sqrt{3}}{2}X_1+\frac{\sqrt{3}}{2}X_2+Y_1+Y_2$

```
// Define the geometric transformation constant
const float SQRT3_DIV2 = 0.8660254f; 

void calculate_global_movement(float dx1, float dy1, float dx2, float dy2, 
                                float *out_wx, float *out_wy, float *out_wz) {
    // 1. Pre-calculate common scaled variables to save clock cycles
    float scale_dx1 = SQRT3_DIV2 * dx1;
    float scale_dx2 = SQRT3_DIV2 * dx2;

    // 2. Solve for X and Y angular velocities (decoupled dy method)
    *out_wx = - scale_dx1 - (0.5f * dx2);
    *out_wy = (0.5f * dx1) - scale_dx2;

    // 3. Solve for Z twist using a balanced average from both sensors
    *out_wz = - scale_dx1 + scale_dx2 + dy1 + dy2;
}
```
