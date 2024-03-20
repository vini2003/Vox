#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(binding = 2) uniform Extras {
    vec4 colorModulation;
    float decay;
} extras;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 colorModulation;
layout(location = 3) out float decay;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0) * vec4(2.5, 1.0, 1.0, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;

    colorModulation = extras.colorModulation;
    decay = extras.decay;
}