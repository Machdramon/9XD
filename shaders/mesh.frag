#version 450
layout(location=0) in vec3 n;
layout(location=1) in vec2 tex;
layout(location=2) in vec3 mappedColor;
layout(location=0) out vec4 outputColor;
layout(push_constant) uniform Settings { mat4 mvp; vec4 color; vec4 light; } settings;
void main(){float shade=1;bool lit=mod(settings.light.w,2.)>.5;if(lit && length(n)>0.0001)shade=.25+.75*max(dot(normalize(n),normalize(settings.light.xyz)),0.);vec3 color=settings.light.w>=2?mappedColor:settings.color.rgb;outputColor=vec4(color*shade,settings.color.a);}
