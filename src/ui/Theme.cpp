#include "Theme.hpp"

#include <Geode/loader/Mod.hpp>

#include <algorithm>
#include <array>
#include <string_view>

using namespace geode::prelude;

namespace baconsistent::ui::theme {
namespace {
std::string basename(char const* expandedName) {
    if (!expandedName) {
        return {};
    }
    std::string value(expandedName);
    auto const slash = value.find_last_of("/\\");
    return slash == std::string::npos ? value : value.substr(slash + 1);
}

bool themedAsset(std::string_view name) {
    static constexpr std::array<std::string_view, 12> names{
        "panel-card.png",
        "panel-card-dark.png",
        "tab-pill.png",
        "tab-pill-active.png",
        "stage-row.png",
        "stage-row-selected.png",
        "stage-row-complete.png",
        "progress-track.png",
        "progress-fill.png",
        "round-ribbon.png",
        "bacon-separator.png",
        "header-brand.png",
    };
    return std::find(names.begin(), names.end(), name) != names.end();
}

bool surfaceAsset(std::string_view name) {
    return name == "panel-card.png" || name == "panel-card-dark.png" ||
        name == "tab-pill.png" || name == "tab-pill-active.png" ||
        name == "stage-row.png" || name == "stage-row-selected.png" ||
        name == "stage-row-complete.png";
}

ccColor3B darken(ccColor3B value, float multiplier) {
    auto clampByte = [](float v) -> GLubyte {
        return static_cast<GLubyte>(std::clamp(v, 0.f, 255.f));
    };
    return {
        clampByte(static_cast<float>(value.r) * multiplier),
        clampByte(static_cast<float>(value.g) * multiplier),
        clampByte(static_cast<float>(value.b) * multiplier),
    };
}
} // namespace

Preset preset() {
    auto const value = Mod::get()->getSettingValue<std::string>("theme-preset");
    if (value == "Night") {
        return Preset::Night;
    }
    if (value == "Turkmenistan") {
        return Preset::Turkmenistan;
    }
    if (value == "Udmurtia") {
        return Preset::Udmurtia;
    }
    if (value == "Tatarstan") {
        return Preset::Tatarstan;
    }
    return Preset::Original;
}

bool customColorsEnabled() {
    return Mod::get()->getSettingValue<bool>("custom-colors");
}

Palette palette() {
    Palette result{};
    switch (preset()) {
        case Preset::Night:
            result = {
                {239, 241, 255},
                {159, 169, 205},
                {194, 167, 255},
                {255, 126, 157},
                {114, 255, 181},
                {255, 92, 118},
                {55, 60, 87},
            };
            break;
        case Preset::Turkmenistan:
            result = {
                {255, 250, 231},
                {220, 236, 220},
                {255, 199, 44},
                {210, 38, 48},
                {95, 240, 140},
                {255, 83, 83},
                {0, 133, 58},
            };
            break;
        case Preset::Udmurtia:
            result = {
                {250, 250, 250},
                {190, 190, 190},
                {232, 34, 45},
                {255, 255, 255},
                {255, 255, 255},
                {255, 72, 78},
                {32, 32, 35},
            };
            break;
        case Preset::Tatarstan:
            result = {
                {255, 252, 245},
                {218, 232, 220},
                {0, 138, 78},
                {215, 35, 50},
                {125, 239, 160},
                {230, 68, 80},
                {70, 32, 37},
            };
            break;
        default:
            result = {
                {255, 224, 170},
                {184, 197, 225},
                {255, 218, 105},
                {255, 153, 145},
                {112, 255, 173},
                {255, 92, 92},
                {113, 91, 105},
            };
            break;
    }

    if (customColorsEnabled()) {
        result.accent = Mod::get()->getSettingValue<ccColor3B>("custom-accent-color");
        result.surface = Mod::get()->getSettingValue<ccColor3B>("custom-surface-color");
        result.text = Mod::get()->getSettingValue<ccColor3B>("custom-text-color");
        result.muted = Mod::get()->getSettingValue<ccColor3B>("custom-muted-color");
        result.success = Mod::get()->getSettingValue<ccColor3B>("custom-success-color");
        result.bacon = Mod::get()->getSettingValue<ccColor3B>("custom-bacon-color");
        result.danger = Mod::get()->getSettingValue<ccColor3B>("custom-danger-color");
    }
    return result;
}

bool isTurkmenistan() {
    return preset() == Preset::Turkmenistan;
}

bool isUdmurtia() {
    return preset() == Preset::Udmurtia;
}

bool isTatarstan() {
    return preset() == Preset::Tatarstan;
}

std::string resource(char const* expandedName) {
    auto const base = basename(expandedName);
    if (!themedAsset(base) || preset() == Preset::Original) {
        return expandedName ? std::string(expandedName) : std::string{};
    }

    std::string prefix;
    switch (preset()) {
        case Preset::Night: prefix = "night-"; break;
        case Preset::Turkmenistan: prefix = "tm-"; break;
        case Preset::Udmurtia: prefix = "ud-"; break;
        case Preset::Tatarstan: prefix = "tt-"; break;
        default: return expandedName ? std::string(expandedName) : std::string{};
    }
    return Mod::get()->expandSpriteName(prefix + base);
}

std::string pauseIconResource() {
    if (isTurkmenistan()) {
        return Mod::get()->expandSpriteName("tm-emblem.png");
    }
    if (isUdmurtia()) {
        return Mod::get()->expandSpriteName("ud-emblem.png");
    }
    if (isTatarstan()) {
        return Mod::get()->expandSpriteName("tt-emblem.png");
    }
    return Mod::get()->expandSpriteName("pause-icon.png");
}

std::string themeName() {
    switch (preset()) {
        case Preset::Night: return "Night";
        case Preset::Turkmenistan: return "Turkmenistan";
        case Preset::Udmurtia: return "Udmurtia";
        case Preset::Tatarstan: return "Tatarstan";
        default: return "Original";
    }
}

void applySurfaceTint(CCSprite* sprite, char const* expandedName, bool dark) {
    if (!sprite || !customColorsEnabled() || !surfaceAsset(basename(expandedName))) {
        return;
    }
    auto value = palette().surface;
    if (dark) {
        value = darken(value, .72f);
    }
    sprite->setColor(value);
}

} // namespace baconsistent::ui::theme
