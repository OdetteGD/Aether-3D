#version 450
layout(location=0) in vec3 inPos;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec3 inColor;
layout(location=0) out vec3 vWorld;
layout(location=1) out vec3 vNormal;
layout(location=2) out vec3 vColor;
layout(location=3) out vec3 vSkyDir;
layout(set=0,binding=0,std140) uniform Camera {
 mat4 viewProj;
 mat4 view;
 vec4 viewportNearFar;
 mat4 lightViewProj;
 vec4 cameraPosTime;
 vec4 sunDirIntensity;
 vec4 skyParams;
} camera;
layout(push_constant) uniform RenderMode { uint mode; } renderMode;
void main(){
 if(renderMode.mode==2u){
  vec2 p=(gl_VertexIndex==0)?vec2(-1.0,-1.0):(gl_VertexIndex==1?vec2(3.0,-1.0):vec2(-1.0,3.0));
  float aspect=camera.viewportNearFar.x/max(camera.viewportNearFar.y,1.0);
  float tanHalf=tan(camera.skyParams.w*0.5);
  vec3 viewDir=normalize(vec3(p.x*aspect*tanHalf,-p.y*tanHalf,-1.0));
  vSkyDir=normalize(transpose(mat3(camera.view))*viewDir);
  vWorld=vec3(0);vNormal=vec3(0,1,0);vColor=vec3(1);
  gl_Position=vec4(p,0.9999,1);
 }else{
  vec4 wp=vec4(inPos,1.0);
  vWorld=wp.xyz;vNormal=inNormal;vColor=inColor;vSkyDir=vec3(0);
  gl_Position=(renderMode.mode==1u?camera.lightViewProj:camera.viewProj)*wp;
 }
}