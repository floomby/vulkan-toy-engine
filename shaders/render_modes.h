#include "../econf.h"

#define RMODE_NONE      0
#define RMODE_FLAT      1
#define RMODE_TEXT      2
#define RMODE_TOOLTIP   3
#define RMODE_IMAGE     4

#define RINT_SKYBOX     0
#define RINT_OBJ        1
#define RINT_ICON       2
#define RINT_PROJECTILE 3
#define RINT_HEALTH     4
#define RINT_RESOURCE   5
#define RINT_UNCOMPLETE 6

#define getRINT(x)      ((x) & 0x7)
#define getRMODE(x)     (getRINT(x))
#define getRFLAGS(x)    ((x) & 0xfffffff8)

#define RFLAG_NONE          0
#define RFLAG_HIGHLIGHT     (1 << 3)
#define RFLAG_TRANSPARENT   (1 << 4)
#define RFLAG_NO_HOVER      (1 << 5)
#define RFLAG_CLOAKED       (1 << 6)

#ifndef __GNUG__

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

#endif