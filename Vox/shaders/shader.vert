#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition.x, inPosition.y, 0.0, 1.0);
    fragColor = vec3(inPosition.z, 1.0f, 1.0f);
}