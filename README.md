# 3D Waterfall Spectrogram

A real-time 3D waterfall renderer for spectral data, built with Qt6 and OpenGL 3.3.

New rows of data scroll into the scene continuously. Each row is drawn as a strip on
a 3D surface, where height and color map to signal intensity. Older rows drift back
along the Z axis until they leave the grid.

The data source here is synthetic. A background worker generates a moving signal
(a Gaussian peak plus a sine wave and some noise) so you can see the pipeline running
without wiring up real hardware. Swapping in a real FFT feed is just a matter of
emitting your own rows from `DataWorker`.

## How it works

The whole thing runs on two threads.

The worker thread produces one row of `GRID_WIDTH` floats every 16 ms and hands it off
through a queued signal. It cycles through three preallocated buffers so the hot path
never allocates.

The GUI thread renders. The data lives in a single `1024 x 512` floating point texture
that acts as a ring buffer. Instead of shifting pixels around when new data arrives,
we just write into the next row and bump a `headOffset` value. The vertex shader reads
that offset and wraps the texture lookup, so the GPU does all the scrolling for free.

Text overlays (FPS, grid size) are drawn with a transparent `QWidget` sitting on top of
the GL viewport, not painted into the GL scene. That keeps the text sharp at any DPI and
costs nothing on the GPU.
main.cpp

worker thread -> DataWorker generates rows, emits dataReady

gui thread -> SpectrogramWidget OpenGL render pipeline

data texture 1024x512 R32F, ring buffer

colormap 256-entry 1D texture

mesh 1024x512 grid, displaced in the vertex shader

-> HudOverlay transparent QWidget on top, FPS + resolution
Requirements

    Qt 6 (Core, Gui, Widgets, OpenGLWidgets)
    CMake 3.28 or newer
    A C++20 compiler (tested with MSVC 2022)
    A GPU with OpenGL 3.3 core profile

Building

git clone https://github.com/YOUR_USERNAME/Waterfall3D.git

cd Waterfall3D

cmake -B build

cmake --build build --config Release

If CMake can’t find Qt, point it at your install:

cmake -B build -DCMAKE_PREFIX_PATH=“C:/Qt/6.8.3/msvc2022_64”

On Windows the build runs windeployqt automatically and drops the required Qt DLLs

next to the executable, so the build folder is ready to run as-is.
Controls

    Drag with the left mouse button to orbit the camera
    Scroll to zoom in and out

Feeding your own data

SpectrogramWidget::addNewData takes a shared_ptr<vector<float>> of exactly

GRID_WIDTH values. Connect anything that emits that signal and the visualizer will

pick it up. The current DataWorker is just one example producer; replace its

generateData body with your own source.

A couple of constraints worth knowing:

    The row size has to match GRID_WIDTH (1024) or the frame is dropped.
    GRID_DEPTH must stay a power of two. The ring buffer index uses a bitmaskinstead of a modulo, and there’s a static_assert that will stop the build ifyou break this.

Project layout

main.cpp entry point, thread setup

SpectrogramWidget.h/.cpp OpenGL render engine

HudOverlay.h/.cpp transparent HUD overlay

DataWorker.h/.cpp background data generator

CMakeLists.txt

app_icon.rc / .ico Windows icon
