#version 450

layout (binding = 0) uniform sampler2D samplerColor;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;


void main() {
    vec4 pixel = texture(samplerColor, in_uv);

    out_color = vec4(pixel.rgb, 1.0f);
}
