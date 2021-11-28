#version 450

#include "render_modes.h"

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 normal;
} ubo;

layout(binding = 3) uniform ViewProjPosNearFar {
    mat4 view;
    mat4 proj;
    vec3 pos;
    vec2 nearFar;
} lighting;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 vertNormal;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 normalInterp;
layout(location = 3) out vec3 vertPos;
layout(location = 4) out vec3 shadowCoord;
layout(location = 5) out vec3 skyCoord;

layout( push_constant ) uniform constants {
    mat4 view;
    mat4 projection;
    vec3 pointing;
    int index;
    int type;
    vec3 teamColor;
} pushConstants;

void main() {
    vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
    vec4 vertPos4 = pushConstants.view * worldPos;
    vertPos = vec3(vertPos4) / vertPos4.w;
    normalInterp = vec3(ubo.normal * vec4(vertNormal, 0.0));
    fragTexCoord = inTexCoord;
    // Leave this in for now for debugging stuff if we want
    outColor = pushConstants.teamColor;
    skyCoord = inPosition;

    vec4 inLightingSpace = lighting.proj * lighting.view * worldPos;
    shadowCoord = inLightingSpace.xyz / inLightingSpace.w;

    vec4 inClipSpace = pushConstants.projection * vertPos4;
    // draw behind everything even if in front
    if (getRINT(pushConstants.type) == RINT_SKYBOX) inClipSpace = inClipSpace.xyww;
    gl_Position = inClipSpace;
}
