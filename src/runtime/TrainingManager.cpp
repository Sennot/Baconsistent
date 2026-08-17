#include "TrainingManager.hpp"
#include "../core/PercentageMath.hpp"
#include "ProfileRegistry.hpp"

#include <Geode/loader/Mod.hpp>

#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <set>
#include <utility>

using namespace geode::prelude;

namespace baconsistent {

namespace {
std::uint64_t fnv1a(std::string_view text, std::uint64_t seed = 14695981039346656037ull) {
    auto hash = seed;
    for (auto const ch : text) {
        hash ^= static_cast<unsigned char>(ch);
        hash *= 1099511628211ull;
    }
    return hash;
}

int configuredTarget() {
    return static_cast<int>(Mod::get()->getSettingValue<int64_t>("repetitions"));
}

std::string encodeDoubles(std::vector<double> const& values) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(6);
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) {
            out << ',';
        }
        out << values[i];
    }
    return out.str();
}

std::vector<double> decodeDoubles(std::string const& encoded) {
    std::vector<double> values;
    if (encoded.empty()) {
        return values;
    }

    std::istringstream in(encoded);
    std::string token;
    while (std::getline(in, token, ',')) {
        try {
            auto value = std::stod(token);
            if (std::isfinite(value)) {
                values.push_back(value);
            }
        }
        catch (...) {
            return {};
        }
    }
    return values;
}

std::string formatPercent(double value) {
    auto const rounded = std::round(value);
    if (std::abs(value - rounded) < 0.05) {
        return fmt::format("{:.0f}", rounded);
    }
    return fmt::format("{:.1f}", value);
}

std::string legacyImportMarkerKey(std::string const& legacyKey) {
    return fmt::format("legacy-import-v1.{}", legacyKey);
}

std::string legacyMetadataKey(std::string const& legacyKey, std::string_view suffix) {
    return fmt::format("profiles.{}.{}", legacyKey, suffix);
}
} // namespace

TrainingManager& TrainingManager::get() {
    static TrainingManager instance;
    return instance;
}

TrainingManager::TrainingManager() : m_plan(10, 20), m_session(&m_plan) {}

bool TrainingManager::enabled() const {
    return m_loaded && m_profileActive && Mod::get()->getSettingValue<bool>("enabled");
}

PercentageMode TrainingManager::percentageMode() const {
    if (!m_profileActive || m_levelKey.empty()) {
        return PercentageMode::Modern22;
    }
    return ProfileRegistry::get().profileUsesLegacy21(m_levelKey)
        ? PercentageMode::Legacy21
        : PercentageMode::Modern22;
}

int TrainingManager::detectedStartPosCount() const {
    if (!m_profileActive || m_startPos21Boundaries.size() < 2) {
        return 0;
    }
    return std::max(0, static_cast<int>(m_startPos21Boundaries.size()) - 2);
}

int TrainingManager::scannableStartPosCount() const {
    return static_cast<int>(std::min(
        m_currentAnalysis.legacy21Percentages.size(),
        m_currentAnalysis.modern22Percentages.size()
    ));
}

std::string TrainingManager::currentProfileName() const {
    if (!m_profileActive || m_levelKey.empty()) {
        return {};
    }
    return ProfileRegistry::get().profileName(m_levelKey);
}

std::vector<ProfileSummary> TrainingManager::profiles() const {
    return ProfileRegistry::get().profiles();
}

std::vector<LegacySessionSummary> TrainingManager::legacySessions() const {
    std::set<std::string> legacyKeys;
    auto const& save = Mod::get()->getSaveContainer();

    for (auto const& [key, value] : save) {
        (void)value;
        auto parsed = core::parseLegacySavedKey(key);
        if (!parsed) {
            continue;
        }
        if (parsed->suffix == "startpos-2-1" || parsed->suffix == "startpos-2-2") {
            legacyKeys.insert(parsed->levelKey);
        }
    }

    std::vector<LegacySessionSummary> result;
    auto& registry = ProfileRegistry::get();
    for (auto const& legacyKey : legacyKeys) {
        auto const legacy21 = decodeDoubles(Mod::get()->getSavedValue<std::string>(
            profileSaveKey(legacyKey, "startpos-2-1")
        ));
        auto const modern22 = decodeDoubles(Mod::get()->getSavedValue<std::string>(
            profileSaveKey(legacyKey, "startpos-2-2")
        ));
        if (legacy21.size() < 3 || modern22.size() != legacy21.size()) {
            continue;
        }

        auto name = core::legacySessionFallbackName(legacyKey);
        auto const nameKey = legacyMetadataKey(legacyKey, "name");
        if (Mod::get()->hasSavedValue(nameKey)) {
            auto savedName = Mod::get()->getSavedValue<std::string>(nameKey);
            if (!savedName.empty()) {
                name = std::move(savedName);
            }
        }

        auto const markerKey = legacyImportMarkerKey(legacyKey);
        std::string importedProfileId;
        if (Mod::get()->hasSavedValue(markerKey)) {
            importedProfileId = Mod::get()->getSavedValue<std::string>(markerKey);
        }
        if (importedProfileId.empty() && registry.hasProfile(legacyKey)) {
            // Some v0.4 -> v0.5 upgrades already carried the legacy identity
            // into the profile index. Treat that as imported rather than
            // presenting a false "not recovered" state.
            importedProfileId = legacyKey;
        }

        auto round = 1;
        auto const roundKey = profileSaveKey(legacyKey, "round");
        if (Mod::get()->hasSavedValue(roundKey)) {
            round = std::max(1, Mod::get()->getSavedValue<int>(roundKey));
        }

        result.push_back({
            legacyKey,
            name,
            std::max(0, static_cast<int>(legacy21.size()) - 2),
            round,
            !importedProfileId.empty() && registry.hasProfile(importedProfileId),
            importedProfileId,
        });
    }

    std::sort(result.begin(), result.end(), [](auto const& a, auto const& b) {
        if (a.alreadyImported != b.alreadyImported) {
            return !a.alreadyImported;
        }
        return a.name < b.name;
    });
    return result;
}

