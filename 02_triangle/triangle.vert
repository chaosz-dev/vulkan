#version 450

const vec3 positions[6] = vec3[](
    vec3(-0.5f, -0.5f, 0.0f),
    vec3(-0.5f, 0.5f, 0.0f),
    vec3(0.5f, -0.5f, 0.0f),

    vec3(-0.5f, -0.5f, 0.0f),
    vec3(0.0f, 0.5f, 0.0f),
    vec3(0.5f, -0.5f, 0.0f)
);

const vec3 colors[3] = vec3[](
    vec3(1.0f, 0.0f, 0.0f),
    vec3(0.0f, 1.0f, 0.0f),
    vec3(0.0f, 0.0f, 1.0f)
);

layout(location = 0) out vec3 out_color;

layout(push_constant) uniform PushConstants {
    layout(offset = 0) mat4 mvp;
} constants;

void main() {
    vec3 current_pos = positions[gl_VertexIndex];

    // TASK:
    // Flip the triangle upside down: (aka y axis flip)

    gl_Position = constants.mvp * vec4(current_pos, 1.0f);
    out_color = colors[gl_VertexIndex];
}
