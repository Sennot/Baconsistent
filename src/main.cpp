#include <Geode/Geode.hpp>
#include <Geode/loader/GameEvent.hpp>
#include <Geode/loader/SettingV3.hpp>

#include "runtime/TrainingManager.hpp"

using namespace geode::prelude;

$on_game(Loaded) {
    listenForKeybindSettingPresses(
        "manual-add-repetition",
        [](Keybind const&, bool down, bool repeat, double) {
            if (!down || repeat || !PlayLayer::get()) {
                return;
            }
            auto& manager = baconsistent::TrainingManager::get();
            if (manager.enabled()) {
                (void)manager.manualAdjustLive(1);
            }
        }
    );

    listenForKeybindSettingPresses(
        "manual-complete-stage",
        [](Keybind const&, bool down, bool repeat, double) {
            if (!down || repeat || !PlayLayer::get()) {
                return;
            }
            auto& manager = baconsistent::TrainingManager::get();
            if (manager.enabled()) {
                (void)manager.manualCompleteLive();
            }
        }
    );
}
