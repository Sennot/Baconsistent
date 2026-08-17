#include "TrainingPopup.hpp"
#include "StageEditorLayer.hpp"
#include "ProfileCreatePopup.hpp"
#include "ProfileManagerPopup.hpp"
#include "Theme.hpp"
#include "../runtime/TrainingManager.hpp"

#include <Geode/ui/GeodeUI.hpp>
#include <Geode/ui/ScrollLayer.hpp>

#include <algorithm>
#include <array>
#include <cmath>

using namespace geode::prelude;

namespace {
constexpr float kPopupWidth = 440.f;
constexpr float kPopupHeight = 290.f;

ccColor3B cream() { return baconsistent::ui::theme::palette().text; }
ccColor3B muted() { return baconsistent::ui::theme::palette().muted; }
ccColor3B green() { return baconsistent::ui::theme::palette().success; }
ccColor3B bacon() { return baconsistent::ui::theme::palette().bacon; }
ccColor3B gold() { return baconsistent::ui::theme::palette().accent; }

CCSprite* spriteSized(char const* file, float width, float height) {
    auto path = baconsistent::ui::theme::resource(file);
    auto sprite = CCSprite::create(path.c_str());
    if (!sprite) {
        return nullptr;
    }
    auto const size = sprite->getContentSize();
    if (size.width > 0.f && size.height > 0.f) {
        sprite->setScaleX(width / size.width);
        sprite->setScaleY(height / size.height);
    }
    baconsistent::ui::theme::applySurfaceTint(sprite, file, std::string(file).find("dark") != std::string::npos);
    return sprite;
}

CCSprite* spriteFit(char const* file, float maxWidth, float maxHeight) {
    auto path = baconsistent::ui::theme::resource(file);
    auto sprite = CCSprite::create(path.c_str());
    if (!sprite) {
        return nullptr;
    }
    auto const size = sprite->getContentSize();
    if (size.width > 0.f && size.height > 0.f) {
        auto const scale = std::min(maxWidth / size.width, maxHeight / size.height);
        sprite->setScale(scale);
    }
    return sprite;
}

CCLabelBMFont* label(
    std::string const& text,
    char const* font,
    float scale,
    CCPoint position,
    ccColor3B color = {255, 255, 255},
    CCPoint anchor = {.5f, .5f}
) {
    auto node = CCLabelBMFont::create(text.c_str(), font);
    node->setScale(scale);
    node->setPosition(position);
    node->setColor(color);
    node->setAnchorPoint(anchor);
    return node;
}

CCNode* makeTabNode(std::string const& text, char const* icon, bool active) {
    auto root = CCNode::create();
    root->setContentSize({96.f, 31.f});

    auto bg = spriteSized(active ? "tab-pill-active.png"_spr : "tab-pill.png"_spr, 96.f, 31.f);
    if (bg) {
        bg->setPosition({48.f, 15.5f});
        root->addChild(bg);
    }

    if (auto spr = spriteFit(icon, 22.f, 22.f)) {
        spr->setPosition({17.f, 15.5f});
        root->addChild(spr);
    }

    auto title = label(text, "bigFont.fnt", .31f, {57.f, 15.5f}, active ? gold() : cream());
    root->addChild(title);
    return root;
}

CCNode* makeCard(CCSize size, bool dark = false) {
    auto root = CCNode::create();
    root->setContentSize(size);
    auto bg = spriteSized(dark ? "panel-card-dark.png"_spr : "panel-card.png"_spr, size.width, size.height);
    if (bg) {
        bg->setPosition({size.width / 2.f, size.height / 2.f});
        root->addChild(bg);
    }
    return root;
}

CCMenuItemSpriteExtra* actionButton(
    std::string const& text,
    char const* icon,
    CCObject* target,
    SEL_MenuHandler selector,
    float width = 106.f
) {
    auto root = CCNode::create();
    root->setContentSize({width, 31.f});
    auto bg = spriteSized("tab-pill.png"_spr, width, 31.f);
    if (bg) {
        bg->setPosition({width / 2.f, 15.5f});
        root->addChild(bg);
    }
    if (auto spr = spriteFit(icon, 21.f, 21.f)) {
        spr->setPosition({19.f, 15.5f});
        root->addChild(spr);
    }
    auto textLabel = label(text, "bigFont.fnt", .31f, {width / 2.f + 9.f, 15.5f}, cream());
    root->addChild(textLabel);
    return CCMenuItemSpriteExtra::create(root, target, selector);
}

CCMenuItemSpriteExtra* smallTextButton(
    std::string const& text,
    CCObject* target,
    SEL_MenuHandler selector,
    float width
) {
    auto root = CCNode::create();
    root->setContentSize({width, 24.f});
    if (auto bg = spriteSized("tab-pill.png"_spr, width, 24.f)) {
        bg->setPosition({width / 2.f, 12.f});
        root->addChild(bg);
    }
    root->addChild(label(text, "bigFont.fnt", .30f, {width / 2.f, 12.f}, cream()));
    return CCMenuItemSpriteExtra::create(root, target, selector);
}

CCMenuItemSpriteExtra* smallIconButton(
    char const* icon,
    CCObject* target,
    SEL_MenuHandler selector,
    float width
) {
    auto root = CCNode::create();
    root->setContentSize({width, 24.f});
    if (auto bg = spriteSized("tab-pill.png"_spr, width, 24.f)) {
        bg->setPosition({width / 2.f, 12.f});
        root->addChild(bg);
    }
    if (auto spr = spriteFit(icon, 17.f, 17.f)) {
        spr->setPosition({width / 2.f, 12.f});
        root->addChild(spr);
    }
    return CCMenuItemSpriteExtra::create(root, target, selector);
}

void addProgress(CCNode* parent, CCPoint pos, float width, float ratio) {
    ratio = std::clamp(ratio, 0.f, 1.f);
    if (auto track = spriteSized("progress-track.png"_spr, width, 9.f)) {
        track->setAnchorPoint({0.f, .5f});
        track->setPosition(pos);
        parent->addChild(track);
    }
    if (ratio > 0.001f) {
        if (auto fill = spriteSized("progress-fill.png"_spr, std::max(3.f, width * ratio), 9.f)) {
            fill->setAnchorPoint({0.f, .5f});
            fill->setPosition(pos);
            parent->addChild(fill);
        }
    }
}

int completedStages(baconsistent::core::TrainingPlan const& plan) {
    int complete = 0;
    for (std::size_t i = 0; i < plan.size(); ++i) {
        complete += plan.completed(i) ? 1 : 0;
    }
    return complete;
}

std::string rateText(baconsistent::core::AggregateStats const& stats) {
    return stats.attempts <= 0 ? "--" : fmt::format("{:.1f}%", stats.successRate());
}

void addMetric(CCNode* parent, std::string const& name, std::string const& value, float y) {
    parent->addChild(label(name, "chatFont.fnt", .48f, {12.f, y}, muted(), {0.f, .5f}));
    parent->addChild(label(value, "bigFont.fnt", .33f, {parent->getContentSize().width - 12.f, y}, cream(), {1.f, .5f}));
}


} // namespace

