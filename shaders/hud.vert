#version 450

// layout(binding = 0) uniform UniformBufferObject {
//     mat4 model;
//     float highlight;
// } ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

// I may need these, but I think I will just draw in normalized device coordinates
// It would seem to be the easiest way to do things
layout( push_constant ) uniform constants {
    mat4 view;
	mat4 projection;
} pushConstants;

vec2 positions[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, -1.0),
    vec2(1.0, 1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, -1.0)
);

void main() {
    // gl_Position = pushConstants.projection * pushConstants.view * vec4(positions[gl_VertexIndex], 0.0, 1.0);
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}


// void main() {
//     gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
// }
