#include "ProfileRegistry.hpp"

#include <Geode/Geode.hpp>
#include <Geode/loader/Mod.hpp>

#include <algorithm>
#include <sstream>
#include <string_view>

using namespace geode::prelude;

namespace baconsistent {

namespace {
constexpr char const* kProfileIndexKey = "profiles.index-v2";
constexpr char const* kLegacyProfileIndexKey = "profiles.index-v1";
constexpr char const* kProfileV2ReadyKey = "profiles.v2-ready";

std::string profileKey(std::string const& id, std::string_view suffix) {
    return fmt::format("profiles.{}.{}", id, suffix);
}

std::string bindingKey(std::string const& levelKey) {
    return fmt::format("bindings-v2.{}", levelKey);
}

std::vector<std::string> decodeLines(std::string const& encoded) {
    std::vector<std::string> ids;
    std::istringstream input(encoded);
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || std::find(ids.begin(), ids.end(), line) != ids.end()) {
            continue;
        }
        ids.push_back(line);
    }
    return ids;
}
}

ProfileRegistry& ProfileRegistry::get() {
    static ProfileRegistry instance;
    return instance;
}

std::vector<std::string> ProfileRegistry::profileIds() const {
    auto encoded = Mod::get()->getSavedValue<std::string>(kProfileIndexKey, "");
    if (!Mod::get()->getSavedValue<bool>(kProfileV2ReadyKey, false)) {
        // Read the old v0.4.x registry so existing profiles remain visible, but
        // do not auto-bind or auto-create anything. The first v0.5 registry
        // mutation migrates the index and marks v2 authoritative, including an
        // intentionally empty index after deleting the last profile.
        encoded = Mod::get()->getSavedValue<std::string>(kLegacyProfileIndexKey, "");
    }
    return decodeLines(encoded);
}

void ProfileRegistry::saveProfileIds(std::vector<std::string> const& ids) const {
    std::ostringstream output;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (i != 0) {
            output << '\n';
        }
        output << ids[i];
    }
    Mod::get()->setSavedValue(kProfileIndexKey, output.str());
    Mod::get()->setSavedValue(kProfileV2ReadyKey, true);
}

std::vector<ProfileSummary> ProfileRegistry::profiles() const {
    std::vector<ProfileSummary> result;
    for (auto const& id : profileIds()) {
        result.push_back({
            id,
            Mod::get()->getSavedValue<std::string>(profileKey(id, "name"), id),
            std::max(0, Mod::get()->getSavedValue<int>(profileKey(id, "startpos-count"), 0)),
            Mod::get()->getSavedValue<bool>(profileKey(id, "legacy-2-1"), false),
        });
    }
    return result;
}

bool ProfileRegistry::hasProfile(std::string const& profileId) const {
    if (profileId.empty()) {
        return false;
    }
    auto const ids = profileIds();
    return std::find(ids.begin(), ids.end(), profileId) != ids.end();
}

std::string ProfileRegistry::profileName(std::string const& profileId) const {
    if (profileId.empty()) {
        return {};
    }
    return Mod::get()->getSavedValue<std::string>(profileKey(profileId, "name"), profileId);
}

bool ProfileRegistry::profileUsesLegacy21(std::string const& profileId) const {
    if (profileId.empty()) {
        return false;
    }
    return Mod::get()->getSavedValue<bool>(profileKey(profileId, "legacy-2-1"), false);
}

std::optional<std::string> ProfileRegistry::bindingFor(std::string const& levelKey) const {
    if (levelKey.empty()) {
        return std::nullopt;
    }
    auto value = Mod::get()->getSavedValue<std::string>(bindingKey(levelKey), "");
    if (value.empty() || !hasProfile(value)) {
        return std::nullopt;
    }
    return value;
}

void ProfileRegistry::registerProfile(
    std::string const& profileId,
    std::string const& name,
    int startPosCount,
    bool legacy21
) {
    if (profileId.empty()) {
        return;
    }

    auto ids = profileIds();
    ids.erase(std::remove(ids.begin(), ids.end(), profileId), ids.end());
    ids.insert(ids.begin(), profileId);
    saveProfileIds(ids);

    auto safeName = name.empty() ? profileId : name;
    std::replace(safeName.begin(), safeName.end(), '\n', ' ');
    std::replace(safeName.begin(), safeName.end(), '\r', ' ');
    Mod::get()->setSavedValue(profileKey(profileId, "name"), safeName);
    Mod::get()->setSavedValue(profileKey(profileId, "startpos-count"), std::max(0, startPosCount));
    Mod::get()->setSavedValue(profileKey(profileId, "legacy-2-1"), legacy21);
    flush();
}

bool ProfileRegistry::bind(std::string const& levelKey, std::string const& profileId) {
    if (levelKey.empty() || profileId.empty() || !hasProfile(profileId)) {
        return false;
    }
    Mod::get()->setSavedValue(bindingKey(levelKey), profileId);
    flush();
    return true;
}

void ProfileRegistry::unbind(std::string const& levelKey) {
    if (levelKey.empty()) {
        return;
    }
    Mod::get()->setSavedValue(bindingKey(levelKey), std::string{});
    flush();
}

bool ProfileRegistry::removeProfile(std::string const& profileId) {
    if (profileId.empty() || !hasProfile(profileId)) {
        return false;
    }

    auto ids = profileIds();
    ids.erase(std::remove(ids.begin(), ids.end(), profileId), ids.end());
    saveProfileIds(ids);

    // New profile-* identities are fully cleared. A v0.4-era profile may use
    // online:* / local:* directly; keep its metadata as part of the legacy
    // recovery backup even after removing it from the modern index.
    auto const legacyIdentity = profileId.starts_with("online:") || profileId.starts_with("local:");
    if (!legacyIdentity) {
        Mod::get()->setSavedValue(profileKey(profileId, "name"), std::string{});
        Mod::get()->setSavedValue(profileKey(profileId, "startpos-count"), 0);
        Mod::get()->setSavedValue(profileKey(profileId, "legacy-2-1"), false);
    }
    flush();
    return true;
}

void ProfileRegistry::flush() const {
    if (auto result = Mod::get()->saveData(); result.isErr()) {
        log::warn("Baconsistent: profile registry save failed: {}", result.unwrapErr());
    }
}

} // namespace baconsistent