std::optional<std::string> TrainingManager::importLegacySession(
    std::string const& legacyKey,
    std::string name,
    bool legacy21
) {
    if (!legacyKey.starts_with("online:") && !legacyKey.starts_with("local:")) {
        return std::nullopt;
    }

    auto oldLegacy = decodeDoubles(Mod::get()->getSavedValue<std::string>(
        profileSaveKey(legacyKey, "startpos-2-1")
    ));
    auto oldModern = decodeDoubles(Mod::get()->getSavedValue<std::string>(
        profileSaveKey(legacyKey, "startpos-2-2")
    ));
    if (oldLegacy.size() < 3 || oldModern.size() != oldLegacy.size()) {
        return std::nullopt;
    }

    // Legacy payloads store full boundaries including 0 and 100. Normalize the
    // physical marker pairs again so malformed/duplicate old values cannot
    // create a broken modern profile.
    oldLegacy.erase(oldLegacy.begin());
    oldLegacy.pop_back();
    oldModern.erase(oldModern.begin());
    oldModern.pop_back();
    auto normalized = core::normalizeDualPercentMarkers(std::move(oldLegacy), std::move(oldModern));
    if (normalized.legacy21.size() < 3 || normalized.modern22.size() != normalized.legacy21.size()) {
        return std::nullopt;
    }

    if (name.empty()) {
        name = core::legacySessionFallbackName(legacyKey);
        auto const oldNameKey = legacyMetadataKey(legacyKey, "name");
        if (Mod::get()->hasSavedValue(oldNameKey)) {
            auto oldName = Mod::get()->getSavedValue<std::string>(oldNameKey);
            if (!oldName.empty()) {
                name = std::move(oldName);
            }
        }
    }

    auto const profileId = makeProfileId(name);
    if (profileId.empty()) {
        return std::nullopt;
    }

    auto& save = Mod::get()->getSaveContainer();
    Mod::get()->setSavedValue(profileSaveKey(profileId, "startpos-2-1"), encodeDoubles(normalized.legacy21));
    Mod::get()->setSavedValue(profileSaveKey(profileId, "startpos-2-2"), encodeDoubles(normalized.modern22));

    // Copy the compatible pre-profile training payload verbatim. Counts,
    // targets, stats and round encodings were intentionally kept stable across
    // the profile rewrite. The signature is regenerated from normalized 2.1
    // boundaries so restored repetition counters are accepted on first load.
    for (auto const suffix : {"counts", "targets", "stats-v1", "selected", "round"}) {
        auto const oldKey = profileSaveKey(legacyKey, suffix);
        if (auto oldValue = save.get(oldKey); oldValue.isOk()) {
            save[profileSaveKey(profileId, suffix)] = oldValue.unwrap();
        }
    }
    Mod::get()->setSavedValue(
        profileSaveKey(profileId, "plan-signature"),
        "startpos:" + encodeDoubles(normalized.legacy21)
    );

    ProfileRegistry::get().registerProfile(
        profileId,
        name,
        std::max(0, static_cast<int>(normalized.legacy21.size()) - 2),
        legacy21
    );

    // Keep legacy data untouched. This marker only lets the UI show that a
    // recovery already exists; deleting the imported profile makes the legacy
    // session importable again.
    Mod::get()->setSavedValue(legacyImportMarkerKey(legacyKey), profileId);
    Mod::get()->setSavedValue(legacyMetadataKey(profileId, "legacy-source"), legacyKey);
    if (auto saveResult = Mod::get()->saveData(); saveResult.isErr()) {
        log::warn("Baconsistent: legacy import save failed: {}", saveResult.unwrapErr());
    }

    return profileId;
}


