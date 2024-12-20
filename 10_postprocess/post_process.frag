#version 450

layout (binding = 0) uniform sampler2D samplerColor;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

vec4 doLaplace() {
    vec4 result = vec4(0.0f);

    vec2 texelSize = 1.0 / textureSize(samplerColor, 0);
    
    mat3 laplace = mat3(
        0.0f, -1.0f, 0.0f,
        -1.0f, 4.0f, -1.0f,
        0.0f, -1.0f, 0.0f
    );

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec4 otherPixel = texture(samplerColor, in_uv + ( vec2(x,y) * texelSize) );
            result += laplace[x + 1][y + 1] * otherPixel;
        }
    }

    return result;
}

vec4 doBlur() {
    vec4 result = vec4(0.0f);

    vec2 texelSize = 1.0 / textureSize(samplerColor, 0);

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec4 otherPixel = texture(samplerColor, in_uv + ( vec2(x,y) * texelSize) );
            result += otherPixel;
        }
    }

    result /= 9.0f;
    return result;
}

vec4 doSepia() {
    vec4 pixel = texture(samplerColor, in_uv);

    vec4 sepia = vec4(112, 66, 20, 255) / 255.0f;

    return mix(pixel, sepia, 0.20f);
}

void main() {
    vec4 result = vec4(1.0);

    int mode = 3;

    switch (mode) {
        case 0: {
            vec4 pixel = texture(samplerColor, in_uv);
            result = pixel;
            break;
        }
        case 1: {
            vec4 pixel = texture(samplerColor, in_uv);
            result = mix(pixel, doLaplace(), 0.8f);
            break;
        }
        case 2: result = doBlur(); break;
        case 3: result = doSepia(); break;
    }

    out_color = vec4(result.rgb, 1.0f);
}
