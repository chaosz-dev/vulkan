#version 450

layout(location = 0) in vec3 in_position;

layout(push_constant) uniform PushConstants {
    layout(offset = 0) mat4 mvp;
} constants;

void main() {
    gl_Position = constants.mvp * vec4(in_position, 1.0f);
}
