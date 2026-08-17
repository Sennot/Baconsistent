#include "ProfileCreatePopup.hpp"

#include "Theme.hpp"
#include "../runtime/TrainingManager.hpp"

#include <algorithm>
#include <utility>

using namespace geode::prelude;

namespace {
CCLabelBMFont* makeLabel(
    std::string const& text,
    float scale,
    CCPoint position,
    ccColor3B color,
    CCPoint anchor = {.5f, .5f}
) {
    auto* node = CCLabelBMFont::create(text.c_str(), "bigFont.fnt");
    node->setScale(scale);
    node->setPosition(position);
    node->setColor(color);
    node->setAnchorPoint(anchor);
    return node;
}

CCSprite* cardSprite(char const* expanded, float width, float height, bool dark = false) {
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
}

ProfileCreatePopup* ProfileCreatePopup::create(std::function<void()> onChanged) {
    auto* ret = new ProfileCreatePopup();
    if (ret->init(std::move(onChanged))) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ProfileCreatePopup::init(std::function<void()> onChanged) {
    if (!Popup::init(350.f, 215.f)) {
        return false;
    }
    if (m_bgSprite && (baconsistent::ui::theme::preset() != baconsistent::ui::theme::Preset::Original || baconsistent::ui::theme::customColorsEnabled())) {
        m_bgSprite->setColor(baconsistent::ui::theme::palette().surface);
    }
    m_onChanged = std::move(onChanged);
    setID("profile-create-popup"_spr);

    auto const colors = baconsistent::ui::theme::palette();
    m_mainLayer->addChild(makeLabel("CREATE STARTPOS PROFILE", .46f, {175.f, 190.f}, colors.accent));
    if (baconsistent::ui::theme::isTurkmenistan() || baconsistent::ui::theme::isUdmurtia() || baconsistent::ui::theme::isTatarstan()) {
        auto flagPath = Mod::get()->expandSpriteName(
            baconsistent::ui::theme::isUdmurtia() ? "ud-flag.png" : (baconsistent::ui::theme::isTatarstan() ? "tt-flag.png" : "tm-flag.png")
        );
        if (auto* flag = CCSprite::create(flagPath.c_str())) {
            auto const size = flag->getContentSize();
            if (size.width > 0.f && size.height > 0.f) {
                flag->setScale(std::min(30.f / size.width, 18.f / size.height));
            }
            flag->setPosition({41.f, 190.f});
            m_mainLayer->addChild(flag, 4);
        }
    }

    if (auto* card = cardSprite("panel-card-dark.png"_spr, 310.f, 120.f, true)) {
        card->setPosition({175.f, 111.f});
        m_mainLayer->addChild(card);
    }

    auto& manager = baconsistent::TrainingManager::get();
    m_mainLayer->addChild(makeLabel(
        fmt::format("{} detected StartPos", manager.scannableStartPosCount()),
        .30f,
        {175.f, 157.f},
        colors.muted
    ));

    m_nameInput = TextInput::create(232.f, "Profile name", "bigFont.fnt");
    if (!m_nameInput) {
        return false;
    }
    m_nameInput->setPosition({175.f, 132.f});
    m_nameInput->setMaxCharCount(32);
    m_nameInput->setString(manager.currentLevelName().c_str(), false);
    m_mainLayer->addChild(m_nameInput, 3);

    auto* toggles = CCMenu::create();
    toggles->setPosition({0.f, 0.f});
    m_mainLayer->addChild(toggles, 4);

    m_legacyToggle = CCMenuItemToggler::createWithStandardSprites(
        this,
        menu_selector(ProfileCreatePopup::onToggle),
        .62f
    );
    if (!m_legacyToggle) {
        return false;
    }
    m_legacyToggle->setPosition({68.f, 92.f});
    // Match the familiar Blitzkrieg creation flow: legacy 2.1 percentages are
    // preselected, but the user can toggle them off for modern 2.2 timing.
    m_legacyToggle->toggle(true);
    toggles->addChild(m_legacyToggle);
    m_mainLayer->addChild(makeLabel("2.1 percentages", .30f, {88.f, 92.f}, colors.text, {0.f, .5f}));

    m_bindToggle = CCMenuItemToggler::createWithStandardSprites(
        this,
        menu_selector(ProfileCreatePopup::onToggle),
        .62f
    );
    if (!m_bindToggle) {
        return false;
    }
    m_bindToggle->setPosition({68.f, 62.f});
    toggles->addChild(m_bindToggle);
    m_mainLayer->addChild(makeLabel("Bind current level after create", .28f, {88.f, 62.f}, colors.text, {0.f, .5f}));

    if (baconsistent::ui::theme::isTurkmenistan()) {
        m_mainLayer->addChild(makeLabel("ASHGABAT PROFILE MINISTRY", .24f, {175.f, 39.f}, colors.success));
    }
    else if (baconsistent::ui::theme::isUdmurtia()) {
        m_mainLayer->addChild(makeLabel("IZHEVSK PROFILE BUREAU", .24f, {175.f, 39.f}, colors.accent));
    }
    else if (baconsistent::ui::theme::isTatarstan()) {
        m_mainLayer->addChild(makeLabel("KAZAN PROFILE KHANATE", .24f, {175.f, 39.f}, colors.success));
    }

    auto* createRoot = CCNode::create();
    createRoot->setContentSize({100.f, 30.f});
    if (auto* bg = cardSprite("tab-pill-active.png"_spr, 100.f, 30.f, false)) {
        bg->setPosition({50.f, 15.f});
        createRoot->addChild(bg);
    }
    createRoot->addChild(makeLabel("CREATE", .33f, {50.f, 15.f}, colors.accent));
    auto* createButton = CCMenuItemSpriteExtra::create(
        createRoot,
        this,
        menu_selector(ProfileCreatePopup::onCreateProfile)
    );
    createButton->setPosition({175.f, 24.f});
    m_buttonMenu->addChild(createButton);
    return true;
}

void ProfileCreatePopup::onToggle(CCObject*) {
    // CCMenuItemToggler updates its own state before invoking the callback.
}

void ProfileCreatePopup::onCreateProfile(CCObject*) {
    auto& manager = baconsistent::TrainingManager::get();
    if (!manager.hasScannableStartPositions()) {
        FLAlertLayer::create(
            "No StartPos",
            "This level no longer contains StartPos objects. Re-open the StartPos copy and try again.",
            "OK"
        )->show();
        return;
    }

    std::string name = m_nameInput ? std::string(m_nameInput->getString().c_str()) : manager.currentLevelName();
    if (name.empty()) {
        name = manager.currentLevelName();
    }

    auto created = manager.createProfileFromCurrentStartPositions(
        name,
        m_legacyToggle && m_legacyToggle->isOn()
    );
    if (!created) {
        FLAlertLayer::create("Create failed", "Baconsistent could not create this StartPos profile.", "OK")->show();
        return;
    }

    if (m_bindToggle && m_bindToggle->isOn()) {
        (void)manager.bindCurrentLevelToProfile(*created);
    }

    if (m_onChanged) {
        m_onChanged();
    }
    onClose(nullptr);
}
