#pragma once

#include <Geode/Geode.hpp>

#include <string>

namespace baconsistent::ui::theme {

enum class Preset {
    Original,
    Night,
    Turkmenistan,
    Udmurtia,
    Tatarstan,
};

struct Palette {
    cocos2d::ccColor3B text;
    cocos2d::ccColor3B muted;
    cocos2d::ccColor3B accent;
    cocos2d::ccColor3B bacon;
    cocos2d::ccColor3B success;
    cocos2d::ccColor3B danger;
    cocos2d::ccColor3B surface;
};

[[nodiscard]] Preset preset();
[[nodiscard]] Palette palette();
[[nodiscard]] bool customColorsEnabled();
[[nodiscard]] bool isTurkmenistan();
[[nodiscard]] bool isUdmurtia();
[[nodiscard]] bool isTatarstan();
[[nodiscard]] std::string resource(char const* expandedName);
[[nodiscard]] std::string pauseIconResource();
[[nodiscard]] std::string themeName();

// Applies the optional user surface tint to card / row / pill textures. It is
// intentionally conservative so custom colors do not destroy icon artwork.
void applySurfaceTint(cocos2d::CCSprite* sprite, char const* expandedName, bool dark = false);

} // namespace baconsistent::ui::theme