TrainingPopup* TrainingPopup::create(PauseLayer* host) {
    auto ret = new TrainingPopup();
    ret->m_pauseHost = host;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool TrainingPopup::init() {
    if (!Popup::init(kPopupWidth, kPopupHeight)) {
        return false;
    }
    if (m_bgSprite && (baconsistent::ui::theme::preset() != baconsistent::ui::theme::Preset::Original || baconsistent::ui::theme::customColorsEnabled())) {
        m_bgSprite->setColor(baconsistent::ui::theme::palette().surface);
    }

    setID("training-popup"_spr);

    // Own branded header. The actual gameplay remains completely clean; all
    // Baconsistent UI intentionally lives inside the pause flow.
    if (auto header = spriteFit("header-brand.png"_spr, 190.f, 38.f)) {
        header->setPosition({kPopupWidth / 2.f, 263.f});
        m_mainLayer->addChild(header, 3);
    }
    m_mainLayer->addChild(label("BACONSISTENT", "bigFont.fnt", .35f, {252.f, 263.f}, cream()), 4);
    if (baconsistent::ui::theme::isTurkmenistan() || baconsistent::ui::theme::isUdmurtia() || baconsistent::ui::theme::isTatarstan()) {
        auto const udmurt = baconsistent::ui::theme::isUdmurtia();
        auto const tatar = baconsistent::ui::theme::isTatarstan();
        auto const emblemName = Mod::get()->expandSpriteName(udmurt ? "ud-emblem.png" : (tatar ? "tt-emblem.png" : "tm-emblem.png"));
        auto const flagName = Mod::get()->expandSpriteName(udmurt ? "ud-flag.png" : (tatar ? "tt-flag.png" : "tm-flag.png"));
        if (auto emblem = spriteFit(emblemName.c_str(), 30.f, 30.f)) {
            emblem->setPosition({38.f, 262.f});
            m_mainLayer->addChild(emblem, 5);
        }
        if (auto flag = spriteFit(flagName.c_str(), 33.f, 22.f)) {
            flag->setPosition({74.f, 262.f});
            m_mainLayer->addChild(flag, 5);
        }
        m_mainLayer->addChild(label(
            udmurt ? "IZHEVSK EDITION" : (tatar ? "KAZAN EDITION" : "ASHGABAT EDITION"),
            "bigFont.fnt", .20f, {357.f, 246.f}, udmurt ? bacon() : green()
        ), 5);
    }
    if (auto separator = spriteSized("bacon-separator.png"_spr, 390.f, 7.f)) {
        separator->setPosition({kPopupWidth / 2.f, 240.f});
        m_mainLayer->addChild(separator, 2);
    }

    auto settingsIcon = spriteFit("icon-settings.png"_spr, 26.f, 26.f);
    if (settingsIcon) {
        auto settings = CCMenuItemSpriteExtra::create(
            settingsIcon,
            this,
            menu_selector(TrainingPopup::onSettings)
        );
        settings->setID("settings-button"_spr);
        settings->setPosition({407.f, 260.f});
        m_buttonMenu->addChild(settings, 5);
    }

    auto& manager = baconsistent::TrainingManager::get();
    // An unbound level opens directly on profile management. This keeps the
    // workflow explicit: scan/create/bind first, then train.
    if (!manager.hasActiveProfile()) {
        m_activePage = 3;
    }
    if (manager.hasActiveProfile()) {
        auto badge = makeCard({68.f, 25.f}, true);
        badge->setPosition({315.f, 248.f});
        badge->addChild(label(fmt::format("ROUND {}", manager.roundNumber()), "bigFont.fnt", .29f, {34.f, 12.5f}, gold()));
        m_mainLayer->addChild(badge, 3);
    }

    m_tabMenu = CCMenu::create();
    m_tabMenu->setPosition({0.f, 0.f});
    m_tabMenu->setID("tabs"_spr);
    m_mainLayer->addChild(m_tabMenu, 4);

    m_pageRoot = CCNode::create();
    m_pageRoot->setPosition({0.f, 0.f});
    m_pageRoot->setID("page-root"_spr);
    m_mainLayer->addChild(m_pageRoot, 1);

    rebuildTabs();
    rebuildPage();
    return true;
}

void TrainingPopup::onClose(CCObject* sender) {
    baconsistent::TrainingManager::get().clearRecentRoundBanner();
    Popup::onClose(sender);
}

void TrainingPopup::rebuildTabs() {
    if (!m_tabMenu) {
        return;
    }
    m_tabMenu->removeAllChildrenWithCleanup(true);

    struct TabDef { char const* name; char const* icon; };
    std::array<TabDef, 4> const tabs{{
        {"Stages", "icon-stages.png"_spr},
        {"Stats", "icon-stats.png"_spr},
        {"Rounds", "icon-rounds.png"_spr},
        {"Profiles", "icon-settings.png"_spr},
    }};

    std::array<float, 4> const x{58.f, 166.f, 274.f, 382.f};
    for (int i = 0; i < 4; ++i) {
        auto button = CCMenuItemSpriteExtra::create(
            makeTabNode(tabs[static_cast<std::size_t>(i)].name, tabs[static_cast<std::size_t>(i)].icon, i == m_activePage),
            this,
            menu_selector(TrainingPopup::onTab)
        );
        button->setTag(i);
        if (i == 0) button->setID("tab-stages"_spr);
        else if (i == 1) button->setID("tab-stats"_spr);
        else if (i == 2) button->setID("tab-rounds"_spr);
        else button->setID("tab-profiles"_spr);
        button->setPosition({x[static_cast<std::size_t>(i)], 218.f});
        m_tabMenu->addChild(button);
    }
}

void TrainingPopup::rebuildPage() {
    if (!m_pageRoot) {
        return;
    }
    m_pageRoot->removeAllChildrenWithCleanup(true);

    auto& manager = baconsistent::TrainingManager::get();
    if (m_activePage == 3) {
        buildProfilesPage();
        return;
    }
    if (!manager.loaded() || !manager.hasActiveProfile()) {
        buildUnavailablePage();
        return;
    }

    manager.refreshSettings();
    switch (m_activePage) {
        case 1: buildStatsPage(); break;
        case 2: buildRoundsPage(); break;
        default: buildStagesPage(); break;
    }
}

void TrainingPopup::buildUnavailablePage() {
    auto& manager = baconsistent::TrainingManager::get();
    auto card = makeCard({360.f, 145.f}, true);
    card->setPosition({40.f, 44.f});
    if (auto icon = spriteFit("icon-settings.png"_spr, 44.f, 44.f)) {
        icon->setPosition({180.f, 110.f});
        card->addChild(icon);
    }

    if (!manager.loaded()) {
        card->addChild(label("NO CLASSIC LEVEL", "bigFont.fnt", .44f, {180.f, 76.f}, gold()));
        auto info = label("Open a classic level and pause.\nBaconsistent never draws a permanent gameplay HUD.", "chatFont.fnt", .52f, {180.f, 44.f}, muted());
        info->setAlignment(kCCTextAlignmentCenter);
        card->addChild(info);
    }
    else {
        card->addChild(label("NO PROFILE BOUND", "bigFont.fnt", .44f, {180.f, 78.f}, gold()));
        auto info = label(
            manager.hasScannableStartPositions()
                ? "StartPos detected, but nothing is attached automatically.\nOpen the Profiles tab to create or bind a profile."
                : "This level has no active profile.\nOpen Profiles and bind any profile you created earlier.",
            "chatFont.fnt", .50f, {180.f, 44.f}, muted()
        );
        info->setAlignment(kCCTextAlignmentCenter);
        card->addChild(info);
    }
    if (baconsistent::ui::theme::isTurkmenistan()) {
        card->addChild(label("PROFILE PERMIT REQUIRED BY ASHGABAT", "bigFont.fnt", .23f, {180.f, 15.f}, green()));
    }
    else if (baconsistent::ui::theme::isUdmurtia()) {
        card->addChild(label("PROFILE STAMP REQUIRED BY IZHEVSK", "bigFont.fnt", .23f, {180.f, 15.f}, bacon()));
    }
    else if (baconsistent::ui::theme::isTatarstan()) {
        card->addChild(label("PROFILE DECREE REQUIRED BY KAZAN", "bigFont.fnt", .23f, {180.f, 15.f}, green()));
    }
    m_pageRoot->addChild(card);
}

void TrainingPopup::buildStagesPage() {
    auto& manager = baconsistent::TrainingManager::get();
    auto const& plan = manager.plan();
    auto const selected = std::clamp(manager.selected(), 0, std::max(0, plan.parts() - 1));
    auto const selectedIndex = static_cast<std::size_t>(selected);

    // Round-complete acknowledgement is deliberately pause-only.
    if (manager.recentCompletedRound() > 0) {
        auto ribbon = CCNode::create();
        ribbon->setContentSize({388.f, 18.f});
        if (auto bg = spriteSized("round-ribbon.png"_spr, 388.f, 18.f)) {
            bg->setPosition({194.f, 9.f});
            ribbon->addChild(bg);
        }
        ribbon->addChild(label(
            fmt::format("ROUND {} COMPLETE  -  ROUND {} READY", manager.recentCompletedRound(), manager.roundNumber()),
            "bigFont.fnt", .23f, {194.f, 9.f}, cream()
        ));
        ribbon->setPosition({26.f, 184.f});
        m_pageRoot->addChild(ribbon, 5);
    }

    // Left: selected stage card.
    auto selectedCard = makeCard({160.f, 150.f}, true);
    selectedCard->setPosition({24.f, 32.f});
    if (auto icon = spriteFit("icon-target.png"_spr, 27.f, 27.f)) {
        icon->setPosition({22.f, 131.f});
        selectedCard->addChild(icon);
    }
    selectedCard->addChild(label(
        baconsistent::ui::theme::isTurkmenistan() ? "ASHGABAT STAGE"
            : (baconsistent::ui::theme::isUdmurtia() ? "IZHEVSK STAGE" : (baconsistent::ui::theme::isTatarstan() ? "KAZAN STAGE" : "SELECTED STAGE")),
        "bigFont.fnt", .31f, {91.f, 132.f}, gold()
    ));
    selectedCard->addChild(label(manager.segmentRangeText(selected), "bigFont.fnt", .47f, {80.f, 103.f}, cream()));

    auto const count = plan.count(selectedIndex);
    auto const target = plan.target(selectedIndex);
    selectedCard->addChild(label(fmt::format("{} / {}", count, target), "goldFont.fnt", .49f, {80.f, 78.f}, plan.completed(selectedIndex) ? green() : bacon()));
    addProgress(selectedCard, {17.f, 68.f}, 126.f, target > 0 ? static_cast<float>(count) / static_cast<float>(target) : 0.f);

    // Per-stage target controls live inside the card, so they never collide
    // with the pause action row below the card.
    auto targetMenu = CCMenu::create();
    targetMenu->setPosition({0.f, 0.f});
    targetMenu->setContentSize(selectedCard->getContentSize());
    auto minus = smallTextButton("-", this, menu_selector(TrainingPopup::onTargetMinus), 25.f);
    auto plus = smallTextButton("+", this, menu_selector(TrainingPopup::onTargetPlus), 25.f);
    minus->setPosition({34.f, 51.f});
    plus->setPosition({126.f, 51.f});
    targetMenu->addChild(minus);
    targetMenu->addChild(plus);
    auto targetReset = smallTextButton(fmt::format("Target {}", target), this, menu_selector(TrainingPopup::onTargetReset), 62.f);
    targetReset->setPosition({80.f, 51.f});
    targetMenu->addChild(targetReset);
    selectedCard->addChild(targetMenu, 4);

    // Manual correction controls. They only edit repetitions; statistics are
    // intentionally unchanged so success rate / streaks stay honest.
    auto manualMenu = CCMenu::create();
    manualMenu->setPosition({0.f, 0.f});
    manualMenu->setContentSize(selectedCard->getContentSize());
    auto repMinus = smallTextButton("-1", this, menu_selector(TrainingPopup::onRepMinus), 36.f);
    auto repPlus = smallTextButton("+1", this, menu_selector(TrainingPopup::onRepPlus), 36.f);
    auto repDone = smallIconButton("badge-complete.png"_spr, this, menu_selector(TrainingPopup::onCompleteStage), 40.f);
    repMinus->setPosition({31.f, 22.f});
    repPlus->setPosition({80.f, 22.f});
    repDone->setPosition({130.f, 22.f});
    manualMenu->addChild(repMinus);
    manualMenu->addChild(repPlus);
    manualMenu->addChild(repDone);
    selectedCard->addChild(manualMenu, 4);
    m_pageRoot->addChild(selectedCard);

    // Right: scrollable fixed-stage browser, displayed backwards by default.
    auto listPanel = makeCard({226.f, 150.f}, false);
    listPanel->setPosition({190.f, 32.f});
    listPanel->addChild(label(
        baconsistent::ui::theme::isTurkmenistan()
            ? fmt::format("STATE CONSISTENCY {}/{}", completedStages(plan), plan.parts())
            : (baconsistent::ui::theme::isUdmurtia()
                ? fmt::format("UDMURT CONSISTENCY {}/{}", completedStages(plan), plan.parts())
                : (baconsistent::ui::theme::isTatarstan()
                ? fmt::format("TATAR CONSISTENCY {}/{}", completedStages(plan), plan.parts())
                : fmt::format("{} / {} STAGES COMPLETE", completedStages(plan), plan.parts()))),
        "bigFont.fnt", .28f, {113.f, 139.f}, gold()
    ));
    listPanel->addChild(label(
        fmt::format("{}  |  {} StartPos", manager.currentProfileName(), manager.detectedStartPosCount()),
        "chatFont.fnt", .43f, {113.f, 124.f}, muted()
    ));

    auto scroll = ScrollLayer::create(CCSize{206.f, 105.f}, true, true);
    scroll->setPosition({10.f, 9.f});
    scroll->setID("stage-scroll"_spr);
    scroll->setCancelTouchLimit(5.f);
    scroll->m_contentLayer->setContentSize({206.f, std::max(105.f, static_cast<float>(plan.parts()) * 29.f)});

    auto stageMenu = CCMenu::create();
    stageMenu->setPosition({0.f, 0.f});
    stageMenu->setContentSize(scroll->m_contentLayer->getContentSize());
    stageMenu->setID("stage-menu"_spr);

    auto const totalHeight = scroll->m_contentLayer->getContentSize().height;
    for (int visual = 0; visual < plan.parts(); ++visual) {
        auto const i = plan.parts() - 1 - visual;
        auto const idx = static_cast<std::size_t>(i);
        auto const isSelected = i == selected;
        auto const isComplete = plan.completed(idx);

        auto row = CCNode::create();
        row->setContentSize({198.f, 25.f});
        auto const bgName = isComplete ? "stage-row-complete.png"_spr : (isSelected ? "stage-row-selected.png"_spr : "stage-row.png"_spr);
        if (auto bg = spriteSized(bgName, 198.f, 25.f)) {
            bg->setPosition({99.f, 12.5f});
            row->addChild(bg);
        }

        row->addChild(label(manager.segmentRangeText(i), "bigFont.fnt", .28f, {10.f, 12.5f}, isSelected ? gold() : cream(), {0.f, .5f}));
        row->addChild(label(
            fmt::format("{}/{}", plan.count(idx), plan.target(idx)),
            "bigFont.fnt", .27f, {174.f, 12.5f}, isComplete ? green() : muted(), {1.f, .5f}
        ));
        if (isComplete) {
            if (auto check = spriteFit("badge-complete.png"_spr, 18.f, 18.f)) {
                check->setPosition({187.f, 12.5f});
                row->addChild(check);
            }
        }

        auto item = CCMenuItemSpriteExtra::create(row, this, menu_selector(TrainingPopup::onStage));
        item->setTag(i);
        item->setPosition({103.f, totalHeight - 14.5f - static_cast<float>(visual) * 29.f});
        stageMenu->addChild(item);
    }
    scroll->m_contentLayer->addChild(stageMenu);
    scroll->scrollToTop();
    listPanel->addChild(scroll, 2);
    m_pageRoot->addChild(listPanel);

    // Bottom action row.
    auto actions = CCMenu::create();
    actions->setPosition({0.f, 0.f});
    actions->setID("stage-actions"_spr);

    auto back = actionButton("Back", "icon-stages.png"_spr, this, menu_selector(TrainingPopup::onRecommended), 82.f);
    back->setPosition({62.f, 15.f});
    actions->addChild(back);

    auto reset = actionButton("Reset", "icon-reset.png"_spr, this, menu_selector(TrainingPopup::onResetPart), 82.f);
    reset->setPosition({158.f, 15.f});
    actions->addChild(reset);

    auto editor = actionButton("Editor", "icon-target.png"_spr, this, menu_selector(TrainingPopup::onStageEditor), 88.f);
    editor->setPosition({258.f, 15.f});
    actions->addChild(editor);

    auto profile = actionButton("Profiles", "icon-settings.png"_spr, this, menu_selector(TrainingPopup::onProfilesTab), 88.f);
    profile->setPosition({360.f, 15.f});
    actions->addChild(profile);
    m_pageRoot->addChild(actions, 4);
}

void TrainingPopup::buildStatsPage() {
    auto& manager = baconsistent::TrainingManager::get();
    auto const& stats = manager.stats();
    auto const selected = static_cast<std::size_t>(std::max(0, manager.selected()));

    struct CardDef {
        char const* title;
        baconsistent::core::AggregateStats const* stats;
        char const* icon;
    };
    std::array<CardDef, 3> const cards{{
        {"SELECTED STAGE", &stats.stage(selected), "icon-target.png"_spr},
        {"CURRENT ROUND", &stats.round(), "icon-rounds.png"_spr},
        {"LIFETIME", &stats.lifetime(), "icon-stats.png"_spr},
    }};
    std::array<float, 3> const xs{20.f, 158.f, 296.f};

    for (std::size_t i = 0; i < cards.size(); ++i) {
        auto card = makeCard({124.f, 153.f}, i == 1);
        card->setPosition({xs[i], 34.f});
        if (auto icon = spriteFit(cards[i].icon, 27.f, 27.f)) {
            icon->setPosition({20.f, 130.f});
            card->addChild(icon);
        }
        card->addChild(label(cards[i].title, "bigFont.fnt", .28f, {72.f, 131.f}, i == 1 ? bacon() : gold()));
        auto const& st = *cards[i].stats;
        addMetric(card, "Attempts", fmt::format("{}", st.attempts), 105.f);
        addMetric(card, "Successes", fmt::format("{}", st.successes), 85.f);
        addMetric(card, "Success rate", rateText(st), 65.f);
        addMetric(card, "Current streak", fmt::format("{}", st.currentStreak), 45.f);
        addMetric(card, "Best streak", fmt::format("{}", st.bestStreak), 25.f);
        addMetric(card, "Playtime", baconsistent::core::formatDuration(st.playtimeSeconds), 8.f);
        m_pageRoot->addChild(card);
    }

    auto const& plan = manager.plan();
    auto footer = makeCard({400.f, 25.f}, true);
    footer->setPosition({20.f, 5.f});
    footer->addChild(label(
        fmt::format("Round {}  |  {}/{} reps  |  {}/{} stages", manager.roundNumber(), plan.totalCompletions(), plan.totalGoal(), completedStages(plan), plan.parts()),
        "bigFont.fnt", .28f, {200.f, 12.5f}, cream()
    ));
    m_pageRoot->addChild(footer);
    if (baconsistent::ui::theme::isTurkmenistan()) {
        m_pageRoot->addChild(label("STATISTICS CERTIFIED BY THE ASHGABAT CONSISTENCY OFFICE", "bigFont.fnt", .18f, {220.f, 2.f}, green()));
    }
    else if (baconsistent::ui::theme::isUdmurtia()) {
        m_pageRoot->addChild(label("STATISTICS STAMPED BY THE IZHEVSK CONSISTENCY BUREAU", "bigFont.fnt", .18f, {220.f, 2.f}, bacon()));
    }
    else if (baconsistent::ui::theme::isTatarstan()) {
        m_pageRoot->addChild(label("STATISTICS SEALED BY THE KAZAN CONSISTENCY MINISTRY", "bigFont.fnt", .18f, {220.f, 2.f}, green()));
    }
}

void TrainingPopup::buildRoundsPage() {
    auto& manager = baconsistent::TrainingManager::get();
    auto const& plan = manager.plan();
    auto const& stats = manager.stats();

    auto current = makeCard({171.f, 153.f}, true);
    current->setPosition({20.f, 34.f});
    if (auto icon = spriteFit("icon-rounds.png"_spr, 36.f, 36.f)) {
        icon->setPosition({27.f, 127.f});
        current->addChild(icon);
    }
    current->addChild(label(fmt::format("ROUND {}", manager.roundNumber()), "goldFont.fnt", .47f, {103.f, 128.f}, gold()));
    current->addChild(label(fmt::format("{}/{}", plan.totalCompletions(), plan.totalGoal()), "bigFont.fnt", .44f, {85.5f, 97.f}, cream()));
    addProgress(current, {20.f, 81.f}, 131.f, plan.totalGoal() > 0 ? static_cast<float>(plan.totalCompletions()) / static_cast<float>(plan.totalGoal()) : 0.f);
    addMetric(current, "Stages", fmt::format("{}/{}", completedStages(plan), plan.parts()), 61.f);
    addMetric(current, "Attempts", fmt::format("{}", stats.round().attempts), 42.f);
    addMetric(current, "Success", rateText(stats.round()), 23.f);
    addMetric(current, "Best streak", fmt::format("{}", stats.round().bestStreak), 8.f);
    m_pageRoot->addChild(current);

    auto historyCard = makeCard({221.f, 153.f}, false);
    historyCard->setPosition({199.f, 34.f});
    historyCard->addChild(label(
        baconsistent::ui::theme::isTurkmenistan() ? "STATE ROUNDS OF GLORY"
            : (baconsistent::ui::theme::isUdmurtia() ? "ROUNDS OF UDMURTIA" : (baconsistent::ui::theme::isTatarstan() ? "ROUNDS OF TATARSTAN" : "COMPLETED ROUNDS")),
        "bigFont.fnt", .28f, {110.5f, 137.f}, gold()
    ));

    auto const& history = stats.history();
    if (history.empty()) {
        historyCard->addChild(label("No completed rounds yet", "chatFont.fnt", .55f, {110.5f, 86.f}, muted()));
        historyCard->addChild(label("Finish every fixed stage to start Round 2.", "chatFont.fnt", .43f, {110.5f, 65.f}, muted()));
    }
    else {
        auto const visible = std::min<std::size_t>(history.size(), 5);
        for (std::size_t row = 0; row < visible; ++row) {
            auto const& summary = history[history.size() - 1 - row];
            auto y = 114.f - static_cast<float>(row) * 24.f;
            historyCard->addChild(label(fmt::format("Round {}", summary.round), "bigFont.fnt", .30f, {13.f, y}, cream(), {0.f, .5f}));
            historyCard->addChild(label(
                fmt::format("{} att  |  {}  |  x{}", summary.stats.attempts, rateText(summary.stats), summary.stats.bestStreak),
                "chatFont.fnt", .43f, {207.f, y}, muted(), {1.f, .5f}
            ));
        }
    }
    m_pageRoot->addChild(historyCard);

    auto menu = CCMenu::create();
    menu->setPosition({0.f, 0.f});
    auto reset = actionButton("Reset Round", "icon-reset.png"_spr, this, menu_selector(TrainingPopup::onResetRound), 124.f);
    reset->setPosition({220.f, 15.f});
    menu->addChild(reset);
    m_pageRoot->addChild(menu, 4);
}

void TrainingPopup::buildProfilesPage() {
    auto& manager = baconsistent::TrainingManager::get();
    auto const profiles = manager.profiles();

    auto status = makeCard({400.f, 70.f}, true);
    status->setPosition({20.f, 119.f});
    status->addChild(label(
        baconsistent::ui::theme::isTurkmenistan() ? "STATE PROFILE REGISTRY"
            : (baconsistent::ui::theme::isUdmurtia() ? "UDMURT PROFILE REGISTRY" : (baconsistent::ui::theme::isTatarstan() ? "TATAR PROFILE REGISTRY" : "PROFILE MANAGER")),
        "bigFont.fnt", .36f, {200.f, 54.f}, gold()
    ));
    status->addChild(label(
        manager.loaded() ? manager.currentLevelName() : "No classic level loaded",
        "bigFont.fnt", .29f, {14.f, 31.f}, cream(), {0.f, .5f}
    ));

    std::string state = "UNBOUND";
    if (manager.hasActiveProfile()) {
        state = fmt::format("BOUND: {}", manager.currentProfileName());
    }
    else if (manager.hasScannableStartPositions()) {
        state = fmt::format("{} StartPos detected", manager.scannableStartPosCount());
    }
    status->addChild(label(state, "chatFont.fnt", .46f, {386.f, 15.f}, manager.hasActiveProfile() ? green() : muted(), {1.f, .5f}));
    m_pageRoot->addChild(status);

    auto body = makeCard({400.f, 105.f}, false);
    body->setPosition({20.f, 5.f});
    body->addChild(label(
        baconsistent::ui::theme::isTurkmenistan() ? "ASHGABAT PROFILE CONTROL"
            : (baconsistent::ui::theme::isUdmurtia() ? "IZHEVSK PROFILE BUREAU" : (baconsistent::ui::theme::isTatarstan() ? "KAZAN PROFILE KHANATE" : "PROFILE CONTROL")),
        "bigFont.fnt", .30f, {200.f, 88.f}, gold()
    ));
    body->addChild(label(
        profiles.empty()
            ? "No saved profiles yet. Open a StartPos copy and create one."
            : fmt::format("{} saved profile{}  •  Select, bind, unbind or delete safely in the manager.", profiles.size(), profiles.size() == 1 ? "" : "s"),
        "chatFont.fnt", .46f, {200.f, 60.f}, muted()
    ));

    auto menu = CCMenu::create();
    menu->setPosition({0.f, 0.f});
    auto manage = actionButton(
        baconsistent::ui::theme::isTurkmenistan() ? "Open Registry"
            : (baconsistent::ui::theme::isUdmurtia() ? "Open Udmurt Registry" : (baconsistent::ui::theme::isTatarstan() ? "Open Tatar Registry" : "Manage Profiles")),
        "icon-settings.png"_spr,
        this,
        menu_selector(TrainingPopup::onOpenProfileManager),
        150.f
    );
    manage->setPosition({200.f, 29.f});
    menu->addChild(manage);
    body->addChild(menu, 4);
    m_pageRoot->addChild(body);
}

void TrainingPopup::onStageEditor(CCObject* sender) {
    auto& manager = baconsistent::TrainingManager::get();
    if (!m_pauseHost || !manager.hasActiveProfile()) {
        FLAlertLayer::create("Stage Editor", "Bind a profile first.", "OK")->show();
        return;
    }
    auto* editor = baconsistent::ui::StageEditorLayer::create(m_pauseHost);
    if (!editor) {
        FLAlertLayer::create("Stage Editor", "Could not open the experimental editor on this level.", "OK")->show();
        return;
    }
    m_pauseHost->addChild(editor, 10000);
    onClose(sender);
}

void TrainingPopup::onTab(CCObject* sender) {
    m_activePage = std::clamp(sender ? static_cast<CCNode*>(sender)->getTag() : 0, 0, 3);
    rebuildTabs();
    rebuildPage();
}

void TrainingPopup::onStage(CCObject* sender) {
    if (!sender) {
        return;
    }
    baconsistent::TrainingManager::get().select(static_cast<CCNode*>(sender)->getTag());
    rebuildPage();
}

void TrainingPopup::onRecommended(CCObject*) {
    baconsistent::TrainingManager::get().selectRecommended();
    rebuildPage();
}


void TrainingPopup::onResetPart(CCObject*) {
    createQuickPopup(
        "Reset stage",
        "Reset repetitions for the selected fixed stage? Statistics are kept.",
        "Cancel",
        "Reset",
        [this](auto, bool second) {
            if (second) {
                baconsistent::TrainingManager::get().resetSelected();
                rebuildPage();
            }
        }
    );
}

void TrainingPopup::onResetRound(CCObject*) {
    createQuickPopup(
        "Reset current round",
        "Reset all repetition counters in this round? Lifetime statistics and completed-round history are kept.",
        "Cancel",
        "Reset",
        [this](auto, bool second) {
            if (second) {
                baconsistent::TrainingManager::get().resetRoundProgress();
                rebuildPage();
            }
        }
    );
}

void TrainingPopup::onTargetMinus(CCObject*) {
    baconsistent::TrainingManager::get().adjustSelectedTarget(-1);
    rebuildPage();
}

void TrainingPopup::onTargetPlus(CCObject*) {
    baconsistent::TrainingManager::get().adjustSelectedTarget(1);
    rebuildPage();
}

void TrainingPopup::onTargetReset(CCObject*) {
    baconsistent::TrainingManager::get().resetSelectedTarget();
    rebuildPage();
}

void TrainingPopup::onRepMinus(CCObject*) {
    baconsistent::TrainingManager::get().manualAdjustSelected(-1);
    rebuildPage();
}

void TrainingPopup::onRepPlus(CCObject*) {
    baconsistent::TrainingManager::get().manualAdjustSelected(1);
    rebuildPage();
}

void TrainingPopup::onCompleteStage(CCObject*) {
    baconsistent::TrainingManager::get().manualCompleteSelected();
    rebuildPage();
}


void TrainingPopup::onProfilesTab(CCObject*) {
    m_activePage = 3;
    rebuildTabs();
    rebuildPage();
}

void TrainingPopup::onOpenProfileManager(CCObject*) {
    if (auto* popup = ProfileManagerPopup::create()) {
        popup->show();
    }
}

void TrainingPopup::onSettings(CCObject*) {
    openSettingsPopup(Mod::get(), true);
}
