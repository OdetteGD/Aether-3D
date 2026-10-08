#version 450

layout(location = 0) in vec3 worldNormal;
layout(location = 1) in vec2 uv;
layout(location = 0) out vec4 outColor;

struct PointLight {
    vec4 positionRadius;
    vec4 colorIntensity;
};

layout(set = 0, binding = 0, std430) readonly buffer LightBuffer {
    PointLight lights[];
};

layout(set = 0, binding = 2, std430) readonly buffer TileHeaderBuffer {
    uvec2 headers[];
};

layout(push_constant) uniform ForwardPlusConstants {
    mat4 inverseProjection;
    uint screenWidth;
    uint screenHeight;
    uint lightCount;
    uint maxLightsPerTile;
} constants;

void main() {
    const uvec2 pixel = uvec2(gl_FragCoord.xy);
    const uint tilesX = (constants.screenWidth + 15u) / 16u;
    const uint tileIndex = (pixel.y / 16u) * tilesX + (pixel.x / 16u);
    const uvec2 header = headers[tileIndex];

    const vec3 normal = normalize(worldNormal);
    vec3 lighting = vec3(0.03);

    for (uint i = 0u; i < header.y; ++i) {
        const uint lightIndex = header.x + i;
        if (lightIndex >= constants.lightCount) break;

        const PointLight light = lights[lightIndex];
        lighting += light.colorIntensity.rgb *
                     light.colorIntensity.a *
                     max(dot(normal, vec3(0.0, 1.0, 0.0)), 0.0);
    }

    outColor = vec4(lighting, 1.0);
}
