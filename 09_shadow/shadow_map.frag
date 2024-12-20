#version 450

layout(location = 0) in vec3 in_debugcolor;

// DISABLED fornow
//layout(location = 0) out vec4 out_color;

void main() {
    // empty as the required gl_FragDepth set is done by default
    // gl_FragDepth = gl_FragCoord.z;

    //out_color = vec4(in_debugcolor, 1.0f);
}