bool TrainingManager::commitEditedBoundaries(std::vector<double> legacy21, std::vector<double> modern22) {
    if (!m_profileActive || m_levelKey.empty() || legacy21.size() < 3 || legacy21.size() != modern22.size()) {
        return false;
    }

    constexpr double epsilon = 0.01;
    if (std::abs(legacy21.front()) > epsilon || std::abs(modern22.front()) > epsilon ||
        std::abs(legacy21.back() - 100.0) > epsilon || std::abs(modern22.back() - 100.0) > epsilon) {
        return false;
    }
    for (std::size_t i = 1; i < legacy21.size(); ++i) {
        if (!std::isfinite(legacy21[i]) || !std::isfinite(modern22[i]) ||
            legacy21[i] <= legacy21[i - 1] + epsilon || modern22[i] <= modern22[i - 1] + epsilon) {
            return false;
        }
    }

    // Keep stage identity by index. The TrainingPlan reconfigure call preserves
    // counts and per-stage targets when the number of boundaries is unchanged.
    if (legacy21.size() != m_startPos21Boundaries.size()) {
        return false;
    }

    // Opening/saving the editor is not a gameplay attempt. Remove the paused
    // attempt from statistics before changing its A->B layout. PlayLayerHook
    // starts a fresh attempt when gameplay resumes.
    m_stats.cancelAttempt();
    m_attemptProgressCounted = false;

    m_startPos21Boundaries = std::move(legacy21);
    m_startPos22Boundaries = std::move(modern22);
    Mod::get()->setSavedValue(saveKey("startpos-2-1"), encodeDoubles(m_startPos21Boundaries));
    Mod::get()->setSavedValue(saveKey("startpos-2-2"), encodeDoubles(m_startPos22Boundaries));

    m_plan.reconfigureBoundaries(activeBoundaries(), configuredTarget());
    m_session.attach(&m_plan);
    m_stats.configureStages(m_plan.size());
    m_activePlanSignature = configuredPlanSignature();
    Mod::get()->setSavedValue(saveKey("plan-signature"), m_activePlanSignature);
    ++m_layoutRevision;
    flushCrashSafe();
    return true;
}

std::string TrainingManager::sourceLabel() const {
    if (!m_profileActive) {
        return "No profile";
    }
    return percentageMode() == PercentageMode::Legacy21 ? "Profile / 2.1%" : "Profile / 2.2%";
}

void TrainingManager::clearProfileState(bool keepRuntimeMarkers) {
    m_levelKey.clear();
    m_activePlanSignature.clear();
    m_startPos21Boundaries.clear();
    m_startPos22Boundaries.clear();
    if (!keepRuntimeMarkers) {
        m_runtimeStartPositions.clear();
    }
    m_hasStartPosProfile = false;
    m_usingStartPosBoundaries = false;
    m_profileActive = false;
    m_recentCompletedRound = 0;
    m_attemptProgressCounted = false;
    m_attemptGuard = {};
    m_stats = {};
    m_roundNumber = 1;
}

void TrainingManager::loadLevel(GJGameLevel* level, StartPosAnalysis const& analysis) {
    if (m_loaded && m_profileActive) {
        finishAttempt();
        saveProgress();
        saveStats();
        saveRound();
        flushCrashSafe();
    }

    m_currentLevelKey = makePhysicalLevelKey(level);
    m_currentLevelName = levelDisplayName(level);
    m_currentAnalysis = analysis;
    m_loaded = !m_currentLevelKey.empty();
    m_runtimeStartPositions = analysis.runtimeMarkers;
    clearProfileState(true);

    if (!m_loaded) {
        return;
    }

    // v0.5.0: StartPos scanning is discovery only. It NEVER creates a profile,
    // refreshes an existing profile, or binds the current level automatically.
    // A profile becomes active exclusively through an explicit saved binding.
    auto binding = ProfileRegistry::get().bindingFor(m_currentLevelKey);
    if (binding) {
        (void)activateProfile(*binding);
    }
}

void TrainingManager::loadActiveProfileState() {
    if (!m_profileActive || !m_hasStartPosProfile || m_levelKey.empty()) {
        return;
    }

    auto const savedTargets = Mod::get()->getSavedValue<std::string>(saveKey("targets"), "");
    auto const savedCounts = Mod::get()->getSavedValue<std::string>(saveKey("counts"), "");
    auto const savedSignature = Mod::get()->getSavedValue<std::string>(saveKey("plan-signature"), "");

    applyConfiguredPlan(true);
    m_plan.decodeTargets(savedTargets);
    if (savedSignature == m_activePlanSignature) {
        m_plan.decodeCounts(savedCounts);
    }

    m_stats.configureStages(m_plan.size());
    m_stats.decode(Mod::get()->getSavedValue<std::string>(saveKey("stats-v1"), ""));
    m_roundNumber = std::max(1, Mod::get()->getSavedValue<int>(saveKey("round"), 1));

    auto const savedSelected = Mod::get()->getSavedValue<int>(saveKey("selected"), -1);
    if (savedSelected >= 0 && savedSelected < static_cast<int>(m_plan.size())) {
        m_session.setSelected(savedSelected);
    }
    else {
        m_session.setSelected(m_plan.recommendedBackwards());
    }

    if (m_plan.allCompleted()) {
        advanceRound();
    }

    saveSelected();
    saveProgress();
    saveStats();
    saveRound();
    flushCrashSafe();
}

