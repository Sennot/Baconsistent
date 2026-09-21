// Run the production GlobalStore::checkRun body with only game/UI side effects stubbed.
#include "../src/utils/fixedTraining.hpp"
#include <cassert>
#include <iostream>
#include <string>

namespace fmt { template<class... T> std::string format(const char*, T...) { return {}; } }
namespace geode {
namespace log { inline void error(const char*) {} }
enum class NotificationIcon { Success };
constexpr float NOTIFICATION_DEFAULT_TIME = 0;
struct Notification {
    template<class... T> static Notification* create(T...) { static Notification n; return &n; }
    void show() {}
};
}
using namespace geode;
struct Mod {
    static Mod* get() { static Mod m; return &m; }
    template<class T> T getSettingValue(const char*) { return T{}; }
};
struct RunClosedEvent { template<class... T> void send(T...) {} };
class GlobalStore {
public:
    Profile profile;
    float runStart = 0, runEnd = 0;
    int saves = 0;
    Profile* getProfileById(std::string const&) { return &profile; }
    void saveProfile(Profile const&) { ++saves; }
    int checkRun(std::string const&, float);
};
#include "check_run_body.inc"

int main()
{
    GlobalStore store;
    store.profile.requiredPasses = 2;
    store.profile.data.stages = bacon::makeFixedStages({10, 20, 30, 90});
    auto& parts = store.profile.data.stages[0].ranges;
    // Passing 10-20 and continuing to 35 still counts exactly one part.
    store.runStart = 10; store.runEnd = 35;
    assert(store.checkRun("p", 5) == -1);
    assert(parts[1].completionCounter == 1 && !parts[1].checked);
    assert(parts[2].completionCounter == 0 && parts[1].bestRunTo == 35);
    assert(store.checkRun("p", 6) == 0);
    assert(parts[1].completionCounter == 2 && parts[1].checked);
    assert(parts[1].recordedPasses == 2 && parts[1].attempts == 2);
    assert(parts[1].firstRunTo == 35 && parts[1].attemptsToComplete == 1);
    assert(parts[1].from == 10 && parts[1].to == 20);
    // Dying before the endpoint and spawning inside a part do not pass it.
    store.runStart = 20; store.runEnd = 29;
    assert(store.checkRun("p", 2) == -1 && parts[2].completionCounter == 0);
    store.runStart = 25; store.runEnd = 30;
    assert(store.checkRun("p", 2) == -1 && parts[2].completionCounter == 0);
    // Final endpoint is counted; failures between successes do not reset counters.
    store.runStart = 90; store.runEnd = 100;
    store.checkRun("p", 4);
    store.runEnd = 95;
    store.checkRun("p", 2);
    assert(parts.back().completionCounter == 1);
    store.runEnd = 100;
    assert(store.checkRun("p", 4) == 0 && parts.back().completionCounter == 2);
    // +/- never creates artificial gameplay stats or closes other parts.
    bacon::adjustPasses(store.profile, parts.back(), -1);
    assert(!parts.back().checked && parts.back().attempts == 3);
    assert(parts.back().recordedPasses == 2);
    bacon::adjustPasses(store.profile, parts.back(), 1);
    assert(parts.back().checked && parts.back().recordedPasses == 2);
    for (auto& part : parts)
        bacon::adjustPasses(store.profile, part, 2 - part.completionCounter);
    assert(store.profile.data.stages[0].checked);
    assert(store.checkRun("p", 4) == -1); // Continue after completing the profile.
    assert(parts.back().completionCounter == 3);
    assert(store.saves == 8);
    assert(bacon::appendCycle(store.profile));
    auto& oldCycle = store.profile.data.stages[0];
    auto& newCycle = store.profile.data.stages[1];
    auto& newPart = newCycle.ranges.back();
    bacon::setRunGoal(store.profile, newPart, 3);
    assert(store.checkRun("p", 4) == -1);
    assert(newPart.completionCounter == 1 && !newPart.checked);
    assert(oldCycle.ranges.back().completionCounter == 3); // No accidental credit to cycle 1.
    assert(newPart.attempts == 1 && newPart.timePlayed == 4);
    assert(store.checkRun("p", 4) == -1 && !newPart.checked);
    assert(store.checkRun("p", 4) == 0 && newPart.checked);
    assert(newPart.completionCounter == 3 && oldCycle.ranges.back().completionCounter == 3);
    bacon::setRunGoal(store.profile, oldCycle.ranges.back(), 4);
    assert(store.checkRun("p", 4) == 1); // Finish reopened historical goal, then resume cycle 2.
    assert(oldCycle.ranges.back().completionCounter == 4 && newPart.completionCounter == 3);
    assert(bacon::activeStage(store.profile) == &newCycle);
    std::cout << "Production checkRun regression checks passed\n";
}
