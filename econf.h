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

#endif