bool TrainingManager::activateProfile(std::string const& profileId) {
    if (!m_loaded || profileId.empty()) {
        return false;
    }

    auto runtime = m_runtimeStartPositions;
    clearProfileState(false);
    m_runtimeStartPositions = std::move(runtime);
    m_levelKey = profileId;
    loadCachedStartPosProfile();
    if (!m_hasStartPosProfile) {
        clearProfileState(true);
        return false;
    }

    m_profileActive = true;
    m_usingStartPosBoundaries = true;
    loadActiveProfileState();
    return true;
}

std::optional<std::string> TrainingManager::createProfileFromCurrentStartPositions(
    std::string name,
    bool legacy21
) {
    if (!m_loaded || !m_currentAnalysis.foundAny()) {
        return std::nullopt;
    }

    auto normalized = core::normalizeDualPercentMarkers(
        m_currentAnalysis.legacy21Percentages,
        m_currentAnalysis.modern22Percentages
    );
    if (normalized.legacy21.size() < 3 || normalized.modern22.size() != normalized.legacy21.size()) {
        return std::nullopt;
    }

    if (name.empty()) {
        name = m_currentLevelName.empty() ? "StartPos Profile" : m_currentLevelName;
    }

    auto profileId = makeProfileId(name);
    if (profileId.empty() || !writeStartPosProfile(profileId, m_currentAnalysis)) {
        return std::nullopt;
    }

    ProfileRegistry::get().registerProfile(
        profileId,
        name,
        static_cast<int>(normalized.legacy21.size()) - 2,
        legacy21
    );
    flushCrashSafe();
    return profileId;
}

bool TrainingManager::deleteProfile(std::string const& profileId) {
    if (profileId.empty() || !ProfileRegistry::get().hasProfile(profileId)) {
        return false;
    }

    if (m_profileActive && m_levelKey == profileId) {
        unbindCurrentLevel();
    }

    // A v0.4-era profile can itself use the legacy level identity (online:* /
    // local:*). Removing it from the modern registry must not destroy the old
    // pre-profile payload, because v0.5.3 deliberately treats that payload as
    // a recovery backup. New profile-* identities are deleted normally.
    auto const legacyIdentity = profileId.starts_with("online:") || profileId.starts_with("local:");
    if (!legacyIdentity) {
        clearProfilePayload(profileId);
    }
    auto const removed = ProfileRegistry::get().removeProfile(profileId);
    if (removed) {
        if (auto result = Mod::get()->saveData(); result.isErr()) {
            log::warn("Baconsistent: profile delete save failed: {}", result.unwrapErr());
        }
    }
    return removed;
}

bool TrainingManager::bindCurrentLevelToProfile(std::string const& profileId) {
    if (!m_loaded || m_currentLevelKey.empty() || !ProfileRegistry::get().hasProfile(profileId)) {
        return false;
    }

    if (m_profileActive) {
        finishAttempt();
    }

    if (!ProfileRegistry::get().bind(m_currentLevelKey, profileId)) {
        return false;
    }
    if (!activateProfile(profileId)) {
        ProfileRegistry::get().unbind(m_currentLevelKey);
        return false;
    }

    // Do not create a gameplay attempt from this UI method. Profile binding
    // normally happens while PauseLayer is open; PlayLayerHook notices the
    // profile transition on the first resumed frame, resets its noclip detector
    // and starts a clean attempt from the actual current position.
    return true;
}

void TrainingManager::unbindCurrentLevel() {
    if (!m_loaded || m_currentLevelKey.empty()) {
        return;
    }

    if (m_profileActive) {
        finishAttempt();
        saveProgress();
        saveStats();
        saveRound();
    }
    ProfileRegistry::get().unbind(m_currentLevelKey);
    clearProfileState(true);
}

void TrainingManager::unloadLevel() {
    if (m_loaded && m_profileActive) {
        finishAttempt();
        saveProgress();
        saveStats();
        saveRound();
        flushCrashSafe();
    }

    m_loaded = false;
    m_currentLevelKey.clear();
    m_currentLevelName.clear();
    m_currentAnalysis = {};
    clearProfileState(false);
}

void TrainingManager::refreshSettings() {
    if (!m_loaded || !m_profileActive) {
        return;
    }
    applyConfiguredPlan(false);
}

