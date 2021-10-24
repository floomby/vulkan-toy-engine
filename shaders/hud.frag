#version 450

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputColor;
layout(binding = 1) uniform sampler2D texSampler[128];

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in flat int inTexIndex;

layout(location = 0) out vec4 outColor;

void main() {
    if (inTexIndex < 0) {
        outColor = vec4(mix(subpassLoad(inputColor).rgb, inColor.rgb, inColor.a), 1.0);
        return;
    }
    // outColor = vec4(mix(subpassLoad(inputColor).rgb, inColor.rgb, texture(texSampler[inTexIndex], inTexCoord).r), 1.0);
    outColor = vec4(mix(subpassLoad(inputColor).rgb, inColor.rgb, texture(texSampler[inTexIndex], inTexCoord).r), 1.0);;
}