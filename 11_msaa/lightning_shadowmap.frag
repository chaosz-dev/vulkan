#version 450

const bool PCF = true;

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_fragPos;
layout(location = 3) in vec4 in_fragPosLightSpace;


layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D modelTexture;
layout(set = 0, binding = 1) uniform sampler2D shadowMap; 

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

float SimpleShadow(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // transform x-y to [0,1] range
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    if(projCoords.z > 1.0 || projCoords.z < -1.0f) {
        return 0.0f;
    }

    return currentDepth > closestDepth  ? 1.0 : 0.0;
}

float PCFShadow(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // transform x-y to [0,1] range
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    /*
    vec3 normal = normalize(in_normal);
    vec3 lightDir = normalize(constants.lightPosition - in_fragPos);
    float bias = max(0.01 * (1.0 - dot(normal, lightDir)), 0.001);
    */
    float shadow = 0.0;

    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth > pcfDepth  ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0 || projCoords.z < -1.0f) {
        shadow = 0.0;
    }

    return shadow;
}

void main() {
    vec4 pixel = texture(modelTexture, in_uv);

    // distance based attenuation
    float distance    = length(constants.lightPosition - in_fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance +
                               light.quadratic * (distance * distance));

    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;// * attenuation;

    // diffuse
    vec3 norm = normalize(in_normal);
    vec3 lightDir = normalize(constants.lightPosition - in_fragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor; //* attenuation;

    // final result
    float shadow = PCF ? PCFShadow(in_fragPosLightSpace) : SimpleShadow(in_fragPosLightSpace);

    vec3 result = (ambient + (1.0 - shadow)) * diffuse;
    out_color = vec4(result * pixel.rgb, 1.0);
}
