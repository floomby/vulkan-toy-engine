#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 normal;
    float highlight;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 vertNormal;

layout( push_constant ) uniform constants {
    mat4 projection;
    mat4 view;
    vec3 lightPosition;
} pushConstants;

// What a shader
void main() {
    gl_Position = pushConstants.projection * pushConstants.view * ubo.model * vec4(inPosition, 1.0);
}