# Waterfall3D

Waterfall3D is a real-time 3D spectrogram waterfall viewer built with C++, Qt 6, OpenGL 3.3 Core Profile, and CMake.

The application renders incoming spectrum rows as a height-displaced 3D surface. Each new row is uploaded directly into an OpenGL texture, and the vertex shader samples that texture to produce the animated waterfall effect. A small HUD overlay shows the current FPS and grid resolution.

This project is currently driven by a synthetic data generator, but the rendering path is designed so the generated data can be replaced with real FFT or spectrum data later.

## Features

- Real-time 3D spectrogram waterfall rendering
- OpenGL 3.3 Core Profile pipeline
- Qt 6 based desktop application
- GPU-side height displacement using a 2D floating-point texture
- Ring-buffer style texture updates for continuous streaming
- 1D colormap texture for amplitude coloring
- Separate worker thread for data generation
- Triple-buffered spectrum row pool using `std::shared_ptr<std::vector<float>>`
- Simple HUD overlay showing FPS and grid size
- Mouse-controlled camera rotation and zoom
- CMake build system
- Windows deployment support through `windeployqt`

## Preview

The application displays a continuously updating 3D waterfall surface.

Each row represents one spectrum frame.

- X axis: spectrum bin index
- Z axis: time/history direction
- Y axis: amplitude
- Color: amplitude mapped through a colormap

