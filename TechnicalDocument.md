# Technical Documentation

# Lighting

> Entry point: `void TrainView::setLighting()`

The lighting system is based on the **Phong Reflection Model**:

\[
I = I_{Ambient} + I_{Diffuse} + I_{Specular}
\]

The implementation uses OpenGL fixed-function lighting (`GL_LIGHTING`) with three light sources: a directional, a point, and a spot light.

## Components

### Ambient

Omnidirectional ambient light that illuminates all surfaces uniformly.

- Simulates indirect lighting and prevents completely dark surfaces.
- Implemented as **global** ambient light via `glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ...)` rather than through per-light ambient terms (except point light for local warmth).

### Diffuse

Light scattered equally in all directions upon hitting a surface.

- Provides the primary shading based on surface orientation relative to the light direction.
- Depends on the Lambertian term `max(dot(N, L), 0)` where `N` is the surface normal and `L` is the light direction.

### Specular

Directional reflection creating glossy highlights.

- Simulates shiny surface properties.
- Controlled via material specular color and `GL_SHININESS`.

## Directional Light (`GL_LIGHT0`)

> Helper: `setDirectionalLight()` inside `TrainView::setLighting()`

Simulates parallel rays from an infinitely distant source (e.g., sunlight).

### Configuration

```cpp
float directionalAmbient[]  = { 0.0f, 0.0f, 0.0f, 1.0 };
float directionalDiffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0 };
float directionalLightPosition[] = { 0.0f, 1.0f, 1.0f, 0.0f };

glLightfv(GL_LIGHT0, GL_AMBIENT,  directionalAmbient);
glLightfv(GL_LIGHT0, GL_DIFFUSE,  directionalDiffuse);
glLightfv(GL_LIGHT0, GL_POSITION, directionalLightPosition);
```

### Properties

| **Property**  | **Value**                     | **Description**                                      |
| ------------- | ----------------------------- | ---------------------------------------------------- |
| Ambient       | `(0, 0, 0)`                   | No per-light ambient; ambient handled globally       |
| Diffuse       | `(1, 1, 1)`                   | Pure white directional light                         |
| Position      | `(0, 1, 1, 0)`                | Direction vector; w=0 indicates directional          |
| Direction     | From origin towards (0, 1, 1) | 45° above horizontal plane                           |

### Activation Logic

- **Top View**: Always enabled (for consistent edit lighting).
- **Other Views**: Enabled/disabled via `tw->directionalLightButton->value()`.

```cpp
if (tw->topCam->value()) {
    setDirectionalLight();
    glEnable(GL_LIGHT0);
    glDisable(GL_LIGHT1);
    glDisable(GL_LIGHT2);
} else {
    if (tw->directionalLightButton->value()) {
        setDirectionalLight();
        glEnable(GL_LIGHT0);
    } else {
        glDisable(GL_LIGHT0);
    }
    // Point and spot handled separately...
}
```

## Point Light (`GL_LIGHT1`)

> Helper: `setPointLight()` inside `TrainView::setLighting()`

Emits light radially from a single point at the **selected track control point**, with intensity attenuating by distance.

### Configuration

```cpp
float pointAmbient[] = { 0.05f, 0.05f, 0.02f, 1.0f };
float pointDiffuse[] = { 1.0f, 1.0f, 0.2f, 1.0f };

Pnt3f pointPosition(10.0f, 10.0f, 10.0f);
// (optionally) oriented along pointOrientation

float pointLightPosition[] = {
    pointPosition.x, pointPosition.y, pointPosition.z, 1.0f
};

glLightfv(GL_LIGHT1, GL_AMBIENT,  pointAmbient);
glLightfv(GL_LIGHT1, GL_DIFFUSE,  pointDiffuse);
glLightfv(GL_LIGHT1, GL_POSITION, pointLightPosition);
```

### Properties

