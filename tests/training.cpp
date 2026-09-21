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
    std::cout << "Training regression checks passed\n";
}
