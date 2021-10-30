#version 450

#include "render_modes.h"

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputColor;
layout(input_attachment_index = 0, set = 0, binding = 1) uniform subpassInput iconColor;
layout(binding = 2) uniform sampler2D texSampler[128];

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec4 inSecondaryColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in flat int inTexIndex;
layout(location = 4) in flat uint inGuiID;
layout(location = 5) in flat uint inCursorID;
layout(location = 6) in flat uint inRenderMode;
// layout(location = 7) in flat uint inFlags;

layout(location = 0) out vec4 outColor;

const float smoothing = 0.5;
const float boldness = 0.5;

vec4 cubic(float v){
    vec4 n = vec4(1.0, 2.0, 3.0, 4.0) - v;
    vec4 s = n * n * n;
    float x = s.x;
    float y = s.y - 4.0 * s.x;
    float z = s.z - 4.0 * s.y + 6.0 * s.x;
    float w = 6.0 - x - y - z;
    return vec4(x, y, z, w) * (1.0/6.0);
}

vec4 textureBicubic(sampler2D smplr, vec2 texCoords){
    vec2 texSize = textureSize(smplr, 0);
    vec2 invTexSize = 1.0 / texSize;

    texCoords = texCoords * texSize - 0.5;


    vec2 fxy = fract(texCoords);
    texCoords -= fxy;

    vec4 xcubic = cubic(fxy.x);
    vec4 ycubic = cubic(fxy.y);

    vec4 c = texCoords.xxyy + vec2 (-0.5, +1.5).xyxy;

    vec4 s = vec4(xcubic.xz + xcubic.yw, ycubic.xz + ycubic.yw);
    vec4 offset = c + vec4 (xcubic.yw, ycubic.yw) / s;

    offset *= invTexSize.xxyy;

    vec4 sample0 = texture(smplr, offset.xz);
    vec4 sample1 = texture(smplr, offset.yz);
    vec4 sample2 = texture(smplr, offset.xw);
    vec4 sample3 = texture(smplr, offset.yw);

    float sx = s.x / (s.x + s.y);
    float sy = s.z / (s.z + s.w);

    return mix(mix(sample3, sample2, sx), mix(sample1, sample0, sx), sy);
}

vec4 getSubpassPixel() {
    vec4 bgPixel = subpassLoad(iconColor);
    vec4 fgPixel = subpassLoad(inputColor);
    return vec4(mix(bgPixel.rgb, fgPixel.rgb, fgPixel.a), 1.0);
}

void main() {
    switch (inRenderMode) {
        case RMODE_NONE:
            outColor = vec4(mix(getSubpassPixel().rgb, inColor.rgb, inColor.a), 1.0);
            break;
        case RMODE_FLAT:
            outColor = vec4(mix(getSubpassPixel().rgb, inColor.rgb, inColor.a), 1.0);
            break;
        case RMODE_TEXT:
            float distance = textureBicubic(texSampler[inTexIndex], inTexCoord).r;
            float alpha = smoothstep(boldness - smoothing, boldness + smoothing, distance);
            outColor = vec4(mix(mix(getSubpassPixel().rgb, inColor.rgb, inColor.a), inSecondaryColor.rgb, alpha), 1.0);
            if (inGuiID == inCursorID) {
                outColor = smoothstep(0.0, 1.0, outColor);
            }
            break;
    }
}