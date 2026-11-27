#version 450

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;

layout(push_constant) uniform PushConstants {
    vec2 screenSize;
} uPush;

layout(location = 0) out vec4 fragColor;

void main() {
    gl_Position = vec4(2.0 * inPos.x / uPush.screenSize.x - 1.0,
                       2.0 * inPos.y / uPush.screenSize.y - 1.0,
                       0.0, 1.0);
    fragColor = inColor;
}
