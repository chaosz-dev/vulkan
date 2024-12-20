#version 450

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec3 in_fragPos;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D modelTexture;

layout(push_constant) uniform PushConstants {
    layout(offset = 0*4*4*4) mat4 model;
    layout(offset = 1*4*4*4) mat4 view;
    layout(offset = 2*4*4*4) mat4 projection;
    layout(offset = 3*4*4*4) vec3 cameraPosition;
    layout(offset = 3*4*4*4 + 1*4*4) vec3 lightPosition;
} constants;

vec3 lightColor    = vec3(1.0f, 1.0f, 1.0f);

void main() {
    vec4 pixel = texture(modelTexture, in_uv);

    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
    
    // diffuse 
    vec3 norm = normalize(in_normal);
    vec3 lightDir = normalize(constants.lightPosition - in_fragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(constants.cameraPosition - in_fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  

    // combine the colors
    vec3 result = (ambient + diffuse + specular) * pixel.rgb;

    out_color = vec4(result, 1.0);

    gl_FragDepth = gl_FragCoord.z;
}
