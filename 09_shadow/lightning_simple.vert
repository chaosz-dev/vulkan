#version 450

const vec3 colors[3] = vec3[](
    vec3(1.0f, 0.0f, 0.0f),
    vec3(0.0f, 1.0f, 0.0f),
    vec3(0.0f, 0.0f, 1.0f)
);

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec2 out_uv;
layout(location = 2) out vec3 out_normal;
layout(location = 3) out vec3 out_fragPos;

layout(push_constant) uniform PushConstants {
    layout(offset = 0*4*4*4) mat4 model;
    layout(offset = 1*4*4*4) mat4 view;
    layout(offset = 2*4*4*4) mat4 projection;
    layout(offset = 3*4*4*4) vec3 cameraPosition;
} constants;

void main() {
    vec3 current_pos = in_position;

    gl_Position = constants.projection * constants.view * constants.model * vec4(current_pos, 1.0f);

    out_color = colors[gl_VertexIndex % 3];
    out_uv = in_uv;
    out_normal = mat3(transpose(inverse(constants.model))) * in_normal;
    //out_normal = mat3(constants.model) * in_normal;
    out_fragPos = vec3(constants.model * vec4(current_pos, 1.0f));
}