| **Property**    | **Value**            | **Description**                                           |
| --------------- | -------------------- | --------------------------------------------------------- |
| Ambient         | `(0.05, 0.05, 0.02)` | Warm, subtle ambient glow                                 |
| Diffuse         | `(1.0, 1.0, 0.2)`    | Yellowish-white light                                     |
| Position        | Dynamic              | Tracks selected control point or defaults to (10, 10, 10) |
| w-component     | `1.0`                | Indicates positional (point) light source                 |

### Dynamic Positioning

```cpp
Pnt3f pointPosition(10.0f, 10.0f, 10.0f);
Pnt3f pointOrientation(0.0f, -1.0f, 0.0f);

if (!m_pTrack->points.empty()) {
    int selectIndex =
        (selectedCube >= 0 && selectedCube < (int)m_pTrack->points.size())
            ? selectedCube
            : 0;

    pointPosition    = m_pTrack->points[selectIndex].pos;
    pointOrientation = m_pTrack->points[selectIndex].orient;
    pointOrientation.normalize();
}
```

### Activation Logic

- Enabled/disabled via `tw->pointLightButton->value()`.
- Automatically disabled in top view:

```cpp
if (!tw->topCam->value() && tw->pointLightButton->value()) {
    setPointLight();
    glEnable(GL_LIGHT1);
} else {
    glDisable(GL_LIGHT1);
}
```

## Spot Light (`GL_LIGHT2`)

> Helper: `setSpotLight()` inside `TrainView::setLighting()`

Emits a cone of light from the **train’s head**, creating a focused headlight effect.

### Configuration

```cpp
float spotAmbient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
float spotDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };

float spotLightPosition[] = {
    trainPosition.x, trainPosition.y, trainPosition.z, 1.0f
};
float spotLightDirection[] = {
    trainForward.x, trainForward.y, trainForward.z
};

glLightfv(GL_LIGHT2, GL_AMBIENT,        spotAmbient);
glLightfv(GL_LIGHT2, GL_DIFFUSE,        spotDiffuse);
glLightfv(GL_LIGHT2, GL_POSITION,       spotLightPosition);
glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, spotLightDirection);

glLightf(GL_LIGHT2, GL_SPOT_CUTOFF,   30.0f);
glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 15.0f);
```

### Properties

| **Property**     | **Value**       | **Description**                                      |
| ---------------- | --------------- | ---------------------------------------------------- |
| Ambient          | `(0, 0, 0)`     | No ambient; focused lighting only                    |
| Diffuse          | `(1, 1, 1)`     | Pure white beam                                      |
| Position         | `trainPosition` | Attached to train’s current location                 |
| Direction        | `trainForward`  | Aligned with train’s forward vector                  |
| Cutoff Angle     | `30°`           | Half-angle of the light cone                         |
| Exponent         | `15`            | Controls light concentration (higher = tighter beam) |

### Dynamic Behavior

- **Position** and **Direction** updated every frame from `trainPosition` and `trainForward`.
- Enabled/disabled via `tw->spotLightButton->value()`.
- Disabled in top view:

```cpp
if (!tw->topCam->value() && tw->spotLightButton->value()) {
    setSpotLight();
    glEnable(GL_LIGHT2);
} else {
    glDisable(GL_LIGHT2);
}
```

# Different Views

> Entry point: `void TrainView::setProjection()`

Three camera modes are available and toggled via the UI:

- **World View** (Arcball)
- **Top View** (Orthographic)
- **Train View** (Follow / chase camera)

The view selection affects projection, modelview setup, and some lighting decisions.

## World View (Arcball)

Perspective camera controlled by an ArcBall controller.

### Setup

Called in `resetArcball()`:

```cpp
arcball.setup(this,
              40,   // initial distance
              250,  // min distance
              .2f,  // rotation speed
              .4f,  // damping
              0);   // initial elevation
```

### Projection

- Handled by `arcball.setProjection(false)`.
- Typically:
  - FOV ~ 60°
  - Near/Far ~ 0.1 / 5000.0
  - Uses a perspective projection equivalent to `gluPerspective`.

### ModelView

