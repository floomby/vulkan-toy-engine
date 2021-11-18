#version 450

#include "render_modes.h"

layout(binding = 0) uniform LineUBO {
    vec3 a;
    vec3 b;
    vec4 aColor;
    vec4 bColor;
} inCoords;

layout(location = 0) out vec4 outColor;

layout( push_constant ) uniform constants {
    mat4 view;
    mat4 projection;
    vec3 pointing;
    int index;
    int type;
    vec3 teamColor;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inVertNormal;

void main() {
    gl_Position = pushConstants.projection * pushConstants.view * vec4((bool(gl_VertexIndex % 2) ? inCoords.a : inCoords.b ), 1.0);
    outColor = (bool(gl_VertexIndex % 2) ? inCoords.aColor : inCoords.bColor );
}