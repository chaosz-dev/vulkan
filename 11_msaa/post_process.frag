#version 450

layout (binding = 0) uniform sampler2D samplerColor;
layout (binding = 1) uniform sampler2DMS samplerColorMS;

layout(location = 0) in vec2 in_uv;

layout(push_constant) uniform PushConstants {
    layout(offset = 0*4*4) uvec4 postProcMode;

    // samplingMode.x == 0 -> resolvedImg/samplerColor 
    // samplingMode.x == 1 -> msaaImg/samplerColorMS
    layout(offset = 1*4*4) uvec4 samplingMode;

    // samples.x -> number of subsamples to use
    layout(offset = 2*4*4) uvec4 samples;
};

layout(location = 0) out vec4 out_color;

vec4 getPixel(vec2 uv) {
    if (samplingMode.x == 1) {
        ivec2 iuv = ivec2( uv * textureSize(samplerColorMS) );

        vec4 result = vec4(0.0f);
        for (uint idx = 0; idx < samples.x; idx++) {
            result += texelFetch(samplerColorMS, iuv, int(idx));
        }

        return result / samples.x;
    } else {
        return texture(samplerColor, uv);
    }
}

ivec2 textureSize() {
    if (samplingMode.x == 1) {
        return textureSize(samplerColorMS);
    } else {
        return textureSize(samplerColor, 0);
    }
}

vec4 doLaplace() {
    vec4 result = vec4(0.0f);

    vec2 texelSize = 1.0 / textureSize();
    
    mat3 laplace = mat3(
        0.0f, -1.0f, 0.0f,
        -1.0f, 4.0f, -1.0f,
        0.0f, -1.0f, 0.0f
    );

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec4 otherPixel = getPixel( in_uv + ( vec2(x,y) * texelSize) );
            result += laplace[x + 1][y + 1] * otherPixel;
        }
    }

    return result;
}

vec4 doBlur() {
    vec4 result = vec4(0.0f);

    vec2 texelSize = 1.0 / textureSize();

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec4 otherPixel = getPixel( in_uv + ( vec2(x,y) * texelSize) );
            result += otherPixel;
        }
    }

    result /= 9.0f;
    return result;
}

vec4 doSepia() {
    vec4 pixel = getPixel(in_uv);

    vec4 sepia = vec4(112, 66, 20, 255) / 255.0f;

    return mix(pixel, sepia, 0.20f);
}

const float u_lumaThreshold = 0.05f;
const float u_mulReduce = 1.0f / 8.0f;
const float u_minReduce = 1.0f / 128.0f;
const float u_maxSpan = 8.0f;

