#define RMODE_NONE      0
#define RMODE_FLAT      1
#define RMODE_TEXT      2

#define layerZOffset    0.001f

#define RINT_SKYBOX     0
#define RINT_OBJ        1
#define RINT_ICON       2
#define RINT_PROJECTILE 3

#define getRINT(x)      ((x) & 0x7)
#define getRFLAGS(x)    ((x) & 0xfffffff8)

#define RFLAG_NONE      0
#define RFLAG_HIGHLIGHT (1 << 3)