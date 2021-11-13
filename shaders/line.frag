#version 450

#include "render_modes.h"

layout(location = 0) in vec4 inColor;

layout(location = 0) out vec4 outColor;

layout( push_constant ) uniform constants {
    mat4 view;
    mat4 projection;
    vec3 pointing;
    int index;
    int type;
    vec3 teamColor;
} pushConstants;

void main() {
    outColor = inColor;
}