#version 450
layout(location=0) in vec3 vWorld; layout(location=1) in vec3 vNormal; layout(location=2) in vec3 vColor;
layout(location=0) out vec4 outColor;
layout(set=0,binding=0,std140) uniform Camera {
 mat4 viewProj; mat4 view; vec4 viewportNearFar; mat4 lightViewProj; vec4 cameraPosTime; vec4 sunDirIntensity; vec4 skyParams;
} camera;
struct Light { vec4 positionRadius; vec4 colorIntensity; };
layout(set=1,binding=0,std430) readonly buffer Lights { Light lights[4]; } lightBuffer;
layout(set=1,binding=1,std430) readonly buffer Clusters { uint data[]; } clusters;
layout(set=1,binding=2) uniform sampler2DShadow shadowMap;
layout(push_constant) uniform RenderMode { uint mode; } renderMode;
const uint CX=16u,CY=9u,CZ=24u,CLUSTERS=CX*CY*CZ,MAX_LIGHTS=4u;
float hash21(vec2 p){p=fract(p*vec2(123.34,456.21));p+=dot(p,p+45.32);return fract(p.x*p.y);}
float noise2(vec2 p){vec2 i=floor(p),q=fract(p);q=q*q*(3.0-2.0*q);return mix(mix(hash21(i),hash21(i+vec2(1,0)),q.x),mix(hash21(i+vec2(0,1)),hash21(i+vec2(1,1)),q.x),q.y);}
float fbm(vec2 p){float s=0.0,a=0.5;for(int i=0;i<4;i++){s+=noise2(p)*a;p=p*2.03+13.7;a*=0.5;}return s;}
vec3 skyColor(vec3 dir){
 float h=clamp(dir.y*.5+.5,0.0,1.0);vec3 sky=mix(vec3(.46,.66,.92),vec3(.025,.09,.24),pow(h,.72));
 vec3 sd=normalize(camera.sunDirIntensity.xyz);float d=max(dot(dir,sd),0.0);
 sky+=vec3(1.0,.62,.25)*(pow(d,700.0)*14.0+pow(d,18.0)*.20);
 vec2 cp=dir.xz/max(dir.y+.25,.25)*camera.skyParams.z+vec2(camera.cameraPosTime.w*camera.skyParams.y);
 float clouds=smoothstep(.53,.70,fbm(cp))*camera.skyParams.x*smoothstep(-.05,.45,dir.y);
 return mix(sky,vec3(.90,.94,1.0),clouds*.82);
}
float sunShadow(vec3 wp,vec3 n){
 vec4 sc=camera.lightViewProj*vec4(wp+n*.025,1.0);if(sc.w<=0.0)return 1.0;vec3 q=sc.xyz/sc.w*0.5+.5;
 if(q.x<.002||q.x>.998||q.y<.002||q.y>.998||q.z<0.0||q.z>1.0)return 1.0;
 float t=1.0/1024.0,s=0.0;float b=.003;
 s+=texture(shadowMap,vec3(q.xy+vec2(-t,-t),q.z-b));s+=texture(shadowMap,vec3(q.xy+vec2(t,-t),q.z-b));
 s+=texture(shadowMap,vec3(q.xy+vec2(-t,t),q.z-b));s+=texture(shadowMap,vec3(q.xy+vec2(t,t),q.z-b));return s*.25;
}
vec3 Fschlick(float c,vec3 F0){return F0+(1.0-F0)*pow(clamp(1.0-c,0.0,1.0),5.0);}
void main(){
 if(renderMode.mode==2u){outColor=vec4(skyColor(normalize(vWorld-camera.cameraPosTime.xyz)),1.0);return;}
 vec3 n=normalize(vNormal),V=normalize(camera.cameraPosTime.xyz-vWorld);
 float d=max(-(camera.view*vec4(vWorld,1.0)).z,camera.viewportNearFar.z);
 uint sx=min(CX-1u,uint(clamp(gl_FragCoord.x/camera.viewportNearFar.x,0.,.999999)*float(CX)));
 uint sy=min(CY-1u,uint(clamp(gl_FragCoord.y/camera.viewportNearFar.y,0.,.999999)*float(CY)));
 uint sz=min(CZ-1u,uint(clamp(log(d/camera.viewportNearFar.z)/log(camera.viewportNearFar.w/camera.viewportNearFar.z),0.,.999999)*float(CZ)));
 uint ci=(sz*CY+sy)*CX+sx,count=min(clusters.data[ci],MAX_LIGHTS);
 vec3 base=max(vColor,vec3(.025));float metallic=.18+.22*base.r,rough=.30+.28*(1.0-base.g),power=mix(8.,96.,1.-rough);vec3 F0=mix(vec3(.04),base,metallic);
 vec3 lit=skyColor(reflect(-V,n))*.16+base*.045;
 vec3 sd=normalize(camera.sunDirIntensity.xyz),H=normalize(sd+V);float nl=max(dot(n,sd),0.),nh=max(dot(n,H),0.);
 vec3 sf=Fschlick(max(dot(H,V),0.),F0);lit+=(base/3.14159265+sf*pow(nh,power)*(1.-rough*.35))*vec3(1.,.90,.72)*(nl*camera.sunDirIntensity.w*sunShadow(vWorld,n));
 for(uint k=0u;k<MAX_LIGHTS;k++){if(k>=count)break;uint li=clusters.data[CLUSTERS+ci*MAX_LIGHTS+k];Light L=lightBuffer.lights[li];vec3 tl=L.positionRadius.xyz-vWorld;float dist=length(tl),att=clamp(1.-dist/L.positionRadius.w,0.,1.);att*=att;vec3 ld=tl/max(dist,.001),hh=normalize(ld+V);float ldn=max(dot(n,ld),0.),hdn=max(dot(n,hh),0.);vec3 ff=Fschlick(max(dot(hh,V),0.),F0);lit+=(base/3.14159265+ff*pow(hdn,power)*(1.-rough*.35))*L.colorIntensity.rgb*(L.colorIntensity.a*ldn*att);}
 vec3 mapped=lit/(lit+1.0);mapped=pow(max(mapped,vec3(0)),vec3(1./2.2));outColor=vec4(mapped,1);
}