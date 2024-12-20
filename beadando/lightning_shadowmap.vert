#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;

layout(set = 0, binding = 2) uniform li {
    mat4 lightSpaceMatrix;
} lightInfo; 


layout(push_constant) uniform PushConstants {
    layout(offset = 0*4*4*4) mat4 model;
    layout(offset = 1*4*4*4) mat4 view;
    layout(offset = 2*4*4*4) mat4 projection;
    layout(offset = 3*4*4*4) vec3 cameraPosition;
} constants;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_fragPos;
layout(location = 3) out vec4 out_fragPosLightSpace;

const mat4 biasMat = mat4( 
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.5, 0.5, 0.0, 1.0 );

void main() {
    gl_Position = constants.projection * constants.view * constants.model * vec4(in_position, 1.0f);

    out_uv = in_uv;
    out_normal = mat3(transpose(inverse(constants.model))) * in_normal;
    out_fragPos = vec3(constants.model * vec4(in_position, 1.0f));

    out_fragPosLightSpace = /*biasMat * */ lightInfo.lightSpaceMatrix * constants.model * vec4(in_position, 1.0f);
    // out_fragPosLightSpace.xy is in [-1, 1]; range, need to normalize it to [0,1] here or in the fragment shader for uv coords
}