- View matrix controlled by ArcBall; allows orbit, pan, and zoom.
- Used as the main editing view.

## Top View (Orthographic)

Fixed orthographic camera looking straight down.

### Projection

```cpp
float aspect = float(w()) / float(h());
float wi, he;

if (aspect >= 1) {
    wi = 110;
    he = wi / aspect;
} else {
    he = 110;
    wi = he * aspect;
}

glMatrixMode(GL_PROJECTION);
glOrtho(-wi, wi, -he, he, 200, -200);
```

- Orthographic bounds: ±110 units, adjusted by aspect ratio.
- Near/Far reversed (200, -200) since the scene is viewed from above.

### ModelView

```cpp
glMatrixMode(GL_MODELVIEW);
glLoadIdentity();
glRotatef(-90, 1, 0, 0); // rotate scene so Y-up is visible from top
```

- Directional light always enabled.
- Point and spot lights disabled for a clean, flat editing lighting.

## Train View (Follow Camera)

Third-person chase camera that follows the train.

### Projection

```cpp
float aspect = float(w()) / float(h());
glMatrixMode(GL_PROJECTION);
gluPerspective(60.0, aspect, 0.1, 5000.0);
```

- FOV: 60°
- Near: 0.1
- Far: 5000.0

### ModelView

```cpp
const Pnt3f eyeOffset = trainForward * (-10.0f) + trainUp * 4.0f;
const Pnt3f eye    = trainPosition + eyeOffset;
const Pnt3f center = trainPosition + trainForward * 40.0f;

glMatrixMode(GL_MODELVIEW);
glLoadIdentity();
gluLookAt(eye.x, eye.y, eye.z,
          center.x, center.y, center.z,
          trainUp.x, trainUp.y, trainUp.z);
```

- Camera placed behind and above the train, looking forward along `trainForward`.
- Train geometry is not drawn in this view to avoid clipping through the camera.

# Track Drawing

> Entry point: `void TrainView::drawTrack(bool doingShadows)`

The track is generated as a smooth 3D curve from control points using cubic splines, then **sampled densely** to produce:

- Continuous centerline positions.
- Tangent and orientation vectors along the track.
- An orthonormal frame `(tangent, right, up)` at each sample.
- Two rail line strips.
- Repeated cross ties (boxes) oriented by the local frame.

Spline mode directly affects the **shape of the track** and is selected by the UI.

## Spline Modes (Visual & Geometric Differences)

Spline mode is selected via `tw->splineBrowser->value()` and is used by `buildBasisMatrix()`:

- `1 = Linear`
- `2 = Cardinal`
- `3 = Uniform B-spline`

These modes change how control points define the curve and thus how the track is drawn.

### Mode 1 — Linear (Piecewise Linear Track)

- **Basis**: linear interpolation between pairs of positions.
- **Control point requirement**: at least 2 points.
- **Geometry behavior**:
  - Track segments are straight between control points.
  - Sharp corners at control points (C0 continuity; tangent is discontinuous).
  - Good for debugging or “blocky” prototype track layouts.
- **Effect on drawing**:
  - The centerline is a polyline; rails follow these straight segments.
  - Tangents on each segment are constant; at a corner the tangent flips abruptly.
  - Cross ties align exactly with each straight segment; at corners ties suddenly reorient.
- **Arc length**:
  - Table is trivial (piecewise linear); arc-length parameterization still works but has kinks at vertices.

### Mode 2 — Cardinal Spline (C1, Tension-Adjustable Smooth Track)

- **Basis**: Cardinal spline built in `buildBasisMatrix` with tension:

  ```cpp
  float tension = currentTension(0.5f);
  float cardinalM[4][4] = {
      { -tension,        2.0f * tension,  -tension,          0.0f },
      {  2.0f - tension, tension - 3.0f,  0.0f,             1.0f },
      {  tension - 2.0f, 3.0f - 2.0f*tension, tension,      0.0f },
      {  tension,       -tension,         0.0f,             0.0f }
  };
  ```

