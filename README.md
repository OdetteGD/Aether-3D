# Aether 3D

A mobile-first 3D game engine and visual editor built from scratch for Android.

## Direction

Aether is a native C++20 engine rather than a web wrapper:

- Vulkan 1.1+ renderer on Android
- Forward+ renderer architecture with GPU light/cluster resources
- Native Android NDK + CMake
- Scene/entity architecture designed for an in-app editor and runtime
- Touch-first editor shell with Play mode
- GitHub Actions produces an installable arm64-v8a debug APK

## Current milestone

The first milestone establishes the Android shell, native C++20 engine boundary, physical-device/queue selection, Vulkan surface and swapchain lifecycle, synchronized render loop, a Forward+ resource boundary, and CI.

The current renderer deliberately clears the swapchain safely. The actual clustered-light compute shader, PBR materials, scene graph, asset importer, and editor gizmos are next milestones. This keeps the first APK small and makes initialization failures diagnosable.

## Build locally

Install Android SDK 35, NDK 27.2.12479018, CMake 3.31.6 and Gradle 8.10.2:

    gradle :app:assembleDebug

APK:

    app/build/outputs/apk/debug/app-debug.apk

## Planned renderer

    Scene
      -> frustum/camera update
      -> depth prepass
      -> Forward+ light culling compute
      -> clustered light lists
      -> PBR forward shading
      -> transparent forward pass
      -> tonemap/post process
      -> UI/editor overlay

The engine will keep rendering and editor state separate so the same scene can run in Editor or Play mode.

## License

MIT


## Vulkan Forward+ + Kenney World

Aether 3D's Android renderer follows the Forward+ structure used by the referenced Vulkan project: depth pre-pass, depth-aware tile/cluster light culling, then final forward shading. The mobile path keeps the same architecture while using a 32x32 tile and 24 logarithmic depth slices.

The showcase world is built from a verified subset of **Kenney Nature Kit 2.1** (trees, rocks, bushes, grass, flowers, mushrooms and logs). CI downloads the official CC0 archive, verifies its SHA-256, and bakes the selected GLB geometry into the native Vulkan mesh before the APK build.

- Kenney Nature Kit: https://kenney.nl/assets/nature-kit
- Forward+ reference: https://github.com/zimengyang/ForwardPlus_Vulkan


## Forward+ renderer basis

Aether 3D's Vulkan renderer follows the Forward+ pipeline described by [zimengyang/ForwardPlus_Vulkan](https://github.com/zimengyang/ForwardPlus_Vulkan): depth pre-pass, screen-space tile/frustum light culling, compact light lists, and final forward shading. The Android implementation is native to Aether/ANativeWindow and is an independent implementation rather than a verbatim vendor copy. The reference documents 16x16/32x32 tile sizing, point-light frustum culling, material grouping, texture/normal/specular mapping, and debug views. See the upstream reference for the original desktop implementation and attribution.
