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
    const float minZoom2 = 1.0, maxZoom2 = 10000.0;
    const float gimbleStop = 0.1;
    const float minClip = 0.1, maxClip = 150.0;
    const float renderAsIcon2 = 10000.0;
} Camera;

namespace Net {
    static constexpr uint msPerTick = 33;
    static constexpr float ticksPerSecond = 1000.0f / (float)msPerTick;
    static constexpr float secondsPerTick = 1.0f / ticksPerSecond;
}

const float iconNDCSize = 0.05f;
const short port = 5555;

// It is not hard to make this runtime configurable
const int maxTeams = 16;

const int maxSoundSources = 32;

namespace Util {
    template<unsigned... digits>
    struct to_chars { static const char value[]; };

    template<unsigned... digits>
    constexpr char to_chars<digits...>::value[] = { ('0' + digits)..., 0 };

    template<unsigned rem, unsigned... digits>
    struct explode : explode<rem / 10, rem % 10, digits...> { };

    template<unsigned... digits>
    struct explode<0, digits...> : to_chars<digits...> { };
}

template<unsigned num>
struct to_string : Util::explode<num> {};

const std::string portStr = to_string<port>::value;

}

#endif