- **Control point requirement**: at least 4 points.
- **Continuity**:
  - C1 continuous: positions and first derivatives are continuous.
- **Tension effect** (`tw->tensionSlider`):
  - Lower tension (≈ 0): smoother, more relaxed curves.
  - Higher tension (≈ 1): curve hugs control polygon more tightly, approaching polyline.
- **Geometry behavior**:
  - Smooth, flexible track: ideal for “roller-coaster-like” shapes.
  - Tangents change smoothly across control points, avoiding sudden direction changes.
  - Ties and rails smoothly follow the bends and banking defined by orientations.
- **Arc length**:
  - Non-uniform segment lengths; arc-length table is important to keep train speed visually consistent.

### Mode 3 — Uniform B-spline (C2, Very Smooth Track)

- **Basis**: uniform cubic B-spline approximated via:

  ```cpp
  const float k = 1.0f / 6.0f;
  float bSplineM[4][4] = {
      { -k,        3.0f * k,  -3.0f * k, k },
      {  3.0f * k, -6.0f * k,  0.0f,     4.0f * k },
      { -3.0f * k,  3.0f * k,  3.0f * k, k },
      {  k,        0.0f,      0.0f,     0.0f }
  };
  ```

- **Control point requirement**: at least 4 points.
- **Continuity**:
  - C2 continuous: positions and first derivatives are continuous, and even curvature (second derivative) is smooth.
- **Geometry behavior**:
  - Very smooth, flowing track; excellent for aesthetically pleasing coaster paths.
  - Control points act as **handles**, not exact interpolation points: the track usually does not pass through each control point.
  - Banking and orientation interpolation is smoother over longer spans.
- **Visual consequence**:
  - Compared to Cardinal, corners are softer and transitions longer.
  - Good for “long sweeping curves” and gentle S-bends; less ideal if you want the train to visit precise control-point positions.
- **Arc length**:
  - Segment lengths can vary significantly; arc-length table ensures uniform speed.

## Basis Matrix

> Helper: `void TrainView::buildBasisMatrix(int mode, float out[4][4]) const`

- `mode` from `tw->splineBrowser->value()`:
  - 1 = Linear
  - 2 = Cardinal (using `currentTension()` from UI)
  - 3 = Uniform B-spline

The matrix `out` is used to compute spline weights for both track drawing and train evaluation.

---

## Sampling Along the Spline

> Constants: `#define DIVIDE_LINE 1000.0f`

### Per Segment Sampling

Each logical “segment” (between `cpIndex` and `cpIndex+1`) is sampled at `DIVIDE_LINE + 1` points:

- Parameter `t` runs from `0.0` to `1.0`.
- Four control points are used (with wrap-around): `prev, curr, next, next2`.

For each sample:

1. Build parameter vector:

   \[
   T = \begin{bmatrix} t^3 & t^2 & t & 1 \end{bmatrix}^T
   \]

2. Compute weights:

   \[
   w_r = \sum_{c=0}^{3} M_{r,c} \, T_c \quad (r = 0..3)
   \]

3. Position:

   ```cpp
   Pnt3f position =
       posPrev * weights[0] +
       posCurr * weights[1] +
       posNext * weights[2] +
       posNext2 * weights[3];
   ```

4. Tangent (derivative):

   - Build derivative parameter vector:

     \[
     T' = \begin{bmatrix} 3t^2 & 2t & 1 & 0 \end{bmatrix}^T
     \]

   - Compute `dWeights` with same matrix `M`.
   - Tangent:

     ```cpp
     Pnt3f tangent =
         posPrev * dWeights[0] +
         posCurr * dWeights[1] +
         posNext * dWeights[2] +
         posNext2 * dWeights[3];
     tangent.normalize();
     ```

5. Orientation (up hint):

   ```cpp
   Pnt3f orientation =
       orientPrev * weights[0] +
       orientCurr * weights[1] +
       orientNext * weights[2] +
       orientNext2 * weights[3];
   orientation.normalize();
   ```

All `position`, `tangent`, and `orientation` are stored into arrays:

- `trackCenters`
- `trackTangents`
- `trackOrients`

## Frame Construction Along Track

After sampling:

- Build per-sample orthonormal frames `(tangent, right, up)`.

### First Sample

1. Take `tangent = trackTangents[0]`.
2. Project `orient` onto plane perpendicular to `tangent`:

   ```cpp
   float dot = tangent · orient;
   Pnt3f up = orient - dot * tangent;
   ```

3. If `up` is too small, use a fallback (world up or world right) and re-project.
4. Normalize `up`.
5. Compute `right` via cross product:

   ```cpp
   Pnt3f right = tangent * up; // custom type uses cross
   right.normalize();
   up = right * tangent;
   up.normalize();
   ```

6. Store in `upVectors[0]` and `rightVectors[0]`.

### Subsequent Samples

For each `i`:

- Compute `up` from `orient` and `tangent` as above.
- If `up` too small, reuse `upVectors[i-1]`.
- Continuity check:

  ```cpp
  float continuityDot = up · upVectors[i-1];
  if (continuityDot < 0.0f) {
      up = -up; // flip to avoid 180° roll
  }
  ```

- Recompute `right = tangent × up`, `up = right × tangent`, and normalize both.
- Store into `upVectors[i]`, `rightVectors[i]`.

This ensures a stable, continuous frame even on tight curves or loops.

## Arc-Length Resampling (Track Geometry)

If `tw->arcLength->value()` is **enabled**, the track geometry is resampled to uniform spacing:

1. Build `cumLen[i]` along `trackCenters` (distance from start).
2. Total length `totalLen = cumLen.back()`.
3. For each sample index `sample`:
   - Compute target length `target = totalLen * sample / (sampleCount-1)`.
   - Find bracketing indices via linear scan (with `baseIdx`) on `cumLen`.
   - Interpolate position and up vector between `baseIdx` and `nextIdx`.
4. Recompute tangents, rights, and ups on the resampled set, enforcing continuity again.

Effect:

- Rails and ties become **uniformly spaced** in world units.
- Visual spacing remains constant even if the underlying spline sampling would have been non-uniform.

## Rail Drawing

Constants:

- `GUAGE = 5.0f` (track gauge).
- `railOffset = GUAGE / 2.0f`.
- `tieThickness = 0.8f` (shared with tie geometry).
- `railHeight = tieThickness * 0.5f` (rails rest above ties).

### Geometry

- Two rails, each a `GL_LINE_STRIP` offset along ±right and lifted by `railHeight`.
- For each rail (`side = -1` for left, `+1` for right):

  ```cpp
  for (size_t s = 0; s < pointCount; ++s) {
      glBegin(GL_LINE_STRIP);
      for (int k = 0; k <= (int)DIVIDE_LINE; ++k) {
          size_t idx = s * ((int)DIVIDE_LINE + 1) + k;
          const Pnt3f& right = rightVectors[idx];
          const Pnt3f& up    = upVectors[idx];

          Pnt3f railPos =
              trackCenters[idx] +
              right * (side * railOffset) +
              up * railHeight;

          glVertex3f(railPos.x, railPos.y, railPos.z);
      }
      glEnd();
  }
  ```

- Color:
  - Light gray when `!doingShadows`.
  - No color/material changes when `doingShadows == true`.

## Cross Tie Drawing

Ties are drawn as small boxes at regular intervals along the track.

### Parameters

- Tie interval: every 40 samples (`tieInterval = 40`).
- Tie width (along tangent): `tieWidth = 1.5`.
- Tie thickness (vertical): `tieThickness = 0.8`.

### Construction

At each `tieIndex`:

1. Get local frame:

   ```cpp
   const Pnt3f& right   = rightVectors[tieIndex];
   const Pnt3f& up      = upVectors[tieIndex];
   const Pnt3f& tangent = trackTangents[tieIndex];
   ```

