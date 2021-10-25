#version 450

#include "render_modes.h"

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputColor;
layout(binding = 1) uniform sampler2D texSampler[128];

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec4 inSecondaryColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in flat int inTexIndex;
layout(location = 4) in flat uint inGuiID;
layout(location = 5) in flat uint inCursorID;
layout(location = 6) in flat uint inRenderMode;
layout(location = 7) in flat uint inFlags;

layout(location = 0) out vec4 outColor;

const float smoothing = 0.5;

void main() {
    if (inTexIndex < 0) {
        outColor = vec4(mix(subpassLoad(inputColor).rgb, inColor.rgb, inColor.a), 1.0);
        return;
    }
    // outColor = vec4(mix(subpassLoad(inputColor).rgb, inColor.rgb, texture(texSampler[inTexIndex], inTexCoord).r), 1.0);
    float distance = texture(texSampler[inTexIndex], inTexCoord).r;
    float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
    outColor = vec4(mix(subpassLoad(inputColor).rgb, inColor.rgb, alpha), 1.0);
    if (inGuiID == inCursorID) {
        outColor = vec4(1, 1, 1, 1);
    }
}