vec4 doFXAA() {
    ivec2 iuv = ivec2( in_uv * textureSize(samplerColorMS) );

    vec3 rgbM = texelFetch(samplerColorMS, iuv, 0).rgb;

    // https://github.com/McNopper/OpenGL/blob/master/Example42/shader/fxaa.frag.glsl
    // Sampling neighbour texels. Offsets are adapted to OpenGL texture coordinates.
    /*
    vec3 rgbNW = textureOffset(u_colorTexture, v_texCoord, ivec2(-1, 1)).rgb;
    vec3 rgbNE = textureOffset(u_colorTexture, v_texCoord, ivec2(1, 1)).rgb;
    vec3 rgbSW = textureOffset(u_colorTexture, v_texCoord, ivec2(-1, -1)).rgb;
    vec3 rgbSE = textureOffset(u_colorTexture, v_texCoord, ivec2(1, -1)).rgb;
    */
    vec3 rgbNW = texelFetch(samplerColorMS, iuv + ivec2(-1,  1), 0).rgb;
    vec3 rgbNE = texelFetch(samplerColorMS, iuv + ivec2( 1,  1), 0).rgb;
    vec3 rgbSW = texelFetch(samplerColorMS, iuv + ivec2(-1, -1), 0).rgb;
    vec3 rgbSE = texelFetch(samplerColorMS, iuv + ivec2( 1, -1), 0).rgb;

    // see http://en.wikipedia.org/wiki/Grayscale
    const vec3 toLuma = vec3(0.299, 0.587, 0.114);

    // Convert from RGB to luma.
    float lumaNW = dot(rgbNW, toLuma);
    float lumaNE = dot(rgbNE, toLuma);
    float lumaSW = dot(rgbSW, toLuma);
    float lumaSE = dot(rgbSE, toLuma);
    float lumaM = dot(rgbM, toLuma);

    // Gather minimum and maximum luma.
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    // If contrast is lower than a maximum threshold.
    if ((lumaMax - lumaMin) <= (lumaMax * u_lumaThreshold)) {
        // ... do no AA and return.
        return vec4(rgbM, 1.0);
    }

    // Sampling is done along the gradient.
    vec2 samplingDirection;
    samplingDirection.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    samplingDirection.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    // Sampling step distance depends on the luma: The brighter the sampled texels, the smaller the final sampling step direction.
    // This results, that brighter areas are less blurred/more sharper than dark areas.
    float samplingDirectionReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.25 * u_mulReduce, u_minReduce);

    // Factor for norming the sampling direction plus adding the brightness influence.
    float minSamplingDirectionFactor = 1.0 / (min(abs(samplingDirection.x), abs(samplingDirection.y)) + samplingDirectionReduce);

    // Calculate final sampling direction vector by reducing, clamping to a range and finally adapting to the texture size.
    ivec2 size = textureSize();
    vec2 texelStep = 1.0 / size;
    samplingDirection = clamp(samplingDirection * minSamplingDirectionFactor, vec2(-u_maxSpan), vec2(u_maxSpan)) * texelStep;

    // Inner samples on the tab.
    vec3 rgbSampleNeg = texelFetch(samplerColorMS, ivec2(size * (in_uv + samplingDirection * (1.0/3.0 - 0.5))), 0).rgb;
    vec3 rgbSamplePos = texelFetch(samplerColorMS, ivec2(size * (in_uv + samplingDirection * (2.0/3.0 - 0.5))), 0).rgb;

    vec3 rgbTwoTab = (rgbSamplePos + rgbSampleNeg) * 0.5;

    // Outer samples on the tab.
    vec3 rgbSampleNegOuter = texelFetch(samplerColorMS, ivec2(size * (in_uv + samplingDirection * (0.0/3.0 - 0.5))), 0).rgb;
    vec3 rgbSamplePosOuter = texelFetch(samplerColorMS, ivec2(size * (in_uv + samplingDirection * (3.0/3.0 - 0.5))), 0).rgb;

    vec3 rgbFourTab = (rgbSamplePosOuter + rgbSampleNegOuter) * 0.25 + rgbTwoTab * 0.5;

    // Calculate luma for checking against the minimum and maximum value.
    float lumaFourTab = dot(rgbFourTab, toLuma);

    // Are outer samples of the tab beyond the edge
    if (lumaFourTab < lumaMin || lumaFourTab > lumaMax){
        // ... yes, so use only two samples.
        return vec4(rgbTwoTab, 1.0);
    } else {
        // ... no, so use four samples.
        return vec4(rgbFourTab, 1.0);
    }
}

void main() {
    vec4 result = vec4(1.0);

    uint mode = postProcMode.x;

    switch (mode) {
        case 0u: {
            vec4 pixel = getPixel(in_uv);
            result = pixel;
            break;
        }
        case 1u: {
            vec4 pixel = getPixel(in_uv);
            result = mix(pixel, doLaplace(), 0.8f);
            break;
        }
        case 2u: result = doBlur(); break;
        case 3u: result = doSepia(); break;
        case 4u: result = doFXAA(); break;
    }

    out_color = vec4(result.rgb, 1.0f);
}