2. Centerline endpoints extending slightly beyond rails:

   ```cpp
   Pnt3f leftEnd  = trackCenters[tieIndex] + right * (-railOffset * 1.3f);
   Pnt3f rightEnd = trackCenters[tieIndex] + right * ( railOffset * 1.3f);
   ```

3. Half-length along tangent (scalar, not cross):

   ```cpp
   Pnt3f halfWidth(
       tangent.x * (tieWidth * 0.5f),
       tangent.y * (tieWidth * 0.5f),
       tangent.z * (tieWidth * 0.5f));
   ```

4. Thickness vector along `up`:

   ```cpp
   Pnt3f thickness = up * tieThickness;
   ```

5. Build 8 vertices (box corners).
6. Draw 6 faces with quads, using per-face normals:
   - Top/bottom: normals ±`up`.
   - Front/back: ±`tangent`.
   - Left/right: ±`right`.

### Color

- Brown (approx `(139, 90, 43)`) when **not** rendering shadows.
- No color/material updates when `doingShadows == true`.

# Train Drawing

> Entry point: `void TrainView::drawTrain(bool doingShadows)`

The train is represented as a simple box that rides along the spline-defined track.

## Parameterization & Spline Mode

- Uses the same spline modes as the track (`tw->splineBrowser->value()`).
- Minimum control points:
  - Linear: ≥ 2
  - Cardinal/B-spline: ≥ 4

### Raw Parameter

```cpp
float rawParam = m_pTrack->trainU; // can be any real
```

- Wrapped into `[0, pointCount)`:

  ```cpp
  if (rawParam < 0.0f) {
      rawParam = fmod(rawParam, float(pointCount)) + float(pointCount);
  }
  float wrappedParam = fmod(rawParam, float(pointCount));
  if (wrappedParam < 0.0f)
      wrappedParam += float(pointCount);
  ```

- Segment index and local `t`:

  ```cpp
  size_t segmentIndex = size_t(floor(wrappedParam)) % pointCount;
  float  localT       = wrappedParam - floor(wrappedParam);
  ```

---

## Arc-Length Parameterization (Train)

When `tw->arcLength->value()` is enabled:

1. Build per-segment arc-length table (local lambda `buildArcLengthTable`).
2. Total length `totalLen` stored in `ArcLengthTable`.
3. Use `wrappedParam / pointCount` as normalized fraction along loop.
4. Convert to a target length along the loop.
5. `mapLengthToSegment` maps target length to `(segmentIndex, localT)`.

Result:

- Train moves at **approximately constant speed** in world space, regardless of segment spacing or spline mode.

---

## Position, Up-Hint, Tangent

For segment indices (`idxPrev, idxCurr, idxNext, idxNext2`) and basis `M`:

1. Compute weights for `localT` (same as track).
2. Position:

   ```cpp
   Pnt3f position =
       cpPrev.pos * weights[0] +
       cpCurr.pos * weights[1] +
       cpNext.pos * weights[2] +
       cpNext2.pos * weights[3];
   ```

3. Up hint:

   ```cpp
   Pnt3f up =
       cpPrev.orient * weights[0] +
       cpCurr.orient * weights[1] +
       cpNext.orient * weights[2] +
       cpNext2.orient * weights[3];
   ```

4. Tangent from derivative weights (`dWeights`).

5. Handle degenerate tangent:

   - If squared length < epsilon, fallback `(0, 0, 1)`.

---

## Tangent Direction Consistency (Arc Length)

Even in arc-length mode, the derivative direction may not match the forward direction of arc-length traversal.

To fix:

1. Take a small step `testStep` forward in `wrappedParam`.
2. Map this test parameter through arc-length mapping (reusing `buildArcLengthTable` and `mapLengthToSegment`).
3. Evaluate `testPos` at the test segment and `testLocalT`.
4. Compute `toTest = testPos - position`.
5. If `dot(tangent, toTest) < 0`, flip `tangent`.

---

## Building Train Frame

1. Normalize `up` or fall back to `(0,1,0)` if near-zero.
2. Compute `right = tangent × up`; normalize (fallback `(1,0,0)` if necessary).
3. Recompute `up = right × tangent`; normalize.