void TrainingManager::beginAttempt(double percent, bool practiceMode) {
    if (!enabled()) {
        return;
    }

    refreshSettings();
    maybeAutoMatchStart(percent);
    m_session.beginAttempt(percent);
    m_attemptProgressCounted = false;
    m_attemptGuard.begin(
        practiceMode,
        Mod::get()->getSettingValue<bool>("practice-mode-protection"),
        Mod::get()->getSettingValue<bool>("noclip-protection")
    );

    m_stats.configureStages(m_plan.size());
    if (m_attemptGuard.countable()) {
        m_stats.beginAttempt(static_cast<std::size_t>(m_session.selected()));
    }
    // Do not update the persisted save container yet: an unfinished attempt
    // should simply disappear if GD crashes. It becomes durable on success or
    // when the attempt ends normally.
}

void TrainingManager::observePracticeMode(bool practiceMode) {
    if (!m_profileActive || m_attemptProgressCounted) {
        return;
    }

    auto const wasCountable = m_attemptGuard.countable();
    m_attemptGuard.observePracticeMode(practiceMode);
    if (wasCountable && !m_attemptGuard.countable()) {
        m_stats.cancelAttempt();
        saveStats();
        flushCrashSafe();
    }
}

void TrainingManager::markSuppressedDeath() {
    if (!m_profileActive || m_attemptProgressCounted) {
        return;
    }

    auto const wasCountable = m_attemptGuard.countable();
    m_attemptGuard.observeSuppressedDeath();
    if (wasCountable && !m_attemptGuard.countable()) {
        // Death Tracker-style semantics: noclip may be enabled, but the run is
        // invalid only once GD actually attempted a lethal collision and a
        // different hook/mod kept the player alive. Invalid attempts are
        // removed from stats instead of being counted as failures.
        m_stats.cancelAttempt();
        saveStats();
        flushCrashSafe();
    }
}

void TrainingManager::tick(double dt) {
    if (!enabled() || !m_attemptGuard.countable()) {
        return;
    }
    m_stats.tick(dt);
}

void TrainingManager::finishAttempt() {
    if (!m_profileActive) {
        return;
    }

    if (m_attemptGuard.countable()) {
        m_stats.finishAttempt();
    }
    else {
        m_stats.cancelAttempt();
    }
    saveStats();
    // Geode saved values normally reach disk on game shutdown. Force an early
    // write at attempt boundaries so a GD crash cannot wipe the last training
    // progress/statistics batch.
    flushCrashSafe();
}

std::optional<ProgressEvent> TrainingManager::update(double currentPercent) {
    if (!enabled() || !m_attemptGuard.countable()) {
        return std::nullopt;
    }

    auto const indexBefore = m_session.selected();
    auto const target = m_plan.target(static_cast<std::size_t>(indexBefore));
    auto result = m_session.update(
        currentPercent,
        Mod::get()->getSettingValue<bool>("strict-start"),
        Mod::get()->getSettingValue<double>("start-tolerance")
    );

    if (!result) {
        return std::nullopt;
    }

    // The requested A -> B range is already clean at this exact moment. Any
    // later noclip collision or Practice toggle during the optional continued
    // long run must not revoke the fixed-stage completion.
    m_attemptProgressCounted = true;
    m_stats.markSuccess(static_cast<std::size_t>(indexBefore));

    ProgressEvent event;
    event.segmentIndex = indexBefore;
    event.repetitions = *result;
    event.target = target;
    event.segmentCompleted = m_plan.completed(static_cast<std::size_t>(indexBefore));
    event.planCompleted = m_plan.allCompleted();
    event.newRound = m_roundNumber;

    if (event.planCompleted) {
        event.roundCompleted = true;
        event.completedRound = m_roundNumber;
        advanceRound();
        event.newRound = m_roundNumber;
    }
    else if (event.segmentCompleted && Mod::get()->getSettingValue<bool>("auto-previous")) {
        m_session.setSelected(m_plan.previousIncomplete(indexBefore));
        saveSelected();
    }

    saveProgress();
    saveStats();
    saveRound();
    // Progress is the most valuable data in the mod. Flush it immediately
    // after every successful fixed-part completion instead of waiting for GD
    // to exit cleanly.
    flushCrashSafe();
    return event;
}

core::Segment TrainingManager::selectedSegment() const {
    return m_plan.segment(static_cast<std::size_t>(m_session.selected()));
}

int TrainingManager::liveSegmentIndex() const {
    if (!m_profileActive || m_plan.size() == 0) {
        return 0;
    }

    auto* layer = PlayLayer::get();
    if (!layer || !m_usingStartPosBoundaries || m_runtimeStartPositions.empty()) {
        return std::clamp(m_session.selected(), 0, static_cast<int>(m_plan.size()) - 1);
    }

    auto* active = layer->m_startPosObject;
    if (!active) {
        // No StartPos selected means a run from zero: that is stage 0.
        return 0;
    }

    auto marker = std::find_if(
        m_runtimeStartPositions.begin(),
        m_runtimeStartPositions.end(),
        [active](StartPosRuntimeMarker const& value) { return value.object == active; }
    );

    if (marker == m_runtimeStartPositions.end()) {
        return std::clamp(m_session.selected(), 0, static_cast<int>(m_plan.size()) - 1);
    }

    auto const markerPercent = percentageMode() == PercentageMode::Legacy21
        ? marker->legacy21
        : marker->modern22;

    auto bestIndex = 0;
    auto bestDistance = std::numeric_limits<double>::infinity();
    for (int i = 0; i < static_cast<int>(m_plan.size()); ++i) {
        auto const distance = std::abs(m_plan.segment(static_cast<std::size_t>(i)).start - markerPercent);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = i;
        }
    }

    return bestIndex;
}

