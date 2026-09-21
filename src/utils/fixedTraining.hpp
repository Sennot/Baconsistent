#pragma once

#include "../serialization/profile/Profile.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace bacon
{
inline constexpr int defaultPasses = 20;
inline constexpr int maxPasses = 9999;

inline std::vector<Stage> makeFixedStages(std::vector<float> points)
{
    points.erase(std::remove_if(points.begin(), points.end(), [](float point) {
        return !std::isfinite(point) || point <= 0.f || point >= 100.f;
    }), points.end());
    points.push_back(0.f);
    points.push_back(100.f);
    std::sort(points.begin(), points.end());
    points.erase(std::unique(points.begin(), points.end(), [](float a, float b) {
        return std::abs(a - b) < 0.001f;
    }), points.end());

    // One permanent stage: adjacent parts never grow after a pass.
    Stage stage;
    stage.id = "fixed-parts";
    stage.stage = 1;
    for (size_t i = 1; i < points.size(); ++i)
    {
        Range range;
        range.from = points[i - 1];
        range.to = points[i];
        range.id = "part-" + std::to_string(range.from) + "-" + std::to_string(range.to);
        stage.ranges.push_back(range);
    }
    return {stage};
}

inline void refreshCompletion(Profile& profile)
{
    profile.requiredPasses = std::clamp(profile.requiredPasses, 1, maxPasses);
    for (auto& stage : profile.data.stages)
    {
        bool hasConsidered = false;
        bool allChecked = true;
        for (auto& range : stage.ranges)
        {
            range.completionCounter = std::max(0, range.completionCounter);
            range.checked = range.completionCounter >= profile.requiredPasses;
            if (!range.checked)
                range.completedAt = 0;
            if (range.consider)
            {
                hasConsidered = true;
                allChecked = allChecked && range.checked;
            }
        }
        stage.checked = hasConsidered && allChecked;
    }
}

// Old Blitzkrieg exports contain progressively longer ranges. Keep only
// the original adjacent parts and their actual repeated-pass counters.
inline void normalizeTraining(Profile& profile)
{
    if (profile.data.fixedPartsVersion != 1)
    {
        std::vector<float> points;
        if (!profile.data.stages.empty())
            for (auto const& range : profile.data.stages.front().ranges)
                if (range.consider)
                {
                    points.push_back(range.from);
                    points.push_back(range.to);
                }
        if (points.empty())
            points = profile.data.tags;
        auto stages = makeFixedStages(points);
        for (auto& range : stages.front().ranges)
            for (auto const& oldStage : profile.data.stages)
                for (auto const& oldRange : oldStage.ranges)
                    if (std::abs(range.from - oldRange.from) < 0.001f &&
                        std::abs(range.to - oldRange.to) < 0.001f)
                    {
                        range = oldRange;
                        range.consider = true;
                        range.automaticallyClosed = false;
                    }
        profile.data.stages = std::move(stages);
        profile.data.fixedPartsVersion = 1;
    }
    refreshCompletion(profile);
}

inline bool recordPass(Range& range, int target, float from, float to)
{
    const bool wasChecked = range.checked;
    if (range.firstRunTo <= 0.f)
    {
        range.firstRunFrom = from;
        range.firstRunTo = to;
        range.attemptsToComplete = range.attempts;
    }
    if (range.completionCounter < std::numeric_limits<int>::max())
        ++range.completionCounter;
    if (range.recordedPasses < std::numeric_limits<int>::max())
        ++range.recordedPasses;
    range.checked = range.completionCounter >= std::clamp(target, 1, maxPasses);
    range.automaticallyClosed = false;
    if (!wasChecked && range.checked)
        range.completedAt = std::time(nullptr);
    return !wasChecked && range.checked;
}

inline void adjustPasses(Profile& profile, Range& range, int delta)
{
    auto count = static_cast<long long>(range.completionCounter) + delta;
    range.completionCounter = static_cast<int>(std::clamp(
        count, 0LL, static_cast<long long>(std::numeric_limits<int>::max())));
    range.automaticallyClosed = false;
    if (range.completionCounter < profile.requiredPasses)
        range.completedAt = 0;
    // Manual corrections do not fabricate attempts, first clears or best runs.
    refreshCompletion(profile);
}
}
