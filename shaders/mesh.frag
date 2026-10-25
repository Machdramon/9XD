#version 450
layout(location=0) in vec3 n;
layout(location=1) in vec2 tex;
layout(location=2) in vec3 mappedColor;
layout(location=0) out vec4 outputColor;
layout(location=4) in vec3 worldPosition;
layout(location=3) flat in uint material;
layout(set=0,binding=0,std430) readonly buffer TextureData { uint pixels[]; } textureData;
layout(push_constant) uniform Settings { mat4 mvp; vec4 color; vec4 light; vec4 environment; vec4 view; } settings;
float address(float v,uint mode){return mode==1u?clamp(v,0.,1.):mode==2u?1.-abs(mod(v,2.)-1.):fract(v);}
void main(){
    float shade=1.,specular=0.;bool lit=mod(settings.light.w,2.)>.5;
    if(lit && length(n)>.0001){
        vec3 normal=normalize(n),light=normalize(settings.light.xyz);
        int style=int(settings.view.w+.5);
        float diffuse=max(dot(normal,light),0.);
        if(style==1)diffuse=diffuse<.25?.12:diffuse<.65?.5:1.;
        shade=settings.environment.x+settings.environment.y*diffuse;
        if(style==0){vec3 direction=settings.view.xyz-worldPosition;vec3 halfway=light+normalize(length(direction)>.0001?direction:light);halfway=length(halfway)>.0001?normalize(halfway):light;specular=.18*settings.environment.y*pow(max(dot(normal,halfway),0.),32.)*step(.0001,diffuse);}
    }
    vec3 color=settings.light.w>=2?mappedColor:settings.color.rgb;vec4 sampled=vec4(color,settings.color.a);
if(material>0u && settings.environment.w>.5 && !any(isnan(tex)) && !any(isinf(tex))){uint w=textureData.pixels[material],h=textureData.pixels[material+1u],offset=textureData.pixels[material+2u];
uint wrap=textureData.pixels[material+3u];vec2 coords=vec2(address(tex.x,wrap&3u),address(tex.y,(wrap>>2u)&3u));uint x=min(w-1u,uint(coords.x*float(w))),y=min(h-1u,uint((1.0-coords.y)*float(h)));
uint pixel=textureData.pixels[offset+y*w+x];sampled=unpackUnorm4x8(pixel);if(settings.environment.z>.5)sampled.rgb*=color;if(sampled.a<0.01)discard;}
outputColor=vec4(sampled.rgb*shade+vec3(specular),sampled.a);}

