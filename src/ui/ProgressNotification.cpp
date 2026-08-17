#include "ProgressNotification.hpp"
#include "Theme.hpp"

#include <Geode/Geode.hpp>
#include <Geode/ui/Notification.hpp>

#include <algorithm>

using namespace geode::prelude;

namespace baconsistent::ui {

namespace {
CCNode* makeBrandedIcon(char const* resource) {
    auto* root = CCNode::create();
    root->setContentSize({34.f, 34.f});

    if (auto* sprite = CCSprite::create(resource)) {
        auto const size = sprite->getContentSize();
        if (size.width > 0.f && size.height > 0.f) {
            sprite->setScale(std::min(30.f / size.width, 30.f / size.height));
        }
        sprite->setPosition({17.f, 17.f});
        root->addChild(sprite);
    }
    return root;
}

void show(std::string const& text, char const* iconResource, float duration) {
    std::string themedIcon = iconResource;
    if (theme::isTurkmenistan()) {
        themedIcon = Mod::get()->expandSpriteName("tm-emblem.png");
    }
    else if (theme::isUdmurtia()) {
        themedIcon = Mod::get()->expandSpriteName("ud-emblem.png");
    }
    else if (theme::isTatarstan()) {
        themedIcon = Mod::get()->expandSpriteName("tt-emblem.png");
    }
    if (auto* toast = Notification::create(text.c_str(), makeBrandedIcon(themedIcon.c_str()), duration)) {
        toast->show();
    }
}
} // namespace

void showProgressNotification(ProgressEvent const& event) {
    if (!Mod::get()->getSettingValue<bool>("success-notifications")) {
        return;
    }

    auto& manager = TrainingManager::get();
    auto const range = manager.segmentRangeText(event.segmentIndex);
    std::string themeSuffix;
    if (theme::isTurkmenistan()) {
        themeSuffix = "   ASHGABAT APPROVED";
    }
    else if (theme::isUdmurtia()) {
        themeSuffix = "   IZHEVSK APPROVED";
    }
    else if (theme::isTatarstan()) {
        themeSuffix = "   KAZAN APPROVED";
    }

    if (event.roundCompleted) {
        show(
            fmt::format("ROUND {} COMPLETE   ROUND {} STARTED{}", event.completedRound, event.newRound, themeSuffix),
            "badge-round.png"_spr,
            2.8f
        );
        return;
    }

    if (event.segmentCompleted) {
        show(
            fmt::format("{} MASTERED   {}/{}{}", range, event.repetitions, event.target, themeSuffix),
            "badge-complete.png"_spr,
            2.4f
        );
        return;
    }

    auto const left = std::max(0, event.target - event.repetitions);
    show(
        fmt::format("{}   {}/{}   {} LEFT{}", range, event.repetitions, event.target, left, themeSuffix),
        "pause-icon.png"_spr,
        1.8f
    );
}

} // namespace baconsistent::ui
