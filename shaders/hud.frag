#version 450

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputColor;

layout(location = 0) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(mix(subpassLoad(inputColor).rgb, inColor.rgb, inColor.a), 1.0);
    // outColor = subpassLoad(inputColor);
}

// void main() {
//     outColor = vec4(1.0, 0.0, 0.0, 1.0);
// }
