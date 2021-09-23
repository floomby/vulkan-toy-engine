#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in float highlight;

layout(location = 0) out vec4 outColor;

float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

vec4 vectorMap(vec4 value, float min1, float max1, float min2, float max2) {
    return vec4(
        map(value.x, min1, max1, min2, max2),
        map(value.y, min1, max1, min2, max2),
        map(value.z, min1, max1, min2, max2),
        map(value.w, min1, max1, min2, max2)
    );
}

void main() {
    if (highlight > 0.5) {
        outColor = vectorMap(texture(texSampler, fragTexCoord), 0.0, 1.0, 0.2, 1.0);
    } else {
        outColor = texture(texSampler, fragTexCoord);
    }
}
