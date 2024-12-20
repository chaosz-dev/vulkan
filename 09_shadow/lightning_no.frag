#version 450

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec3 in_fragPos;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform PushConstants {
    layout(offset = 0*4*4*4) mat4 model;
    layout(offset = 1*4*4*4) mat4 view;
    layout(offset = 2*4*4*4) mat4 projection;
    layout(offset = 3*4*4*4) vec3 cameraPosition;
    layout(offset = 3*4*4*4 + 1*4*4) vec3 lightPosition;
} constants;


void main() {
    out_color = vec4(in_color, 1.0);
}
