#include "../src/utils/fixedTraining.hpp"
#include <cassert>
#include <iostream>

int main()
{
    auto stages = bacon::makeFixedStages({50, 10, 10, 90, -5, 101,
        std::numeric_limits<float>::quiet_NaN()});
    assert(stages.size() == 1 && stages[0].ranges.size() == 4);
    auto& ranges = stages[0].ranges;
    assert(ranges[0].from == 0 && ranges[0].to == 10);
    assert(ranges[3].from == 90 && ranges[3].to == 100);
    assert(bacon::makeFixedStages({})[0].ranges.size() == 1);

    Profile profile;
    profile.data.stages = stages;
    auto& part = profile.data.stages[0].ranges[2];
    for (int i = 0; i < 19; ++i)
    {
        ++part.attempts;
        assert(!bacon::recordPass(part, 20, 50, 97));
        assert(!part.checked);
    }
    ++part.attempts;
    assert(bacon::recordPass(part, 20, 50, 100));
    assert(part.checked && part.completionCounter == 20);
    assert(part.firstRunTo == 97 && part.attemptsToComplete == 1);
    assert(part.from == 50 && part.to == 90); // No extension after success.
    bacon::adjustPasses(profile, part, -1);
    assert(!part.checked && part.completionCounter == 19);
    bacon::adjustPasses(profile, part, 1);
    assert(part.checked && part.attempts == 20 && part.firstRunTo == 97);
    profile.requiredPasses = 25;
    bacon::refreshCompletion(profile);
    assert(!part.checked && part.completionCounter == 20);
    profile.requiredPasses = 1;
    bacon::refreshCompletion(profile);
    assert(part.checked);
    bacon::adjustPasses(profile, part, -100);
    assert(part.completionCounter == 0 && !part.checked);
    for (auto& range : profile.data.stages[0].ranges)
        bacon::adjustPasses(profile, range, 1);
    assert(profile.data.stages[0].checked);
    bacon::adjustPasses(profile, part, -1);
    assert(!profile.data.stages[0].checked);

    Profile legacy;
    legacy.data.fixedPartsVersion = 0;
    legacy.data.stages = bacon::makeFixedStages({10, 50});
    legacy.data.stages[0].ranges[0].completionCounter = 3;
    auto extended = legacy.data.stages[0];
    extended.ranges[0].to = 100;
    legacy.data.stages.push_back(extended);
    bacon::normalizeTraining(legacy);
    assert(legacy.data.stages.size() == 1);
    assert(legacy.data.stages[0].ranges.size() == 3);
    assert(legacy.data.stages[0].ranges[0].completionCounter == 3);
    assert(!legacy.data.stages[0].ranges[0].checked);
    legacy.requiredPasses = 0;
    bacon::refreshCompletion(legacy);
    assert(legacy.requiredPasses == 1);
    Profile cycles;
    cycles.requiredPasses = 2;
    cycles.data.stages = bacon::makeFixedStages({10, 20});
    auto& firstPart = cycles.data.stages[0].ranges[0];
    bacon::adjustPasses(cycles, firstPart, 2);
    bacon::setRunGoal(cycles, firstPart, 3);
    assert(firstPart.completionCounter == 2 && !firstPart.checked);
    assert(bacon::passGoal(cycles, cycles.data.stages[0].ranges[1]) == 2);
    assert(!bacon::appendCycle(cycles));
    bacon::adjustPasses(cycles, firstPart, 1);
    for (auto& range : cycles.data.stages[0].ranges)
        bacon::adjustPasses(cycles, range, bacon::passGoal(cycles, range) - range.completionCounter);
    firstPart.attempts = 11;
    firstPart.timePlayed = 8.f;
    firstPart.firstRunTo = 10;
    firstPart.recordedPasses = 3;
    const auto firstId = firstPart.id;
    assert(bacon::appendCycle(cycles));
    // References to vector elements must be re-acquired after appending.
    auto& cycle1 = cycles.data.stages[0];
    auto& cycle2 = cycles.data.stages[1];
    assert(cycle1.checked && !cycle2.checked);
    assert(cycle1.ranges[0].attempts == 11 && cycle1.ranges[0].timePlayed == 8.f);
    assert(cycle2.ranges[0].id != firstId);
    assert(cycle2.ranges[0].completionCounter == 0 && cycle2.ranges[0].recordedPasses == 0);
    assert(cycle2.ranges[0].attempts == 0 && cycle2.ranges[0].firstRunTo == 0);
    assert(cycle2.ranges[0].from == cycle1.ranges[0].from && cycle2.ranges[0].to == cycle1.ranges[0].to);
    assert(bacon::passGoal(cycles, cycle2.ranges[0]) == 3);
    bacon::setRunGoal(cycles, cycle2.ranges[0], 8);
    assert(bacon::passGoal(cycles, cycle1.ranges[0]) == 3);
    assert(bacon::passGoal(cycles, cycle2.ranges[1]) == 2);
    bacon::adjustPasses(cycles, cycle2.ranges[0], 1);
    assert(cycle1.ranges[0].completionCounter == 3 && cycle2.ranges[0].completionCounter == 1);
    bacon::normalizeTraining(cycles); // Used after loading saved JSON; must retain both cycles.
    assert(cycles.data.stages.size() == 2 && cycle2.ranges[0].requiredPasses == 8);
    assert(bacon::activeStage(cycles) == &cycle2);
    bacon::setRunGoal(cycles, cycle1.ranges[0], 4);
    assert(bacon::activeStage(cycles) == &cycle1); // A raised old goal reopens only that cycle.
    assert(cycle2.ranges[0].completionCounter == 1);
    bacon::setRunGoal(cycles, cycle1.ranges[0], 3);
    auto merged = bacon::mergeCycleRanges(cycles.data.stages, bacon::makeFixedStages({10, 20, 50}));
    assert(merged.size() == 2 && merged[0].ranges.size() == 3 && merged[1].ranges.size() == 4);
    assert(merged[0].ranges[0].attempts == 11);
    assert(merged[1].ranges[0].completionCounter == 1 && merged[1].ranges[0].requiredPasses == 8);
    assert(merged[1].ranges[2].completionCounter == 0);
    bacon::setRunGoal(cycles, cycle2.ranges[0], 0);
    assert(bacon::passGoal(cycles, cycle2.ranges[0]) == 2);
    assert(!bacon::appendCycle(cycles));
    std::cout << "Training, per-run goals and cycle regression checks passed\n";
}
