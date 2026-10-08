# Aether-3D Mobile Forward+ Renderer

This module is the native C++ rendering foundation for Aether-3D.

## Architecture

The rendering path follows the Forward+ approach demonstrated by
zimengyang/ForwardPlus_Vulkan:

1. depth pre-pass
2. screen-space tile/depth-range construction
3. compute light culling
4. compact per-tile light index lists
5. forward shading using only lights assigned to the current tile

The upstream sample is a Windows/GLFW Vulkan application. Aether-3D keeps
the engine API independent from GLFW and uses ANativeWindow for Android.

## Mobile defaults

- 16x16 tiles
- 1024 point-light capacity
- 2 frames in flight
- Vulkan 1.0 baseline for broad Android compatibility

The final tile size should be selected from device limits and runtime
benchmarking rather than being treated as a universal constant.

## Integration stages

The bootstrap in this branch establishes Android Vulkan instance/device,
queue, surface, swapchain, command pool and frame synchronization.

The next renderer layer adds:

- depth image and depth pre-pass
- light SSBO upload
- tile header/index SSBOs
- compute dispatch and synchronization barriers
- PBR opaque shading
- directional lighting and shadow atlas
- material/texture descriptor caching
- dynamic rendering
- GPU timestamps and thermal-quality scaling

Scene/editor code should only see Aether render objects and light/material
components, never raw Vulkan descriptor layouts.
