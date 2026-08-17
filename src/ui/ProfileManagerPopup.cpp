#include "ProfileManagerPopup.hpp"

#include "ProfileCreatePopup.hpp"
#include "LegacyImportPopup.hpp"
#include "Theme.hpp"
#include "../runtime/TrainingManager.hpp"

#include <Geode/ui/ScrollLayer.hpp>

#include <algorithm>

using namespace geode::prelude;

namespace {
ccColor3B textColor() { return baconsistent::ui::theme::palette().text; }
ccColor3B mutedColor() { return baconsistent::ui::theme::palette().muted; }
ccColor3B accentColor() { return baconsistent::ui::theme::palette().accent; }
ccColor3B successColor() { return baconsistent::ui::theme::palette().success; }
ccColor3B dangerColor() { return baconsistent::ui::theme::palette().danger; }
ccColor3B baconColor() { return baconsistent::ui::theme::palette().bacon; }

CCLabelBMFont* makeLabel(
    std::string const& text,
    char const* font,
    float scale,
    CCPoint position,
    ccColor3B color,
    CCPoint anchor = {.5f, .5f}
) {
    auto* node = CCLabelBMFont::create(text.c_str(), font);
    node->setScale(scale);
    node->setPosition(position);
    node->setColor(color);
    node->setAnchorPoint(anchor);
    return node;
}

CCSprite* themedSprite(char const* expanded, float width, float height, bool dark = false) {
    auto path = baconsistent::ui::theme::resource(expanded);
    auto* sprite = CCSprite::create(path.c_str());
    if (!sprite) {
        return nullptr;
    }
    auto const size = sprite->getContentSize();
    if (size.width > 0.f && size.height > 0.f) {
        sprite->setScaleX(width / size.width);
        sprite->setScaleY(height / size.height);
    }
    baconsistent::ui::theme::applySurfaceTint(sprite, expanded, dark);
    return sprite;
}

CCNode* card(CCSize size, bool dark = false) {
    auto* root = CCNode::create();
    root->setContentSize(size);
    if (auto* bg = themedSprite(dark ? "panel-card-dark.png"_spr : "panel-card.png"_spr, size.width, size.height, dark)) {
        bg->setPosition({size.width / 2.f, size.height / 2.f});
        root->addChild(bg);
    }
    return root;
}

CCMenuItemSpriteExtra* textButton(
    std::string const& title,
    CCObject* target,
    SEL_MenuHandler selector,
    float width,
    ccColor3B color = textColor()
) {
    auto* root = CCNode::create();
    root->setContentSize({width, 29.f});
    if (auto* bg = themedSprite("tab-pill.png"_spr, width, 29.f)) {
        bg->setPosition({width / 2.f, 14.5f});
        root->addChild(bg);
    }
    root->addChild(makeLabel(title, "bigFont.fnt", .30f, {width / 2.f, 14.5f}, color));
    return CCMenuItemSpriteExtra::create(root, target, selector);
}

std::string shortName(std::string value, std::size_t limit) {
    if (value.size() <= limit) {
        return value;
    }
    if (limit <= 3) {
        return value.substr(0, limit);
    }
    return value.substr(0, limit - 3) + "...";
}
} // namespace

