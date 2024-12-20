#version 450

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_fragPos;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D gridImage; 

layout(push_constant) uniform PushConstants {
    layout(offset = 0*4*4*4) mat4 model;
    layout(offset = 1*4*4*4) mat4 view;
    layout(offset = 2*4*4*4) mat4 projection;
    layout(offset = 3*4*4*4) vec3 cameraPosition;
    layout(offset = 3*4*4*4 + 1*4*4) vec3 lightPosition;
} constants;

vec3 lightColor    = vec3(1.0f, 1.0f, 1.0f);

struct {
    float constant;
    float linear;
    float quadratic;
} light = {
    //1.0f, 0.09f, 0.032f
    1.0f, 0.35f, 0.44f
};

void main() {
    vec4 pixel = texture(gridImage, in_uv);

    // distance based attenuation
    float distance    = length(constants.lightPosition - in_fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance +
                               light.quadratic * (distance * distance));

    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor * attenuation;

    // diffuse
    vec3 norm = normalize(in_normal);
    vec3 lightDir = normalize(constants.lightPosition - in_fragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * attenuation;

    // final result
    vec3 result = (ambient + diffuse) * pixel.rgb;
    out_color = vec4(result, 1.0);
}
