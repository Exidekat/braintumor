#version 450

layout (location = 0) out vec4 fragColor;

layout(location = 0) in vec4 positionIn;
layout(location = 1) in vec4 colorIn;

layout (push_constant) uniform constants {
    float time;
} PushConstants;

void main() {
    float off = 0.25 * (sin(PushConstants.time) + 1.0);

    fragColor = colorIn;

    gl_Position = vec4(positionIn.xyz * (off + 0.5), 1.0);

    if (gl_InstanceIndex == 0) {
        gl_Position.x -= off;
    } else {
        gl_Position.x = -gl_Position.x + off;
    }
}