ProfileManagerPopup* ProfileManagerPopup::create() {
    auto* ret = new ProfileManagerPopup();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ProfileManagerPopup::init() {
    if (!Popup::init(420.f, 272.f)) {
        return false;
    }
    setID("profile-manager-popup"_spr);
    if (m_bgSprite && (baconsistent::ui::theme::preset() != baconsistent::ui::theme::Preset::Original || baconsistent::ui::theme::customColorsEnabled())) {
        m_bgSprite->setColor(baconsistent::ui::theme::palette().surface);
    }

    m_content = CCNode::create();
    m_content->setContentSize({420.f, 272.f});
    m_mainLayer->addChild(m_content);

    auto const profiles = baconsistent::TrainingManager::get().profiles();
    auto const& manager = baconsistent::TrainingManager::get();
    if (manager.hasActiveProfile()) {
        m_selectedProfileId = manager.levelKey();
    }
    else if (!profiles.empty()) {
        m_selectedProfileId = profiles.front().id;
    }

    rebuild();
    return true;
}

void ProfileManagerPopup::rebuild() {
    if (!m_content) {
        return;
    }
    m_content->removeAllChildrenWithCleanup(true);

    auto& manager = baconsistent::TrainingManager::get();
    auto const profiles = manager.profiles();
    if (!m_selectedProfileId.empty()) {
        auto it = std::find_if(profiles.begin(), profiles.end(), [this](auto const& p) { return p.id == m_selectedProfileId; });
        if (it == profiles.end()) {
            m_selectedProfileId.clear();
        }
    }
    if (m_selectedProfileId.empty() && !profiles.empty()) {
        m_selectedProfileId = profiles.front().id;
    }

    auto const turkmen = baconsistent::ui::theme::isTurkmenistan();
    auto const udmurt = baconsistent::ui::theme::isUdmurtia();
    auto const tatar = baconsistent::ui::theme::isTatarstan();
    auto const managerTitle = turkmen ? "STATE PROFILE REGISTRY" : (udmurt ? "UDMURT PROFILE REGISTRY" : (tatar ? "TATAR PROFILE REGISTRY" : "PROFILE MANAGER"));
    m_content->addChild(makeLabel(
        managerTitle,
        "bigFont.fnt", .46f, {210.f, 249.f}, accentColor()
    ));

    if (turkmen || udmurt || tatar) {
        auto flagPath = Mod::get()->expandSpriteName(udmurt ? "ud-flag.png" : (tatar ? "tt-flag.png" : "tm-flag.png"));
        if (auto* flag = CCSprite::create(flagPath.c_str())) {
            auto const s = flag->getContentSize();
            if (s.width > 0.f && s.height > 0.f) flag->setScale(std::min(31.f / s.width, 19.f / s.height));
            flag->setPosition({42.f, 249.f});
            m_content->addChild(flag, 5);
        }
    }

    auto* status = card({386.f, 42.f}, true);
    status->setPosition({17.f, 195.f});
    status->addChild(makeLabel(manager.loaded() ? shortName(manager.currentLevelName(), 28) : "No level loaded", "bigFont.fnt", .28f, {12.f, 27.f}, textColor(), {0.f, .5f}));
    std::string binding = "UNBOUND";
    if (manager.hasActiveProfile()) {
        binding = fmt::format("BOUND: {}", shortName(manager.currentProfileName(), 24));
    }
    status->addChild(makeLabel(binding, "chatFont.fnt", .43f, {374.f, 14.f}, manager.hasActiveProfile() ? successColor() : mutedColor(), {1.f, .5f}));
    m_content->addChild(status);

    auto* list = card({232.f, 151.f}, false);
    list->setPosition({17.f, 35.f});
    list->addChild(makeLabel(
        turkmen ? "ASHGABAT ARCHIVE" : (udmurt ? "IZHEVSK ARCHIVE" : (tatar ? "KAZAN ARCHIVE" : "SAVED PROFILES")),
        "bigFont.fnt", .29f, {116.f, 137.f}, accentColor()
    ));
    m_content->addChild(list);

    if (profiles.empty()) {
        auto* empty = makeLabel("No saved profiles yet.\nOpen a StartPos copy and create one.", "chatFont.fnt", .48f, {116.f, 76.f}, mutedColor());
        empty->setAlignment(kCCTextAlignmentCenter);
        list->addChild(empty);
    }
    else {
        auto* scroll = ScrollLayer::create(CCSize{216.f, 112.f}, true, true);
        scroll->setPosition({8.f, 8.f});
        scroll->setCancelTouchLimit(5.f);
        float const rowHeight = 37.f;
        auto const contentHeight = std::max(112.f, static_cast<float>(profiles.size()) * rowHeight);
        scroll->m_contentLayer->setContentSize({216.f, contentHeight});

        auto* rows = CCMenu::create();
        rows->setPosition({0.f, 0.f});
        rows->setContentSize({216.f, contentHeight});
        for (std::size_t i = 0; i < profiles.size(); ++i) {
            auto const& profile = profiles[i];
            auto const selected = profile.id == m_selectedProfileId;
            auto const bound = manager.hasActiveProfile() && manager.levelKey() == profile.id;

            auto* root = CCNode::create();
            root->setContentSize({208.f, 33.f});
            char const* bgName = "stage-row.png"_spr;
            if (selected) {
                bgName = "stage-row-selected.png"_spr;
            }
            else if (bound) {
                bgName = "stage-row-complete.png"_spr;
            }
            if (auto* bg = themedSprite(bgName, 208.f, 33.f)) {
                bg->setPosition({104.f, 16.5f});
                root->addChild(bg);
            }
            root->addChild(makeLabel(shortName(profile.name, 23), "bigFont.fnt", .27f, {9.f, 21.f}, selected ? accentColor() : textColor(), {0.f, .5f}));
            root->addChild(makeLabel(
                fmt::format("{}  •  {} StartPos{}", profile.legacy21 ? "2.1" : "2.2", profile.startPosCount, bound ? "  •  BOUND" : ""),
                "chatFont.fnt", .36f, {9.f, 9.f}, bound ? successColor() : mutedColor(), {0.f, .5f}
            ));

            auto* item = CCMenuItemSpriteExtra::create(root, this, menu_selector(ProfileManagerPopup::onSelectProfile));
            item->setTag(static_cast<int>(i));
            item->setPosition({108.f, contentHeight - 18.5f - static_cast<float>(i) * rowHeight});
            rows->addChild(item);
        }
        scroll->m_contentLayer->addChild(rows);
        scroll->scrollToTop();
        list->addChild(scroll, 3);
    }

    auto* details = card({145.f, 151.f}, true);
    details->setPosition({258.f, 35.f});
    details->addChild(makeLabel(
        turkmen ? "PROFILE DECREE" : (udmurt ? "PROFILE DOSSIER" : "PROFILE DETAILS"),
        "bigFont.fnt", .27f, {72.5f, 137.f}, accentColor()
    ));
    m_content->addChild(details);

    auto selectedIt = std::find_if(profiles.begin(), profiles.end(), [this](auto const& p) { return p.id == m_selectedProfileId; });
    if (selectedIt == profiles.end()) {
        details->addChild(makeLabel("Select a profile", "chatFont.fnt", .48f, {72.5f, 87.f}, mutedColor()));
    }
    else {
        auto const profile = *selectedIt;
        auto const boundHere = manager.hasActiveProfile() && manager.levelKey() == profile.id;
        details->addChild(makeLabel(shortName(profile.name, 18), "bigFont.fnt", .31f, {72.5f, 114.f}, textColor()));
        details->addChild(makeLabel(profile.legacy21 ? "2.1 STARTPOS" : "2.2 STARTPOS", "chatFont.fnt", .42f, {72.5f, 94.f}, baconColor()));
        details->addChild(makeLabel(fmt::format("{} markers", profile.startPosCount), "chatFont.fnt", .42f, {72.5f, 78.f}, mutedColor()));
        details->addChild(makeLabel(boundHere ? "BOUND TO THIS LEVEL" : "READY TO BIND", "bigFont.fnt", .22f, {72.5f, 61.f}, boundHere ? successColor() : mutedColor()));

        auto* actions = CCMenu::create();
        actions->setPosition({0.f, 0.f});
        if (manager.loaded()) {
            auto const bindSelector = boundHere
                ? menu_selector(ProfileManagerPopup::onUnbindSelected)
                : menu_selector(ProfileManagerPopup::onBindSelected);
            auto* bind = textButton(
                boundHere ? "UNBIND" : "BIND",
                this,
                bindSelector,
                111.f,
                boundHere ? baconColor() : successColor()
            );
            bind->setPosition({72.5f, 39.f});
            actions->addChild(bind);
        }
        auto* del = textButton("DELETE", this, menu_selector(ProfileManagerPopup::onDeleteSelected), 111.f, dangerColor());
        del->setPosition({72.5f, 10.f});
        actions->addChild(del);
        details->addChild(actions, 5);
    }

    auto* bottom = CCMenu::create();
    bottom->setPosition({0.f, 0.f});
    if (manager.loaded() && manager.hasScannableStartPositions()) {
        auto* create = textButton(
            turkmen ? "CREATE STATE PROFILE" : (udmurt ? "CREATE UDMURT PROFILE" : "CREATE PROFILE"),
            this, menu_selector(ProfileManagerPopup::onCreateProfile), 156.f, accentColor()
        );
        create->setPosition({95.f, 18.f});
        bottom->addChild(create);
    }
    auto const legacyCount = manager.legacySessions().size();
    if (legacyCount > 0) {
        auto* legacy = textButton(
            turkmen ? fmt::format("OLD PLAN ({})", legacyCount)
                : (udmurt ? fmt::format("OLD ARCHIVE ({})", legacyCount) : fmt::format("LEGACY ({})", legacyCount)),
            this,
            menu_selector(ProfileManagerPopup::onLegacyImport),
            126.f,
            baconColor()
        );
        legacy->setPosition({330.f, 18.f});
        bottom->addChild(legacy);
    }
    m_content->addChild(bottom, 6);
}

void ProfileManagerPopup::onSelectProfile(CCObject* sender) {
    if (!sender) return;
    auto const profiles = baconsistent::TrainingManager::get().profiles();
    auto const index = static_cast<std::size_t>(std::max(0, static_cast<CCNode*>(sender)->getTag()));
    if (index >= profiles.size()) return;
    m_selectedProfileId = profiles[index].id;
    rebuild();
}

void ProfileManagerPopup::onBindSelected(CCObject*) {
    if (m_selectedProfileId.empty()) return;
    auto& manager = baconsistent::TrainingManager::get();
    if (!manager.bindCurrentLevelToProfile(m_selectedProfileId)) {
        FLAlertLayer::create("Bind failed", "This profile could not be loaded or the level is unavailable.", "OK")->show();
        return;
    }
    rebuild();
}

void ProfileManagerPopup::onUnbindSelected(CCObject*) {
    auto& manager = baconsistent::TrainingManager::get();
    if (!manager.hasActiveProfile()) return;
    auto const name = manager.currentProfileName();
    createQuickPopup(
        "Unbind profile",
        fmt::format("Stop using <cy>{}</c> on this level? The profile and all progress are kept.", name),
        "Cancel", "Unbind",
        [this](auto, bool second) {
            if (!second) return;
            baconsistent::TrainingManager::get().unbindCurrentLevel();
            rebuild();
        }
    );
}

void ProfileManagerPopup::onDeleteSelected(CCObject*) {
    if (m_selectedProfileId.empty()) return;
    auto const profiles = baconsistent::TrainingManager::get().profiles();
    auto it = std::find_if(profiles.begin(), profiles.end(), [this](auto const& p) { return p.id == m_selectedProfileId; });
    if (it == profiles.end()) return;
    auto const id = it->id;
    auto const name = it->name;
    createQuickPopup(
        "Delete profile",
        fmt::format("Delete <cr>{}</c>? Repetitions, rounds and statistics for this profile will be permanently removed.", name),
        "Cancel", "Delete",
        [this, id](auto, bool second) {
            if (!second) return;
            if (!baconsistent::TrainingManager::get().deleteProfile(id)) {
                FLAlertLayer::create("Delete failed", "The profile could not be deleted.", "OK")->show();
                return;
            }
            m_selectedProfileId.clear();
            rebuild();
        }
    );
}

void ProfileManagerPopup::onCreateProfile(CCObject*) {
    auto& manager = baconsistent::TrainingManager::get();
    if (!manager.hasScannableStartPositions()) {
        FLAlertLayer::create("No StartPos", "Open a StartPos copy first. Baconsistent only creates profiles from detected StartPos objects.", "OK")->show();
        return;
    }
    if (auto* popup = ProfileCreatePopup::create([this]() { rebuild(); })) {
        popup->show();
    }
}

void ProfileManagerPopup::onLegacyImport(CCObject*) {
    if (auto* popup = LegacyImportPopup::create([this]() { rebuild(); })) {
        popup->show();
    }
}
