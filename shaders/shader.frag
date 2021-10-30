#version 450

// We can change this to a bigger number (I think vulkan doesn't allocate memory or anything based on this array size, opengl would have though)
layout(binding = 1) uniform sampler2D texSampler[4];
layout(binding = 2) uniform sampler2D shadowMap;
layout(binding = 3) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 pos;
} lighting;
layout(binding = 4) uniform samplerCube skyboxSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in float highlight;
layout(location = 3) in vec3 normalInterp;
layout(location = 4) in vec3 vertPos;
layout(location = 5) in vec3 shadowCoord;
layout(location = 6) in vec3 skyCoord;

layout(location = 0) out vec4 outColor;

const vec3 lightColor = vec3(1.0, 1.0, 1.0);
const float lightPower = 40.0;
// const vec3 ambientColor = vec3(0.1, 0.0, 0.0);
// const vec3 diffuseColor = vec3(0.5, 0.0, 0.0);
const vec3 specColor = vec3(1.0, 1.0, 1.0);
const float shininess = 16.0;
const float screenGamma = 2.2; // Assume the monitor is calibrated to the sRGB color space (it should be almost all are)

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

float getShadow(vec3 ndc, uint pcfSize) {
    // outside the device viewing region
    if (abs(ndc.x) > 1.0 ||
        abs(ndc.y) > 1.0 ||
        abs(ndc.z) > 1.0) return 0.0;

    // Translate from normed device coords to shadow map space
    vec2 shadowMapCoord = ndc.xy * 0.5 + 0.5;

    int pcfSizeMinus1 = int(pcfSize - 1);
    float kernelSize = 2.0 * pcfSizeMinus1 + 1.0;
    float numSamples = kernelSize * kernelSize;

    vec2 shadowMapTexelSize = 1.0 / textureSize(shadowMap, 0);
    float litCount = 0.0;

    for (int x = -pcfSizeMinus1; x <= pcfSizeMinus1; x++) {
        for (int y = -pcfSizeMinus1; y <= pcfSizeMinus1; y++) {
            vec2 pcfCoord = shadowMapCoord + vec2(x, y) * shadowMapTexelSize;
 
            // Check if the sample is in light or in the shadow
            if (ndc.z <= texture(shadowMap, pcfCoord.xy).x * 2)
                litCount += 1.0;
        }
    }
 
   return litCount / numSamples;


    // return texture(shadowMap, shadowMapCoord.xy).x;

    // if (ndc.z > texture(shadowMap, shadowMapCoord.xy).x * 2)
    //      return 0.0;

    // // lit
    // return 1.0;
}

layout( push_constant ) uniform constants {
    mat4 view;
    mat4 projection;
    vec3 pointing;
    int index;
    int type;
} pushConstants;

const int mode = 1;

// in view space
vec3 lightPos = (pushConstants.view * vec4(lighting.pos, 1.0)).xyz;

void main() {
    if (pushConstants.type == 2) {
        outColor = texture(texSampler[pushConstants.index], fragTexCoord);
        return;
    }

    // for skybox rendering
    if (pushConstants.type == 1) {
        outColor = texture(skyboxSampler, skyCoord);
        return;
    }

    vec3 diffuseColor = texture(texSampler[pushConstants.index], fragTexCoord).rgb;
    vec3 ambientColor = diffuseColor / 5.0f;

    vec3 normal = normalize(normalInterp);
    vec3 lightDir = lightPos - vertPos;
    float distance = length(lightDir);
    distance = distance * distance;
    lightDir = normalize(lightDir);

    float lambertian = max(dot(lightDir, normal), 0.0);
    float specular = 0.0;

    if (lambertian > 0.0) {
        vec3 viewDir = normalize(-vertPos);

        // blinn phong
        vec3 halfDir = normalize(lightDir + viewDir);
        float specAngle = max(dot(halfDir, normal), 0.0);
        specular = pow(specAngle, shininess);
        
        // phong
        if (mode == 2) {
            vec3 reflectDir = reflect(-lightDir, normal);
            specAngle = max(dot(reflectDir, viewDir), 0.0);
            // NOTE: the exponent means something different here
            specular = pow(specAngle, shininess / 4.0);
        }
    }
    vec3 colorLinear = ambientColor +
        diffuseColor * lambertian * lightColor * lightPower / distance +
        specColor * specular * lightColor * lightPower / distance;
    // apply gamma correction (all the color inputs are in linear color space)
    vec3 colorGammaCorrected = pow(colorLinear, vec3(1.0 / screenGamma));
    // use the gamma corrected color in the fragment
    outColor = vec4(colorGammaCorrected, 1.0);

    if (highlight > 0.5) {
        outColor = vectorMap(outColor, 0.0, 1.0, 0.4, 1.0);
    }

    outColor = vec4((outColor * (getShadow(shadowCoord, 4) * 0.9 + 0.1)).rgb, 1.0);
}
