#version 450

const vec3 colors[3] = vec3[](
    vec3(1.0f, 0.0f, 0.0f),
    vec3(0.0f, 1.0f, 0.0f),
    vec3(0.0f, 0.0f, 1.0f)
);

layout(location = 0) in vec3 in_position;

layout(location = 0) out vec3 out_color;

layout(push_constant) uniform PushConstants {
    layout(offset = 0) mat4 mvp;
} constants;

void main() {
    vec3 current_pos = in_position;

    gl_Position = constants.mvp * vec4(current_pos, 1.0f);

    out_color = colors[gl_VertexIndex % 3];
}
