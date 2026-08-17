#pragma once

#include <optional>
#include <string>
#include <vector>

namespace baconsistent {

struct ProfileSummary {
    std::string id;
    std::string name;
    int startPosCount = 0;
    bool legacy21 = false;
};

// Persistent profile registry. Profiles own the training data / StartPos layout;
// physical GD levels only keep an explicit binding to a profile.
class ProfileRegistry {
public:
    static ProfileRegistry& get();

    [[nodiscard]] std::vector<ProfileSummary> profiles() const;
    [[nodiscard]] bool hasProfile(std::string const& profileId) const;
    [[nodiscard]] std::string profileName(std::string const& profileId) const;
    [[nodiscard]] bool profileUsesLegacy21(std::string const& profileId) const;
    [[nodiscard]] std::optional<std::string> bindingFor(std::string const& levelKey) const;

    void registerProfile(
        std::string const& profileId,
        std::string const& name,
        int startPosCount,
        bool legacy21
    );
    bool bind(std::string const& levelKey, std::string const& profileId);
    void unbind(std::string const& levelKey);
    bool removeProfile(std::string const& profileId);

private:
    ProfileRegistry() = default;

    [[nodiscard]] std::vector<std::string> profileIds() const;
    void saveProfileIds(std::vector<std::string> const& ids) const;
    void flush() const;
};

} // namespace baconsistent
