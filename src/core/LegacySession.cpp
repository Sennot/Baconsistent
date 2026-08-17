#include "LegacySession.hpp"

#include <algorithm>

namespace baconsistent::core {

std::optional<LegacySavedKey> parseLegacySavedKey(std::string_view key) {
    constexpr std::string_view prefix = "levels.";
    if (!key.starts_with(prefix)) {
        return std::nullopt;
    }

    auto const lastDot = key.rfind('.');
    if (lastDot == std::string_view::npos || lastDot <= prefix.size() || lastDot + 1 >= key.size()) {
        return std::nullopt;
    }

    auto const levelKey = key.substr(prefix.size(), lastDot - prefix.size());
    if (!levelKey.starts_with("online:") && !levelKey.starts_with("local:")) {
        return std::nullopt;
    }

    auto const suffix = key.substr(lastDot + 1);
    if (suffix.empty()) {
        return std::nullopt;
    }

    return LegacySavedKey{std::string(levelKey), std::string(suffix)};
}

std::string legacySessionFallbackName(std::string_view levelKey) {
    if (levelKey.starts_with("online:")) {
        auto id = levelKey.substr(std::string_view("online:").size());
        return id.empty() ? "Legacy Online Session" : "Legacy Online #" + std::string(id);
    }
    if (levelKey.starts_with("local:")) {
        auto hash = levelKey.substr(std::string_view("local:").size());
        auto const shown = hash.substr(0, std::min<std::size_t>(8, hash.size()));
        return shown.empty() ? "Legacy Local Session" : "Legacy Local " + std::string(shown);
    }
    return "Legacy Baconsistent Session";
}

} // namespace baconsistent::core
