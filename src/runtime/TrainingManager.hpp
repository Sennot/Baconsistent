#pragma once

#include "../core/AttemptGuard.hpp"
#include "../core/LegacySession.hpp"
#include "../core/TrainingPlan.hpp"
#include "../core/TrainingStats.hpp"
#include "ProfileRegistry.hpp"
#include "StartPosAnalyzer.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace baconsistent {


struct LegacySessionSummary {
    std::string legacyKey;
    std::string name;
    int startPosCount = 0;
    int round = 1;
    bool alreadyImported = false;
    std::string importedProfileId;
};

struct ProgressEvent {
    int segmentIndex = 0;
    int repetitions = 0;
    int target = 20;
    bool segmentCompleted = false;
    bool planCompleted = false;
    bool roundCompleted = false;
    int completedRound = 0;
    int newRound = 1;
};

class TrainingManager {
public:
    static TrainingManager& get();

    void loadLevel(GJGameLevel* level, StartPosAnalysis const& analysis = {});
    void unloadLevel();
    void refreshSettings();

    [[nodiscard]] bool loaded() const { return m_loaded; }
    [[nodiscard]] bool hasActiveProfile() const { return m_profileActive; }
    [[nodiscard]] bool enabled() const;
    [[nodiscard]] std::string const& levelKey() const { return m_levelKey; }
    [[nodiscard]] std::string const& currentLevelKey() const { return m_currentLevelKey; }
    [[nodiscard]] std::string const& currentLevelName() const { return m_currentLevelName; }
    [[nodiscard]] std::string currentProfileName() const;
    [[nodiscard]] std::vector<ProfileSummary> profiles() const;
    [[nodiscard]] std::vector<LegacySessionSummary> legacySessions() const;
    [[nodiscard]] std::optional<std::string> importLegacySession(
        std::string const& legacyKey,
        std::string name,
        bool legacy21
    );

    // Profile workflow intentionally mirrors Blitzkrieg-style explicit profile
    // management: scanning a StartPos copy never creates or binds a profile by
    // itself. The user creates one from the current scan, then explicitly binds
    // any level(s) they want to that profile.
    [[nodiscard]] bool hasScannableStartPositions() const { return m_currentAnalysis.foundAny(); }
    [[nodiscard]] int scannableStartPosCount() const;
    [[nodiscard]] std::optional<std::string> createProfileFromCurrentStartPositions(
        std::string name,
        bool legacy21
    );
    bool bindCurrentLevelToProfile(std::string const& profileId);
    void unbindCurrentLevel();
    bool deleteProfile(std::string const& profileId);

    [[nodiscard]] PercentageMode percentageMode() const;
    [[nodiscard]] bool usingStartPosBoundaries() const { return m_usingStartPosBoundaries; }
    [[nodiscard]] int detectedStartPosCount() const;
    [[nodiscard]] std::string sourceLabel() const;
    [[nodiscard]] bool hasLiveStartPositions() const { return !m_runtimeStartPositions.empty(); }
    [[nodiscard]] std::vector<double> const& profileBoundaries21() const { return m_startPos21Boundaries; }
    [[nodiscard]] std::vector<double> const& profileBoundaries22() const { return m_startPos22Boundaries; }
    [[nodiscard]] std::uint64_t layoutRevision() const { return m_layoutRevision; }
    bool commitEditedBoundaries(std::vector<double> legacy21, std::vector<double> modern22);

    void beginAttempt(double percent, bool practiceMode);
    void observePracticeMode(bool practiceMode);
    void markSuppressedDeath();
    void tick(double dt);
    void finishAttempt();
    std::optional<ProgressEvent> update(double currentPercent);

    [[nodiscard]] core::TrainingPlan const& plan() const { return m_plan; }
    [[nodiscard]] core::TrainingPlan& plan() { return m_plan; }
    [[nodiscard]] core::TrainingStats const& stats() const { return m_stats; }
    [[nodiscard]] int selected() const { return m_session.selected(); }
    [[nodiscard]] int liveSegmentIndex() const;
    [[nodiscard]] core::Segment selectedSegment() const;
    [[nodiscard]] bool attemptCountable() const { return m_attemptGuard.countable(); }
    [[nodiscard]] bool attemptBlockedByPractice() const { return m_attemptGuard.blockedByPractice(); }
    [[nodiscard]] bool attemptBlockedByNoclip() const { return m_attemptGuard.blockedByNoclip(); }

    [[nodiscard]] int roundNumber() const { return m_roundNumber; }
    [[nodiscard]] int completedRounds() const { return std::max(0, m_roundNumber - 1); }
    [[nodiscard]] int recentCompletedRound() const { return m_recentCompletedRound; }
    void clearRecentRoundBanner() { m_recentCompletedRound = 0; }

    void select(int index);
    void selectRelative(int delta);
    void selectRecommended();
    void resetSelected();
    void resetRoundProgress();
    void adjustSelectedTarget(int delta);
    void resetSelectedTarget();
    [[nodiscard]] int selectedTarget() const;

    // Manual correction controls do not alter attempts, success rate, streaks
    // or playtime. They only change the repetition counter and save it.
    bool manualAdjustSelected(int delta);
    bool manualCompleteSelected();
    bool manualAdjustLive(int delta);
    bool manualCompleteLive();

    [[nodiscard]] std::string compactSegmentText(int index) const;
    [[nodiscard]] std::string segmentRangeText(int index) const;

private:
    TrainingManager();

    [[nodiscard]] std::string makePhysicalLevelKey(GJGameLevel* level) const;
    [[nodiscard]] std::string levelDisplayName(GJGameLevel* level) const;
    [[nodiscard]] std::string makeProfileId(std::string_view name) const;
    std::string saveKey(std::string_view suffix) const;
    static std::string profileSaveKey(std::string const& profileId, std::string_view suffix);

    void clearProfileState(bool keepRuntimeMarkers);
    bool activateProfile(std::string const& profileId);
    void loadActiveProfileState();
    void saveProgress();
    void saveSelected();
    void saveStats();
    void saveRound();
    void flushCrashSafe();
    void maybeAutoMatchStart(double percent);
    void advanceRound();
    bool manualAdjustIndex(int index, int delta, bool completeStage);

    void loadCachedStartPosProfile();
    bool writeStartPosProfile(std::string const& profileId, StartPosAnalysis const& analysis);
    void clearProfilePayload(std::string const& profileId);
    void applyConfiguredPlan(bool initialLoad);

    [[nodiscard]] std::vector<double> activeBoundaries() const;
    [[nodiscard]] std::string configuredPlanSignature() const;

    core::TrainingPlan m_plan;
    core::TrainingSession m_session;
    core::TrainingStats m_stats;
    core::AttemptGuard m_attemptGuard;
    std::string m_currentLevelKey;
    std::string m_levelKey; // active profile ID / persistence key
    std::string m_currentLevelName;
    std::string m_activePlanSignature;
    StartPosAnalysis m_currentAnalysis;
    std::vector<double> m_startPos21Boundaries;
    std::vector<double> m_startPos22Boundaries;
    std::vector<StartPosRuntimeMarker> m_runtimeStartPositions;
    bool m_hasStartPosProfile = false;
    bool m_usingStartPosBoundaries = false;
    bool m_loaded = false;
    bool m_profileActive = false;
    bool m_attemptProgressCounted = false;
    int m_roundNumber = 1;
    int m_recentCompletedRound = 0;
    std::uint64_t m_layoutRevision = 0;
};

} // namespace baconsistent
