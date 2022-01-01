#version 450

#include "render_modes.h"

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputColor;
layout(input_attachment_index = 0, set = 0, binding = 1) uniform subpassInput iconColor;
layout(binding = 2) uniform sampler2D texSampler[128];
layout(binding = 3) uniform sampler2D tooltipTexture;

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec4 inSecondaryColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in flat int inTexIndex;
layout(location = 4) in flat uint inGuiID;
layout(location = 5) in flat uint inCursorID;
layout(location = 6) in flat uint inRenderMode;
// layout(location = 7) in flat uint inFlags;

layout(location = 0) out vec4 outColor;

vec4 getSubpassPixel() {
    vec4 bgPixel = subpassLoad(iconColor);
    vec4 fgPixel = subpassLoad(inputColor);
    return vec4(mix(bgPixel.rgb, fgPixel.rgb, fgPixel.a), 1.0);
}

void main() {
    float distance, alpha;
    uint renderMode = getRMODE(inRenderMode);
    uint renderFlags = getRFLAGS(inRenderMode);

    switch (renderMode) {
        case RMODE_NONE:
            outColor = vec4(mix(getSubpassPixel().rgb, inColor.rgb, inColor.a), 1.0);
            break;
        case RMODE_FLAT:
            // outColor = vec4(mix(getSubpassPixel().rgb, inColor.rgb, inColor.a), 1.0);
            outColor = inColor;
            break;
        case RMODE_TEXT:
            // float distance = textureBicubic(texSampler[inTexIndex], inTexCoord).r;
            distance = textureBicubic(texSampler[inTexIndex], inTexCoord).r;
            if (inGuiID == inCursorID && !bool(RFLAG_NO_HOVER & renderFlags)) {
                alpha = smoothstep(0.20, 0.48, distance);
            } else {
                alpha = smoothstep(0.46, 0.54, distance);
            }
            outColor = vec4(inSecondaryColor.rgb, clamp(alpha, 0.0, inSecondaryColor.a));
            outColor = mix(inColor, outColor, outColor.a);
            break;
        case RMODE_TOOLTIP:
            // float a = texture(tooltipTexture, inTexCoord).r;
            // outColor = vec4(a, a, a, 1.0);
            // break;
            distance = textureBicubic(tooltipTexture, inTexCoord).r;
            alpha = smoothstep(0.46, 0.54, distance);
            outColor = vec4(inColor.rgb, clamp(alpha, 0.0, inColor.a));
            outColor = vec4(mix(outColor.rgb, inSecondaryColor.rgb, 1.0 - alpha), max(alpha, inSecondaryColor.a));
            break;
        case RMODE_IMAGE:
            if (bool(RFLAG_NO_HOVER & renderFlags)) {
                outColor = textureBicubic(texSampler[inTexIndex], inTexCoord);
            } else {
                vec4 col = textureBicubic(texSampler[inTexIndex], inTexCoord);
                if (inGuiID == inCursorID) {
                    outColor = vec4(col.r, col.g, col.b, col.a * inSecondaryColor.a);
                } else {
                    outColor = vec4(col.r, col.g, col.b, col.a * inColor.a);
                }
            }
            break;
    }
}