void TrainingManager::select(int index) {
    if (!m_profileActive) {
        return;
    }
    m_session.setSelected(index);
    flushCrashSafe();
}

void TrainingManager::selectRelative(int delta) {
    if (!m_profileActive || m_plan.size() == 0) {
        return;
    }

    auto index = m_session.selected() + delta;
    auto const size = static_cast<int>(m_plan.size());
    while (index < 0) index += size;
    while (index >= size) index -= size;
    select(index);
}

void TrainingManager::selectRecommended() {
    if (!m_profileActive) {
        return;
    }
    m_session.setSelected(m_plan.recommendedBackwards());
    saveSelected();
}

void TrainingManager::resetSelected() {
    if (!m_profileActive) {
        return;
    }
    m_plan.reset(static_cast<std::size_t>(m_session.selected()));
    saveProgress();
    flushCrashSafe();
}

void TrainingManager::resetRoundProgress() {
    if (!m_profileActive) {
        return;
    }
    m_plan.resetAll();
    m_stats.resetCurrentRound();
    m_recentCompletedRound = 0;
    m_session.setSelected(m_plan.recommendedBackwards());
    saveSelected();
    saveProgress();
    saveStats();
    flushCrashSafe();
}

void TrainingManager::adjustSelectedTarget(int delta) {
    if (!m_profileActive || m_plan.size() == 0 || delta == 0) {
        return;
    }
    auto const index = static_cast<std::size_t>(m_session.selected());
    m_plan.setTarget(index, m_plan.target(index) + delta);
    if (m_plan.allCompleted()) {
        advanceRound();
    }
    else {
        saveProgress();
        flushCrashSafe();
    }
}

void TrainingManager::resetSelectedTarget() {
    if (!m_profileActive || m_plan.size() == 0) {
        return;
    }
    auto const index = static_cast<std::size_t>(m_session.selected());
    m_plan.setTarget(index, m_plan.target());
    if (m_plan.allCompleted()) {
        advanceRound();
    }
    else {
        saveProgress();
        flushCrashSafe();
    }
}

int TrainingManager::selectedTarget() const {
    if (!m_profileActive || m_plan.size() == 0) {
        return m_plan.target();
    }
    return m_plan.target(static_cast<std::size_t>(m_session.selected()));
}

bool TrainingManager::manualAdjustIndex(int index, int delta, bool completeStage) {
    if (!m_loaded || !m_profileActive || m_plan.size() == 0) {
        return false;
    }

    index = std::clamp(index, 0, static_cast<int>(m_plan.size()) - 1);
    auto const stage = static_cast<std::size_t>(index);
    auto const wasComplete = m_plan.completed(stage);

    bool changed = false;
    if (completeStage) {
        changed = m_plan.complete(stage);
    }
    else if (delta > 0) {
        for (int i = 0; i < delta; ++i) {
            changed = m_plan.increment(stage) || changed;
        }
    }
    else if (delta < 0) {
        for (int i = 0; i < -delta; ++i) {
            changed = m_plan.decrement(stage) || changed;
        }
    }

    if (!changed) {
        return false;
    }

    // Manual corrections are progress-only. Do not touch TrainingStats.
    m_session.setSelected(index);

    if (m_plan.allCompleted()) {
        advanceRound();
        return true;
    }

    if (!wasComplete && m_plan.completed(stage) && Mod::get()->getSettingValue<bool>("auto-previous")) {
        m_session.setSelected(m_plan.previousIncomplete(index));
    }

    // flushCrashSafe serializes counts, targets, selection, stats and round
    // before forcing Geode savedata to disk.
    flushCrashSafe();
    return true;
}

bool TrainingManager::manualAdjustSelected(int delta) {
    return manualAdjustIndex(m_session.selected(), delta, false);
}

bool TrainingManager::manualCompleteSelected() {
    return manualAdjustIndex(m_session.selected(), 0, true);
}

bool TrainingManager::manualAdjustLive(int delta) {
    return manualAdjustIndex(liveSegmentIndex(), delta, false);
}

bool TrainingManager::manualCompleteLive() {
    return manualAdjustIndex(liveSegmentIndex(), 0, true);
}


