#version 450
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;
layout(location=3) in vec3 vertexColor;
layout(push_constant) uniform Settings { mat4 mvp; vec4 color; vec4 light; } settings;
layout(location=0) out vec3 n;
layout(location=1) out vec2 tex;
layout(location=2) out vec3 mappedColor;
void main(){gl_Position=settings.mvp*vec4(position,1);gl_PointSize=1;n=normal;tex=uv;mappedColor=vertexColor;}
