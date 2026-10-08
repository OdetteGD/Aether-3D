# Third-party rendering reference

Aether-3D's mobile Forward+ design is based on the rendering approach
demonstrated by:

https://github.com/zimengyang/ForwardPlus_Vulkan

The upstream project describes depth pre-pass, grid frustums, compute light
culling, per-tile light lists and forward shading. Its sample uses GLFW and
targets Windows.

Aether-3D implements a separate Android/mobile renderer around those concepts
rather than depending on the upstream application's desktop window/bootstrap
code.

Upstream project credits:
- Liang Peng
- Zimeng Yang

Before redistributing copied upstream source or assets, verify the upstream
repository's current license and third-party notices.
