#version 450

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = subpassLoad(inputColor).rgba;
}

// void main() {
//     outColor = vec4(1.0, 0.0, 0.0, 1.0);
// }
