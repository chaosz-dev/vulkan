#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;

layout(push_constant) uniform PushConstants {
    layout(offset = 0) mat4 mvp;
} constants;

layout(location = 0) out vec2 out_uv;

void main() {
    gl_Position = constants.mvp * vec4(in_position, 1.0f);
    out_uv = in_uv;
}