std::string TrainingManager::segmentRangeText(int index) const {
    if (m_plan.size() == 0) {
        return "-";
    }
    index = std::clamp(index, 0, static_cast<int>(m_plan.size()) - 1);
    auto const seg = m_plan.segment(static_cast<std::size_t>(index));
    return fmt::format("{}-{}%", formatPercent(seg.start), formatPercent(seg.end));
}

std::string TrainingManager::compactSegmentText(int index) const {
    if (m_plan.size() == 0) {
        return "-";
    }
    index = std::clamp(index, 0, static_cast<int>(m_plan.size()) - 1);
    return fmt::format(
        "{}  {}/{}",
        segmentRangeText(index),
        m_plan.count(static_cast<std::size_t>(index)),
        m_plan.target(static_cast<std::size_t>(index))
    );
}

std::string TrainingManager::makePhysicalLevelKey(GJGameLevel* level) const {
    if (!level) {
        return {};
    }

    // Physical identity intentionally ignores m_originalLevel. A StartPos copy
    // and the original are separate levels that may both bind to one profile.
    auto const id = level->m_levelID.value();
    if (id > 0) {
        return fmt::format("online:{}", id);
    }

    std::string const name = level->m_levelName.c_str();
    std::string const data = level->m_levelString.c_str();
    auto hash = fnv1a(name);
    hash = fnv1a(data, hash);
    return fmt::format("local:{:016x}", hash);
}


std::string TrainingManager::levelDisplayName(GJGameLevel* level) const {
    if (!level) {
        return "StartPos Profile";
    }
    std::string name = level->m_levelName.c_str();
    return name.empty() ? "StartPos Profile" : name;
}

std::string TrainingManager::makeProfileId(std::string_view name) const {
    auto const now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    auto hash = fnv1a(m_currentLevelKey);
    hash = fnv1a(name, hash);
    hash = fnv1a(fmt::format("{}", now), hash);
    auto id = fmt::format("profile-{:016x}", hash);
    int collision = 0;
    while (ProfileRegistry::get().hasProfile(id)) {
        id = fmt::format("profile-{:016x}-{}", hash, ++collision);
    }
    return id;
}

std::string TrainingManager::profileSaveKey(std::string const& profileId, std::string_view suffix) {
    return fmt::format("levels.{}.{}", profileId, suffix);
}

std::string TrainingManager::saveKey(std::string_view suffix) const {
    return profileSaveKey(m_levelKey, suffix);
}

void TrainingManager::saveProgress() {
    if (!m_profileActive) {
        return;
    }
    Mod::get()->setSavedValue(saveKey("counts"), m_plan.encodeCounts());
    Mod::get()->setSavedValue(saveKey("targets"), m_plan.encodeTargets());
    Mod::get()->setSavedValue(saveKey("plan-signature"), m_activePlanSignature);
}

void TrainingManager::saveSelected() {
    if (!m_profileActive) {
        return;
    }
    Mod::get()->setSavedValue(saveKey("selected"), m_session.selected());
}

void TrainingManager::saveStats() {
    if (!m_profileActive) {
        return;
    }
    Mod::get()->setSavedValue(saveKey("stats-v1"), m_stats.encode());
}

void TrainingManager::saveRound() {
    if (!m_profileActive) {
        return;
    }
    Mod::get()->setSavedValue(saveKey("round"), m_roundNumber);
}

void TrainingManager::flushCrashSafe() {
    if (!m_profileActive) {
        return;
    }

    // Write-through durability: every explicit flush serializes the COMPLETE
    // active profile state before asking Geode to commit savedata to disk.
    // This avoids relying on every caller remembering which individual field
    // changed (counts, targets, stats, round or selected stage). A successful
    // A->B completion therefore becomes durable immediately, not only when GD
    // closes normally.
    saveProgress();
    saveSelected();
    saveStats();
    saveRound();

    if (auto result = Mod::get()->saveData(); result.isErr()) {
        log::warn("Baconsistent: early profile save failed: {}", result.unwrapErr());
    }
}

void TrainingManager::maybeAutoMatchStart(double percent) {
    if (!Mod::get()->getSettingValue<bool>("auto-match-start") || percent <= 0.5 || m_plan.size() == 0) {
        return;
    }

    auto const tolerance = Mod::get()->getSettingValue<double>("start-tolerance");
    auto bestDistance = std::numeric_limits<double>::infinity();
    auto candidate = -1;

    for (int i = 0; i < static_cast<int>(m_plan.size()); ++i) {
        auto const start = m_plan.segment(static_cast<std::size_t>(i)).start;
        auto const distance = std::abs(percent - start);
        if (distance < bestDistance) {
            bestDistance = distance;
            candidate = i;
        }
    }

    if (candidate >= 0 && bestDistance <= tolerance) {
        m_session.setSelected(candidate);
        flushCrashSafe();
    }
}

