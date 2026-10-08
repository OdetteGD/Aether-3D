#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(push_constant) uniform Camera {
    mat4 viewProjection;
} camera;

layout(location = 0) out vec3 worldNormal;
layout(location = 1) out vec2 uv;

void main() {
    gl_Position = camera.viewProjection * vec4(inPosition, 1.0);
    worldNormal = inNormal;
    uv = inUV;
}
