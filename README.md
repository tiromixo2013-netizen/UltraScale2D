# UltraScale2D

Experimental Android64 Geode mod for Geometry Dash 2.2081.

## Implemented source code

The repository now contains a renderer-independent CPU image-processing core in `src/image_pipeline.hpp`:

- Nearest-neighbor resampling
- Bilinear resampling
- Catmull-Rom bicubic resampling
- Lanczos3 resampling
- Contrast-adaptive spatial sharpening (an original CAS-inspired pass, not AMD FidelityFX CAS)

The mod runs a small 2x2-to-4x4 self-test at load and reports whether the processing functions pass.

## Important implementation status

These algorithms are real C++ implementations, but they are **not yet connected to Geometry Dash's live framebuffer or texture-rendering path**. The current milestone validates the image-processing core and Android64 packaging; it does not yet change the game's displayed image.

Not implemented: AMD FSR 1/2/3, Intel XeSS, NVIDIA NIS, temporal upscaling, motion-vector/depth extraction, and frame generation. Those require renderer integration and, for temporal methods, game-provided motion/depth data. They must not be advertised as working until implemented and tested.

## Build

The GitHub Actions workflow builds the Android64 Geode package. The project uses the Geode SDK from the `GEODE_SDK` environment variable and the official Geode CMake setup.