void TrainingManager::advanceRound() {
    auto const completedRound = m_roundNumber;
    m_stats.completeRound(completedRound);
    m_recentCompletedRound = completedRound;
    ++m_roundNumber;

    m_plan.resetAll();
    m_session.setSelected(m_plan.recommendedBackwards());
    saveSelected();
    saveProgress();
    saveStats();
    saveRound();
    flushCrashSafe();
}

void TrainingManager::loadCachedStartPosProfile() {
    auto legacy = decodeDoubles(Mod::get()->getSavedValue<std::string>(saveKey("startpos-2-1"), ""));
    auto modern = decodeDoubles(Mod::get()->getSavedValue<std::string>(saveKey("startpos-2-2"), ""));

    if (legacy.size() >= 3 && modern.size() == legacy.size()) {
        legacy.erase(legacy.begin());
        legacy.pop_back();
        modern.erase(modern.begin());
        modern.pop_back();
        auto normalized = core::normalizeDualPercentMarkers(std::move(legacy), std::move(modern));
        if (normalized.legacy21.size() >= 3 && normalized.modern22.size() == normalized.legacy21.size()) {
            m_startPos21Boundaries = std::move(normalized.legacy21);
            m_startPos22Boundaries = std::move(normalized.modern22);
            m_hasStartPosProfile = true;
        }
    }
}

bool TrainingManager::writeStartPosProfile(std::string const& profileId, StartPosAnalysis const& analysis) {
    if (profileId.empty() || !analysis.foundAny()) {
        return false;
    }

    auto normalized = core::normalizeDualPercentMarkers(
        analysis.legacy21Percentages,
        analysis.modern22Percentages
    );
    if (normalized.legacy21.size() < 3 || normalized.modern22.size() != normalized.legacy21.size()) {
        return false;
    }

    Mod::get()->setSavedValue(profileSaveKey(profileId, "startpos-2-1"), encodeDoubles(normalized.legacy21));
    Mod::get()->setSavedValue(profileSaveKey(profileId, "startpos-2-2"), encodeDoubles(normalized.modern22));
    return true;
}

void TrainingManager::clearProfilePayload(std::string const& profileId) {
    if (profileId.empty()) {
        return;
    }
    Mod::get()->setSavedValue(profileSaveKey(profileId, "startpos-2-1"), std::string{});
    Mod::get()->setSavedValue(profileSaveKey(profileId, "startpos-2-2"), std::string{});
    Mod::get()->setSavedValue(profileSaveKey(profileId, "counts"), std::string{});
    Mod::get()->setSavedValue(profileSaveKey(profileId, "targets"), std::string{});
    Mod::get()->setSavedValue(profileSaveKey(profileId, "stats-v1"), std::string{});
    Mod::get()->setSavedValue(profileSaveKey(profileId, "plan-signature"), std::string{});
    Mod::get()->setSavedValue(profileSaveKey(profileId, "selected"), -1);
    Mod::get()->setSavedValue(profileSaveKey(profileId, "round"), 1);
}

std::vector<double> TrainingManager::activeBoundaries() const {
    if (!m_profileActive || !m_hasStartPosProfile) {
        return {};
    }
    return percentageMode() == PercentageMode::Legacy21
        ? m_startPos21Boundaries
        : m_startPos22Boundaries;
}

std::string TrainingManager::configuredPlanSignature() const {
    if (!m_profileActive || !m_hasStartPosProfile) {
        return {};
    }
    // Use physical 2.1 marker order as the stable identity so switching the
    // display/runtime percentage mode never detaches counters from a stage.
    return "startpos:" + encodeDoubles(m_startPos21Boundaries);
}

void TrainingManager::applyConfiguredPlan(bool initialLoad) {
    if (!m_profileActive || !m_hasStartPosProfile) {
        return;
    }

    auto const target = configuredTarget();
    auto const boundaries = activeBoundaries();
    auto const signature = configuredPlanSignature();
    if (boundaries.size() < 3 || signature.empty()) {
        return;
    }

    auto const savedSignature = Mod::get()->getSavedValue<std::string>(saveKey("plan-signature"), "");
    auto const layoutChanged = initialLoad
        ? (!savedSignature.empty() && savedSignature != signature)
        : (!m_activePlanSignature.empty() && m_activePlanSignature != signature);

    m_plan.reconfigureBoundaries(boundaries, target);
    m_session.attach(&m_plan);
    m_usingStartPosBoundaries = true;

    if (initialLoad) {
        m_plan.resetAll();
        if (!layoutChanged && savedSignature == signature) {
            m_plan.decodeCounts(Mod::get()->getSavedValue<std::string>(saveKey("counts"), ""));
        }
    }
    else if (layoutChanged) {
        m_plan.resetAll();
        m_plan.resetTargetsToDefault();
        m_session.setSelected(m_plan.recommendedBackwards());
        m_stats.clearStageStats();
        saveSelected();
    }

    m_stats.configureStages(m_plan.size());
    m_activePlanSignature = signature;
    saveProgress();
}


} // namespace baconsistent