## Project Structure
```text
Waterfall3D/
├── CMakeLists.txt
├── main.cpp
├── SpectrogramWidget.h
├── SpectrogramWidget.cpp
├── DataWorker.h
├── DataWorker.cpp
├── HudOverlay.h
├── HudOverlay.cpp
└── app_icon.rc              # Optional, Windows only

## Main Components

### `SpectrogramWidget`

`SpectrogramWidget` is the OpenGL rendering widget.

It is responsible for:

- Creating the OpenGL context-dependent resources
- Building shader programs
- Creating the 3D grid mesh
- Creating the axis lines
- Creating the data texture and colormap texture
- Uploading new spectrum rows into the texture
- Rendering the 3D waterfall surface
- Handling mouse camera controls
- Measuring and emitting FPS

The widget uses:

cpp
QOpenGLWidget
QOpenGLFunctions_3_3_Core
QOpenGLBuffer
QOpenGLVertexArrayObject
QOpenGLShaderProgram
QOpenGLTexture

The spectrogram resolution is defined by:

cpp
static constexpr int GRID_WIDTH = 1024;
static constexpr int GRID_DEPTH = 512;

This means:

- Each spectrum row contains `1024` float values.
- The waterfall keeps `512` rows of history.

### `DataWorker`

`DataWorker` runs in a separate `QThread`.

It generates synthetic spectrum data every 16 milliseconds, roughly targeting 60 updates per second.

The generated data is a mix of:

- A Gaussian-like peak
- A moving sine wave
- Random noise

The output format is:

cpp
std::shared_ptr<std::vector<float>>

Each vector must contain exactly `SpectrogramWidget::GRID_WIDTH` values.

### `HudOverlay`

`HudOverlay` is a lightweight Qt widget placed above the OpenGL widget.

It displays:

- Current FPS
- Grid resolution

Mouse events pass through the HUD because the widget uses:

cpp
setAttribute(Qt::WA_TransparentForMouseEvents);

This keeps camera control responsive even when the cursor is over the HUD area.

## Rendering Design

The application does not rebuild the mesh every frame.

Instead, it creates a static grid once:

text
GRID_WIDTH x GRID_DEPTH

The height is calculated in the vertex shader by sampling a floating-point texture:

glsl
float height = texture(dataTexture, vec2(u, v)).r;
vec3 displacedPos = vec3(aPos.x, height * 1.5, aPos.z);

This design is efficient because:

- Vertex positions are uploaded once.
- Index buffers are uploaded once.
- Only one row of texture data is updated per incoming spectrum frame.
- Height displacement happens on the GPU.
- Color mapping happens on the GPU.

## Ring Buffer Texture Update

The spectrogram data is stored in a 2D OpenGL texture:

cpp
m_dataTexture->setFormat(QOpenGLTexture::R32F);
m_dataTexture->setSize(GRID_WIDTH, GRID_DEPTH);

Each new spectrum row is uploaded with:

cpp
glTexSubImage2D(
GL_TEXTURE_2D,
0,
0,
m_writeRow,
GRID_WIDTH,
1,
GL_RED,
GL_FLOAT,
spectrum->data()
);

After each upload, the write row is advanced:

cpp
m_writeRow = (m_writeRow + 1) & (GRID_DEPTH - 1);

This works because `GRID_DEPTH` is required to be a power of two:

cpp
static_assert((SpectrogramWidget::GRID_DEPTH & (SpectrogramWidget::GRID_DEPTH - 1)) == 0,
"GRID_DEPTH must be a power of two so the ring-buffer index can use a bitmask.");

Using a bitmask is faster than modulo, but it only works correctly when the depth is a power of two.

## Camera Controls

| Action | Control |
|---|---|
| Rotate camera | Hold left mouse button and drag |
| Zoom in/out | Mouse wheel |

The camera is orbit-based.

Internally it uses:

- `m_cameraRadius`
- `m_cameraYaw`
- `m_cameraPitch`

Pitch is clamped to avoid flipping:

cpp
m_cameraPitch = qBound(-89.0f, m_cameraPitch - delta.y() * 0.4f, 89.0f);

## Requirements

### Software

- CMake 3.28 or newer
- C++20 compatible compiler
- Qt 6 with the following components:
  - Core
  - Gui
  - Widgets
  - OpenGLWidgets

### Graphics

- OpenGL 3.3 Core Profile compatible GPU and driver

### Tested/Expected Toolchains

This project is written in a way that should work with:

- MSVC 2022 on Windows
- MinGW with Qt 6, if Qt was installed for that compiler
- Clang or GCC on Linux, assuming Qt 6 and OpenGL development packages are installed

The provided CMake file includes MSVC-specific release optimizations.

## Build Instructions

### 1. Clone the repository

bash
git clone https://github.com/SobhanMdi/Waterfall3D.git
cd Waterfall3D

Replace the repository URL with your own.

### 2. Configure the project

If Qt is already visible to CMake:

bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

If CMake cannot find Qt, pass the Qt installation path manually.

Example on Windows:

bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="G:/Qt/6.8.3/msvc2022_64"

Example on Linux:

bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="/path/to/Qt/6.x.x/gcc_64"

### 3. Build

bash
cmake --build build --config Release

### 4. Run

On Linux or macOS-like environments:

bash
./build/Waterfall3D

On Windows with a multi-config generator such as Visual Studio:

bash
build/Release/Waterfall3D.exe

The exact output path may depend on the selected CMake generator.

## Windows Deployment

The CMake file tries to find `windeployqt` automatically:

cmake
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

If found, it runs after build and copies required Qt DLLs into the executable directory:

cmake
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
COMMAND "${WINDEPLOYQT_EXECUTABLE}"
--no-compiler-runtime
--no-translations
--dir "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
"$<TARGET_FILE:${PROJECT_NAME}>"
)

This is useful for running the application outside Qt Creator or Visual Studio.

Note that the current command uses:

bash
--no-compiler-runtime

So if the target machine does not already have the required MSVC runtime installed, you may need to install it separately or remove that option.

## CMake Overview

The project uses:

cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

Qt modules:

cmake
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets)

Linked libraries:

cmake
target_link_libraries(${PROJECT_NAME} PRIVATE
Qt6::Core
Qt6::Gui
Qt6::Widgets
Qt6::OpenGLWidgets
)

MSVC release optimizations:

cmake
target_compile_options(${PROJECT_NAME} PRIVATE
$<$<CONFIG:Release>:/O2>
$<$<CONFIG:Release>:/Ob2>
$<$<CONFIG:Release>:/Oi>
$<$<CONFIG:Release>:/Ot>
$<$<CONFIG:Release>:/fp:fast>
)

Interprocedural optimization is also enabled for Release builds on MSVC:

cmake
set_target_properties(${PROJECT_NAME} PROPERTIES
INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
)

## Data Flow

The runtime flow is:

text
DataWorker
|
| emits dataReady(shared_ptr<vector<float>>)
v
SpectrogramWidget::addNewData()
|
| uploads one row with glTexSubImage2D
v
OpenGL R32F data texture
|
| sampled in vertex shader
v
Height-displaced 3D waterfall surface

The connection between worker and renderer uses a queued Qt connection:

cpp
QObject::connect(worker, &DataWorker::dataReady,
spectrogramWidget, &SpectrogramWidget::addNewData,
Qt::QueuedConnection);

This is important because the worker runs in a separate thread, while OpenGL resource updates must happen in the GUI/OpenGL context thread.

## Current Limitations

This project is a strong rendering prototype, but it is not a complete signal analysis application yet.

Current limitations:

- The data source is synthetic.
- There is no FFT pipeline included.
- There is no audio input or SDR input.
- Amplitude values are not normalized dynamically.
- The colormap is hardcoded.
- Axis labels are not rendered.
- The OpenGL shaders are embedded directly in the C++ source.
- There is no configuration file or runtime settings panel.

These are reasonable tradeoffs for a compact real-time rendering demo, but they should be addressed if the project is expanded into a production-grade tool.

## Replacing the Demo Data with Real Spectrum Data

To connect a real signal source, replace or extend `DataWorker`.

The renderer expects each row to satisfy:

cpp
spectrum != nullptr
spectrum->size() == SpectrogramWidget::GRID_WIDTH

Each value should usually be normalized to a practical range, for example:

text
0.0 to 1.0

Values above `1.0` may still affect height, but the current colormap lookup expects a normalized range. If values are not normalized, colors may clamp at the end of the colormap.

A typical real pipeline could look like this:

text
Audio/SDR/Input Samples
|
Window Function
|
FFT
|
Magnitude
|
Log Scale / dB Conversion
|
Normalization
|
vector<float> with GRID_WIDTH elements
|
SpectrogramWidget::addNewData()

## Performance Notes

The renderer is designed to avoid expensive per-frame CPU work.

Good decisions already present in the code:

- Static mesh allocation
- Indexed rendering
- Texture-based data streaming
- GPU-side displacement
- GPU-side color mapping
- Separate worker thread
- Reused data buffers
- Bitmask ring-buffer indexing

Areas that may need attention later:

- `makeCurrent()` and `doneCurrent()` are called for every incoming row. This is acceptable for a prototype, but can become a bottleneck at higher update rates.
- `glTexSubImage2D` is called once per row. For very high-throughput data, pixel buffer objects may be worth testing.
- The grid size is fixed at compile time.
- The mesh has `1024 * 512 = 524,288` vertices and roughly `3.1 million` indices. This is fine on modern GPUs, but it is not small.
- Back-face culling is enabled. Depending on camera angle and winding order, this may hide parts of the surface if the view is changed significantly.

## Possible Improvements

Good next steps:

- Add real FFT input
- Add audio capture support
- Add SDR/IQ data support
- Add runtime controls for:
  - height scale
  - camera speed
  - colormap
  - update rate
  - amplitude range
- Add axis labels and tick marks
- Move shaders into separate `.vert` and `.frag` files
- Add shader hot-reloading for development
- Add a pause/resume button
- Add screenshot/export support
- Add command-line arguments
- Add a proper settings file
- Add CMake install rules
- Add CI builds

## Troubleshooting

### CMake cannot find Qt

Pass the Qt path manually:

bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="path/to/Qt"

On Windows, the path usually looks like:

text
C:/Qt/6.x.x/msvc2022_64

or:

text
G:/Qt/6.8.3/msvc2022_64

### Application starts but the OpenGL view is blank

Check that your GPU and driver support OpenGL 3.3 Core Profile.

The application requests:

cpp
format.setVersion(3, 3);
format.setProfile(QSurfaceFormat::CoreProfile);

If the driver cannot provide that context, rendering may fail.

### Shader compilation fails

Run the application from a terminal or debugger and inspect the Qt debug output.

Shader build errors are printed through:

cpp
qCritical() << name << "Vertex Shader Compile Failed:" << prog.log();
qCritical() << name << "Fragment Shader Compile Failed:" << prog.log();
qCritical() << name << "Shader Linking Failed:" << prog.log();

### The executable cannot find Qt DLLs on Windows

Build again and make sure `windeployqt` was found.

If not, add the Qt `bin` directory to `PATH`, or run `windeployqt` manually:

bash
windeployqt path/to/Waterfall3D.exe

### FPS is lower than expected

Possible causes:

- VSync is enabled:

cpp
format.setSwapInterval(1);

This usually caps FPS to the monitor refresh rate.

- GPU is not powerful enough for the current grid size.
- The application is running on an integrated GPU.
- Debug build is being used instead of Release.
- Driver-level settings are overriding OpenGL behavior.

For performance testing, build in Release mode.

## Author

Sobhan , Kianosh

## Status

Version: `1.0.0`

The project is usable as a real-time 3D OpenGL waterfall renderer with synthetic data. It is ready to be extended with a real signal processing pipeline.
