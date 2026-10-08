#version 450
layout(location=0) in vec3 vWorld;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec3 vColor;
layout(location=3) in vec3 vSkyDir;
layout(location=0) out vec4 outColor;
layout(set=0,binding=0,std140) uniform Camera {
 mat4 viewProj;mat4 view;vec4 viewportNearFar;mat4 lightViewProj;vec4 cameraPosTime;vec4 sunDirIntensity;vec4 skyParams;
} camera;
struct Light { vec4 positionRadius; vec4 colorIntensity; };
layout(set=1,binding=0,std430) readonly buffer Lights { Light lights[4]; } lightBuffer;
layout(set=1,binding=1,std430) readonly buffer Clusters { uint data[]; } clusters;
layout(set=1,binding=2) uniform sampler2DShadow shadowMap;
layout(push_constant) uniform RenderMode { uint mode; } renderMode;
const uint CX=16u,CY=9u,CZ=24u,CLUSTERS=CX*CY*CZ,MAX_LIGHTS=4u;

float hash21(vec2 p){p=fract(p*vec2(123.34,456.21));p+=dot(p,p+45.32);return fract(p.x*p.y);}
float noise2(vec2 p){vec2 i=floor(p),q=fract(p);q=q*q*(3.0-2.0*q);return mix(mix(hash21(i),hash21(i+vec2(1,0)),q.x),mix(hash21(i+vec2(0,1)),hash21(i+vec2(1,1)),q.x),q.y);}
float fbm(vec2 p){float s=0.0,a=0.5;for(int i=0;i<4;i++){s+=noise2(p)*a;p=p*2.01+17.13;a*=.5;}return s;}

vec3 skyColor(vec3 dir){
 dir=normalize(dir);vec3 sunDir=normalize(camera.sunDirIntensity.xyz);
 float elev=clamp(dir.y*.5+.5,0.0,1.0);
 vec3 horizon=vec3(.74,.82,.94),zenith=vec3(.16,.34,.70);
 vec3 sky=mix(horizon,zenith,pow(elev,.72));
 float ray=pow(max(1.0-dir.y,0.0),2.2);sky+=vec3(.10,.16,.28)*ray*.42;
 float sunElev=clamp(sunDir.y*.5+.5,0.0,1.0);
 float sunset=pow(1.0-sunElev,2.0)*smoothstep(-.15,.35,dir.y);
 sky=mix(sky,sky+vec3(1.0,.30,.06)*.22,sunset);
 float d=max(dot(dir,sunDir),0.0);
 float disk=smoothstep(cos(radians(.29)),cos(radians(.12)),d);
 float halo=pow(d,64.0)*.10+pow(d,10.0)*.035;
 sky+=vec3(1.0,.83,.55)*(disk*18.0+halo);
 vec2 p=dir.xz/max(dir.y+.20,.20)*camera.skyParams.z+vec2(camera.cameraPosTime.w*camera.skyParams.y,0.0);
 float n=fbm(p),n2=fbm(p*2.35+vec2(7.2,-3.4));
 float cloud=smoothstep(.53,.68,n*.72+n2*.28)*smoothstep(.02,.22,dir.y);
 float silver=0.55+0.45*max(dot(sunDir,normalize(vec3(dir.x, max(dir.y,.18), dir.z))),0.0);
 vec3 cloudCol=mix(vec3(.62,.68,.76),vec3(1.0,.97,.91),silver);
 sky=mix(sky,cloudCol,cloud*.68);
 return sky;
}

