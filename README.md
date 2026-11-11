# UltraScale2D

Experimental Geode mod project targeting Geometry Dash Android 2.2081 (Android64).

## Current status

This is an initial build/loader scaffold only. It currently logs when the mod loads. It does **not** yet implement render scaling, AMD CAS, spatial upscaling, or frame generation.

## Build

The GitHub Actions workflow builds the Android64 target using the official `geode-sdk/build-geode-mod` action. Run it from the Actions tab with **Build Android64 Geode mod** or push a commit to `main`.

When the workflow succeeds, download the `UltraScale2D-Android64` artifact. The resulting `.geode` file can then be installed through Geode.

## Planned work

1. Confirm Android64 build and mod loading on Geometry Dash 2.2081.
2. Add an in-game settings interface.
3. Investigate a safe, measurable render-scale implementation.
4. Evaluate a CAS-style sharpening pass and feasible upscaling methods.
5. Research frame-generation feasibility separately; do not advertise it as supported until implemented and tested.
