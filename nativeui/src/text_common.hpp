#pragma once

#include "surface.hpp" // For nativeui::Color

namespace palladium {

enum class TextAlign {
    Left = 0,
    Center = 1,
    Right = 2,
    Justified = 3
};

enum class TextVAlign {
    Top = 0,
    Middle = 1,
    Bottom = 2
};

struct TextShadow {
    nativeui::Color color = nativeui::Color(0, 0, 0, 0);
    float offset_x = 0.0f;
    float offset_y = 0.0f;
    float blur = 0.0f;
    bool enabled = false;
};

struct TextOutline {
    nativeui::Color color = nativeui::Color(0, 0, 0, 0);
    float width = 0.0f;
    bool enabled = false;
};

} // namespace palladium
