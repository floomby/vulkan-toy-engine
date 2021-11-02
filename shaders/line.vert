#version 450

#include "render_modes.h"

layout(binding = 0) uniform LineUBO {
    vec3 a;
    vec3 b;
} inCoords;

layout(location = 0) out vec4 outColor;

layout( push_constant ) uniform constants {
    mat4 view;
    mat4 projection;
    vec3 pointing;
    int index;
    int type;
} pushConstants;

void main() {
    gl_Position = pushConstants.projection * pushConstants.view * vec4((bool(gl_VertexIndex % 2) ? inCoords.a : inCoords.b ), 1.0);
    outColor = vec4(1.0, 1.0, 1.0, 1.0);
}