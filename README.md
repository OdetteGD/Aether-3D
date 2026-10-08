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
