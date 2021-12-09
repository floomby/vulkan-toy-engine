#define layerZOffset    0.001f

#define ECONF_MAX_HUD_TEXTURES      512
#define ECONF_MAX_ENTITY_TEXTURES   1024
#define ECONF_MAX_GLYPH_TEXTURES    512
#define ECONF_MAX_LINES_IN_TEXT     64

#ifdef __GNUG__
#pragma once

#include <string>
namespace GuiTextures {

const std::string glyphsToCache = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

}

namespace Config {

// Idk if this should be runntime defined or not
struct {
    const float minZoom2 = 1.0, maxZoom2 = 400.0;
    const float gimbleStop = 0.1;
    const float minClip = 0.1, maxClip = 150.0;
    const float renderAsIcon2 = 10000.0;
} Cammera;

namespace Net {
    static constexpr uint msPerTick = 33;
    static constexpr float ticksPerSecond = 1000.0f / (float)msPerTick;
    static constexpr float secondsPerTick = 1.0f / ticksPerSecond;
}

const float iconNDCSize = 0.05f;

}

#endif