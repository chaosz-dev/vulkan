#version 450

const vec3 colors[3] = vec3[](
    vec3(1.0f, 0.0f, 0.0f),
    vec3(0.0f, 1.0f, 0.0f),
    vec3(0.0f, 0.0f, 1.0f)
);

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec3 out_debugcolor;

layout(push_constant) uniform PushConstants {
    layout(offset = 0*4*4*4) mat4 model;
    layout(offset = 1*4*4*4) mat4 lightView;
    layout(offset = 2*4*4*4) mat4 lightProjection;
};

void main() {
    vec3 current_pos = in_position;

    gl_Position = lightProjection * lightView * model * vec4(current_pos, 1.0f);

    out_debugcolor = colors[gl_VertexIndex % 3];
}

