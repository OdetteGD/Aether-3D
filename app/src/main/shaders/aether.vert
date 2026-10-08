#version 450
layout(location=0) in vec3 inPos; layout(location=1) in vec3 inNormal; layout(location=2) in vec3 inColor;
layout(location=0) out vec3 vWorld; layout(location=1) out vec3 vNormal; layout(location=2) out vec3 vColor;
layout(set=0,binding=0,std140) uniform Camera {
 mat4 viewProj; mat4 view; vec4 viewportNearFar; mat4 lightViewProj; vec4 cameraPosTime; vec4 sunDirIntensity; vec4 skyParams;
} camera;
layout(push_constant) uniform RenderMode { uint mode; } renderMode;
void main(){
 vec4 wp;
 if(renderMode.mode==2u) wp=vec4(camera.cameraPosTime.xyz+inPos*60.0,1.0);
 else wp=vec4(inPos,1.0);
 vWorld=wp.xyz;vNormal=inNormal;vColor=inColor;
 gl_Position=(renderMode.mode==1u?camera.lightViewProj:camera.viewProj)*wp;
}