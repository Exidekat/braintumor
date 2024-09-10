#version 450

layout (location = 0) out vec4 fragColor;

layout(location = 0) in vec4 positionIn;
layout(location = 1) in vec4 colorIn;

//vec2 positions[3] = vec2[](
//vec2(0.0, -0.5),
//vec2(0.5, 0.5),
//vec2(-0.5, 0.5)
//);
//
//vec3 colors[3] = vec3[](
//vec3(1.0, 0.0, 0.0),
//vec3(0.0, 1.0, 0.0),
//vec3(0.0, 0.0, 1.0)
//);

layout (push_constant) uniform constants {
    float time;
} PushConstants;

void main() {
    gl_Position = positionIn;
    float off = 0.25 * (sin(PushConstants.time) + 1.0);

    fragColor = colorIn;

    if (gl_InstanceIndex == 0) {
        gl_Position.x -= off;
    } else {
        gl_Position.x += off;
    }
}