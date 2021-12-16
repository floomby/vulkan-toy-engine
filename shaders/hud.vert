#version 450

#include "render_modes.h"

// layout(binding = 0) uniform UniformBufferObject {
//     mat4 model;
//     float highlight;
// } ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inSecondaryColor;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in uint inTexIndex;
layout(location = 5) in uint inGuiID;
layout(location = 6) in uint inRenderMode;
// layout(location = 7) in uint inFlags;

// I may need these, but I think I will just draw in normalized device coordinates
// It would seem to be the easiest way to do things
// layout( push_constant ) uniform constants {
//     mat4 view;
//     mat4 projection;
// } pushConstants;

layout(push_constant) uniform constants {
    vec2 dragBox[2];
    vec2 tooltipBox[2];
    uint cursorID;
} pushConstants;

vec2 positions[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, -1.0),
    vec2(1.0, 1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, -1.0)
);

vec4 dragColor = vec4(0.0, 1.0, 0.0, 0.2);
vec4 tooltipColor = vec4(0.2, 0.2, 0.2, 0.2);

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outSecondaryColor;
layout(location = 2) out vec2 outTexCoord;
layout(location = 3) out int outTexIndex;
layout(location = 4) out uint outGuiID;
layout(location = 5) out uint outCursorID;
layout(location = 6) out uint outRenderMode;
// layout(location = 7) out uint outFlags;

vec2 textureCoordForBox[6] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0)
);

void main() {
    if (gl_VertexIndex < 6) {
        // draw the viewport
        gl_Position = vec4(positions[gl_VertexIndex], 1.0, 1.0);
        outColor = vec4(0.0, 0.0, 0.0, 0.0);
        // Idk if you need to set these or not?
        outTexCoord = vec2(0.0, 0.0);
        outTexIndex = -1;
        outGuiID = 0xffffffff;
        outCursorID = pushConstants.cursorID;
        outRenderMode = RMODE_NONE;
        return;
    } else if (gl_VertexIndex < 12) {
        vec2 dragBox[6] = vec2[](
            pushConstants.dragBox[0],
            pushConstants.dragBox[1],
            vec2(pushConstants.dragBox[0].x, pushConstants.dragBox[1].y),
            pushConstants.dragBox[0],
            pushConstants.dragBox[1],
            vec2(pushConstants.dragBox[1].x, pushConstants.dragBox[0].y)
        );

        // having more than 99 layers of gui stuff seems unlikely
        gl_Position = vec4(dragBox[gl_VertexIndex - 6], 1.0 - layerZOffset, 1.0);
        outColor = dragColor;
        outTexCoord = vec2(0.0, 0.0);
        outTexIndex = -1;
        outGuiID = 0xffffffff;
        outRenderMode = RMODE_NONE;
        return;
    } else if (gl_VertexIndex < 18) {
        vec2 tooltipBox[6] = vec2[](
            pushConstants.tooltipBox[0],
            pushConstants.tooltipBox[1],
            vec2(pushConstants.tooltipBox[0].x, pushConstants.tooltipBox[1].y),
            pushConstants.tooltipBox[0],
            pushConstants.tooltipBox[1],
            vec2(pushConstants.tooltipBox[1].x, pushConstants.tooltipBox[0].y)
        );

        gl_Position = vec4(tooltipBox[gl_VertexIndex - 12], 0.0, 1.0);
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        outTexCoord = textureCoordForBox[gl_VertexIndex - 12];
        outTexIndex = -1;
        outGuiID = 0xffffffff;
        outCursorID = pushConstants.cursorID;
        outRenderMode = RMODE_TOOLTIP;
        outSecondaryColor = tooltipColor;
        return;
    } else {
        // move this to the program
        gl_Position = vec4(inPosition.xy, inPosition.z, 1.0);
        outColor = inColor;
        outTexIndex = int(inTexIndex);
        outTexCoord = inTexCoord;
        outGuiID = inGuiID;
    }
    outCursorID = pushConstants.cursorID;
    outRenderMode = inRenderMode;
    // outFlags = inFlags;
    outSecondaryColor = inSecondaryColor;
}
