#version 450

// layout(binding = 0) uniform UniformBufferObject {
//     mat4 model;
//     float highlight;
// } ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

// I may need these, but I think I will just draw in normalized device coordinates
// It would seem to be the easiest way to do things
// layout( push_constant ) uniform constants {
//     mat4 view;
//     mat4 projection;
// } pushConstants;

layout(push_constant) uniform constants {
    vec2 dragBox[2];
} pushConstants;

vec2 positions[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, -1.0),
    vec2(1.0, 1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, -1.0)
);

// Note that this matches with a value in gui.hpp
float layerZOffset = 0.001f;
vec4 dragColor = vec4(0.0, 1.0, 0.0, 0.2);

layout(location = 0) out vec4 outColor;

void main() {
    vec2 dragBox[6] = vec2[](
        pushConstants.dragBox[0],
        pushConstants.dragBox[1],
        vec2(pushConstants.dragBox[0].x, pushConstants.dragBox[1].y),
        pushConstants.dragBox[0],
        pushConstants.dragBox[1],
        vec2(pushConstants.dragBox[1].x, pushConstants.dragBox[0].y)
    );

    // gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    // gl_Position = pushConstants.projection * pushConstants.view * vec4(positions[gl_VertexIndex], 0.0, 1.0);
    // vec2 box[6] = vec2[](
    //     vec2(pushConstants.dragBox[0].x, pushConstants.dragBox[1].y),
    //     pushConstants.dragBox[0],
    //     pushConstants.dragBox[1],
    //     vec2(pushConstants.dragBox[1].x, pushConstants.dragBox[0].y),
    //     pushConstants.dragBox[0],
    //     pushConstants.dragBox[1]
    // );
    if (gl_VertexIndex < 6) {
        // draw the viewport
        gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
        outColor = vec4(0.0, 0.0, 0.0, 0.0);
    } else if (gl_VertexIndex < 12) {
        gl_Position = vec4(dragBox[gl_VertexIndex - 6], layerZOffset, 1.0);
        outColor = dragColor;
    } else {
        gl_Position = vec4(inPosition, 1.0);
        outColor = inColor; 
    }
}
