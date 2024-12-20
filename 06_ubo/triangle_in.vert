#version 450

layout(location = 0) in vec3 in_position;

layout(location = 0) out vec3 out_color;

layout(push_constant) uniform PushConstants {
    layout(offset = 0) mat4 mvp;
} constants;

layout(set = 0, binding = 0) uniform UBO {
    vec3 colors[3];
} ubo;

void main() {
    vec3 current_pos = in_position;

    gl_Position = constants.mvp * vec4(current_pos, 1.0f);

    out_color = ubo.colors[gl_VertexIndex % 3];
}