Update train globals:

```cpp
trainPosition = position;
trainForward  = tangent;
trainUp       = up;
```

Used by:

- Train camera (`setProjection`).
- Spot light (`setLighting`).

---

## Train Geometry

- Only drawn when `!tw->trainCam->value()`.

### Box Layout

- Half-extent: `5.0f` in each local axis (size `10×10×10`).
- Local axes:
  - Forward: `tangent` (front of train).
  - Right:   `right`.
  - Up:      `up`.

- Box center: `position + up * halfExtent` to lift the train above the centerline.

- 8 corners built from `center ± halfForward ± halfRight ± halfUp`.

### Rendering

- 6 faces drawn with `GL_QUADS`, correct normals for each face.
- Color:
  - Front face: green-like (`89, 110, 57`).
  - Others: white.
  - All colors skipped when `doingShadows == true`.

# Arc Length (Shared Concept)

> Used in both track resampling (in `drawTrack`) and train parameterization (in `drawTrain`).

The concept is the same:

- Precompute cumulative lengths over discretized samples of the spline.
- Use binary search and interpolation to map between distance and parameter (`t` or segment index).

Differences:

- `drawTrack` uses a **single flattened list** of samples over the loop.
- `drawTrain` uses a per-segment `ArcLengthTable` with 2D arrays for `cumLen` and `tSamples`.

# Scenery (Oden)

> Entry point: `void TrainView::drawOden(bool doingShadows)`

Three simple shapes arranged as an “Oden” dish:

1. **Tofu Block** — large base box.
2. **Pork Ball** — GLU sphere.
3. **Pig Blood Cake** — smaller box.

All rendered with flat or smooth normals and basic colors when not in shadow pass.

- Faces defined explicitly with `glBegin(GL_QUADS)` / `glEnd()`.
- Normals per face to interact properly with lighting.

# User Interface

> Main interaction: `int TrainView::handle(int event)` and `TrainWindow` widgets.

## Lighting Controls

- Directional: `tw->directionalLightButton->value()`
- Point:       `tw->pointLightButton->value()`
- Spot:        `tw->spotLightButton->value()`

## Tension Slider

- `tw->tensionSlider->value()` returns tension in `[0.1, 1]`.
- Used in Cardinal spline basis matrix.

# Rendering Pipeline

> Entry point: `void TrainView::draw()`

The main render function orchestrates OpenGL setup, drawing the scene, and drawing shadows.

## Steps

1. **Initialize GLAD** (once per context):

   ```cpp
   if (!gladLoadGL()) {
       throw std::runtime_error("Could not initialize GLAD!");
   }
   ```

2. **Viewport & Clear**:

   ```cpp
   glViewport(0, 0, w(), h());
   glClearColor(0, 0, .3f, 0); // blue-ish background
   glClearStencil(0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
   glEnable(GL_DEPTH);
   ```

3. **Projection Setup**:

   ```cpp
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   setProjection();
   ```

4. **Lighting Setup**:

   ```cpp
   setLighting();
   glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
   ```

5. **Fixed-Function Pipeline**:

   ```cpp
   glUseProgram(0); // ensure fixed-function
   ```

6. **Floor**:

   - `setupFloor()` and `drawFloor(200, 10)` draw a grid on the ground plane.

7. **Scene Objects (non-shadow pass)**:

   ```cpp
   glEnable(GL_LIGHTING);
   setupObjects(); // materials, textures if needed
   drawStuff();    // aliases drawStuff(false)
   ```

   `drawStuff`:
   - Control points (unless in train cam).
   - Track (rails & ties).
   - Train body.
   - Oden scenery.

8. **Shadow Pass (if not in top view)**:

   ```cpp
   if (!tw->topCam->value()) {
       setupShadows();
       drawStuff(true); // geometry only, no color changes
       unsetupShadows();
   }
   ```

   - Uses stencil buffer and projected matrix to render shadows onto the floor.