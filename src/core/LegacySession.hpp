#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace baconsistent::core {

struct LegacySavedKey {
    std::string levelKey;
    std::string suffix;
};

// Parses pre-profile Baconsistent saved-value keys such as
//   levels.online:123456.counts
//   levels.local:0123abcd.startpos-2-1
// New profile payload keys (levels.profile-....*) deliberately do not match.
[[nodiscard]] std::optional<LegacySavedKey> parseLegacySavedKey(std::string_view key);
[[nodiscard]] std::string legacySessionFallbackName(std::string_view levelKey);

} // namespace baconsistent::core