float sunShadow(vec3 wp,vec3 n){
 vec4 sc=camera.lightViewProj*vec4(wp+n*.018,1.0);if(sc.w<=0.0)return 1.0;
 vec3 q=sc.xyz/sc.w*.5+.5;if(q.x<.002||q.x>.998||q.y<.002||q.y>.998||q.z<0.0||q.z>1.0)return 1.0;
 float t=1.0/1024.0,b=.0018,s=0.0;
 s+=texture(shadowMap,vec3(q.xy+vec2(-1,-1)*t,q.z-b));s+=texture(shadowMap,vec3(q.xy+vec2(1,-1)*t,q.z-b));
 s+=texture(shadowMap,vec3(q.xy+vec2(-1,1)*t,q.z-b));s+=texture(shadowMap,vec3(q.xy+vec2(1,1)*t,q.z-b));
 return s*.25;
}
float D_GGX(float NoH,float a){float a2=a*a,f=NoH*NoH*(a2-1.0)+1.0;return a2/(3.14159265*f*f);}
float G_Schlick(float NoV,float k){return NoV/(NoV*(1.0-k)+k);}
float G_Smith(float NoV,float NoL,float r){float k=(r+1.0)*(r+1.0)/8.0;return G_Schlick(NoV,k)*G_Schlick(NoL,k);}
vec3 F_Schlick(float VoH,vec3 F0){return F0+(1.0-F0)*pow(clamp(1.0-VoH,0.0,1.0),5.0);}
vec3 pbrLight(vec3 base,float metallic,float rough,vec3 n,vec3 v,vec3 l,vec3 radiance,float shadow){
 float NoV=max(dot(n,v),0.0),NoL=max(dot(n,l),0.0);if(NoL<=0.0||NoV<=0.0)return vec3(0);
 vec3 h=normalize(v+l);float NoH=max(dot(n,h),0.0),VoH=max(dot(v,h),0.0);
 vec3 F0=mix(vec3(.04),base,metallic),F=F_Schlick(VoH,F0);float D=D_GGX(NoH,max(.045,rough*rough));float G=G_Smith(NoV,NoL,rough);
 vec3 spec=(D*G*F)/max(4.0*NoV*NoL,.001);vec3 kd=(1.0-F)*(1.0-metallic);vec3 diff=kd*base/3.14159265;
 return (diff+spec)*radiance*NoL*shadow;
}

void main(){
 if(renderMode.mode==2u){vec3 s=skyColor(vSkyDir);vec3 mapped=s/(s+vec3(1));mapped=pow(max(mapped,vec3(0)),vec3(1.0/2.2));outColor=vec4(mapped,1);return;}
 vec3 n=normalize(vNormal),v=normalize(camera.cameraPosTime.xyz-vWorld);
 float depth=max(-(camera.view*vec4(vWorld,1)).z,camera.viewportNearFar.z);
 uint sx=min(CX-1u,uint(clamp(gl_FragCoord.x/camera.viewportNearFar.x,0.,.999999)*float(CX)));
 uint sy=min(CY-1u,uint(clamp(gl_FragCoord.y/camera.viewportNearFar.y,0.,.999999)*float(CY)));
 uint sz=min(CZ-1u,uint(clamp(log(depth/camera.viewportNearFar.z)/log(camera.viewportNearFar.w/camera.viewportNearFar.z),0.,.999999)*float(CZ)));
 uint ci=(sz*CY+sy)*CX+sx,count=min(clusters.data[ci],MAX_LIGHTS);
 vec3 base=clamp(vColor,vec3(.015),vec3(.95));float metallic=clamp(.08+.20*base.b,0.0,.72);float rough=clamp(.26+.36*(1.0-base.g),.16,.78);
 vec3 ambient=skyColor(reflect(-v,n))*(.10+.14*(1.0-rough))+vec3(.025,.035,.045);
 vec3 lit=ambient*base;
 vec3 sd=normalize(camera.sunDirIntensity.xyz);
 lit+=pbrLight(base,metallic,rough,n,v,sd,vec3(1.0,.88,.68)*camera.sunDirIntensity.w,sunShadow(vWorld,n));
 for(uint k=0u;k<MAX_LIGHTS;k++){if(k>=count)break;uint li=clusters.data[CLUSTERS+ci*MAX_LIGHTS+k];Light L=lightBuffer.lights[li];vec3 toL=L.positionRadius.xyz-vWorld;float dist=length(toL);float radius=L.positionRadius.w;if(dist>=radius)continue;vec3 l=toL/max(dist,.001);float a=1.0-dist/radius;a*=a;lit+=pbrLight(base,metallic,rough,n,v,l,L.colorIntensity.rgb*(L.colorIntensity.a*a),1.0);}
 vec3 mapped=lit/(lit+vec3(1));mapped=pow(max(mapped,vec3(0)),vec3(1.0/2.2));outColor=vec4(mapped,1);
}