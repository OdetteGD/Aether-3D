#version 450
layout(location=0) in vec3 vWorld;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec3 vColor;
layout(location=0) out vec4 outColor;
layout(set=0,binding=0,std140) uniform Camera { mat4 viewProj; mat4 view; vec4 viewportNearFar; } camera;
struct Light { vec4 positionRadius; vec4 colorIntensity; };
layout(set=1,binding=0,std430) readonly buffer Lights { Light lights[]; } lightBuffer;
layout(set=1,binding=1,std430) readonly buffer Clusters { uint counts[]; } clusters;
const uint CX=16u, CY=9u, CZ=24u;
void main(){
 float NEAR_Z=camera.viewportNearFar.z, FAR_Z=camera.viewportNearFar.w;
 vec3 n=normalize(vNormal); vec3 viewPos=(camera.view*vec4(vWorld,1.0)).xyz; float d=max(-viewPos.z,NEAR_Z);
 uint sx=min(CX-1u,uint(clamp(gl_FragCoord.x/camera.viewportNearFar.x,0.0,0.999999)*float(CX)));
 uint sy=min(CY-1u,uint(clamp(gl_FragCoord.y/camera.viewportNearFar.y,0.0,0.999999)*float(CY)));
 uint sz=min(CZ-1u,uint(clamp(log(d/NEAR_Z)/log(FAR_Z/NEAR_Z),0.0,0.999999)*float(CZ)));
 uint ci=(sz*CY+sy)*CX+sx; uint count=min(clusters.counts[ci],1u); vec3 lit=vec3(0.018,0.025,0.045);
 if(count>0u){ Light L=lightBuffer.lights[0]; vec3 toL=L.positionRadius.xyz-vWorld; float dist=length(toL); vec3 ld=toL/max(dist,0.001); float atten=max(0.0,1.0-dist/L.positionRadius.w); float ndl=max(dot(n,ld),0.0); lit += vColor*L.colorIntensity.rgb*(L.colorIntensity.a*ndl*atten*atten); }
 lit += vColor*0.08; outColor=vec4(lit,1.0);
}
