#version 450
layout(location=0) in vec3 n;
layout(location=1) in vec2 tex;
layout(location=2) in vec3 mappedColor;
layout(location=0) out vec4 outputColor;
layout(location=3) flat in uint material;
layout(set=0,binding=0,std430) readonly buffer TextureData { uint pixels[]; } textureData;
layout(push_constant) uniform Settings { mat4 mvp; vec4 color; vec4 light; } settings;
void main(){float shade=1;bool lit=mod(settings.light.w,2.)>.5;if(lit && length(n)>0.0001)shade=.25+.75*max(dot(normalize(n),normalize(settings.light.xyz)),0.);vec3 color=settings.light.w>=2?mappedColor:settings.color.rgb;vec4 sampled=vec4(color,settings.color.a);
if(material>0u && !any(isnan(tex)) && !any(isinf(tex))){uint w=textureData.pixels[material],h=textureData.pixels[material+1u],offset=textureData.pixels[material+2u];
vec2 coords=fract(tex);uint x=min(w-1u,uint(coords.x*float(w))),y=min(h-1u,uint((1.0-coords.y)*float(h)));
uint pixel=textureData.pixels[offset+y*w+x];sampled=unpackUnorm4x8(pixel);if(sampled.a<0.01)discard;}
outputColor=vec4(sampled.rgb*shade,sampled.a);}
