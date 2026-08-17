#include "LegacyImportPopup.hpp"

#include "Theme.hpp"
#include "../runtime/TrainingManager.hpp"

#include <Geode/ui/ScrollLayer.hpp>

#include <algorithm>
#include <utility>

using namespace geode::prelude;

namespace {
ccColor3B textColor() { return baconsistent::ui::theme::palette().text; }
ccColor3B mutedColor() { return baconsistent::ui::theme::palette().muted; }
ccColor3B accentColor() { return baconsistent::ui::theme::palette().accent; }
ccColor3B successColor() { return baconsistent::ui::theme::palette().success; }
ccColor3B baconColor() { return baconsistent::ui::theme::palette().bacon; }

CCLabelBMFont* label(
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
    if (!sprite) return nullptr;
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

CCMenuItemSpriteExtra* button(
    std::string const& title,
    CCObject* target,
    SEL_MenuHandler selector,
    float width,
    ccColor3B color
) {
    auto* root = CCNode::create();
    root->setContentSize({width, 29.f});
    if (auto* bg = themedSprite("tab-pill-active.png"_spr, width, 29.f)) {
        bg->setPosition({width / 2.f, 14.5f});
        root->addChild(bg);
    }
    root->addChild(label(title, "bigFont.fnt", .29f, {width / 2.f, 14.5f}, color));
    return CCMenuItemSpriteExtra::create(root, target, selector);
}

std::string shortName(std::string value, std::size_t limit) {
    if (value.size() <= limit) return value;
    if (limit <= 3) return value.substr(0, limit);
    return value.substr(0, limit - 3) + "...";
}
} // namespace

LegacyImportPopup* LegacyImportPopup::create(std::function<void()> onChanged) {
    auto* ret = new LegacyImportPopup();
    if (ret->init(std::move(onChanged))) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool LegacyImportPopup::init(std::function<void()> onChanged) {
    if (!Popup::init(400.f, 252.f)) {
        return false;
    }
    setID("legacy-import-popup"_spr);
    m_onChanged = std::move(onChanged);

    if (m_bgSprite && (baconsistent::ui::theme::preset() != baconsistent::ui::theme::Preset::Original || baconsistent::ui::theme::customColorsEnabled())) {
        m_bgSprite->setColor(baconsistent::ui::theme::palette().surface);
    }

    m_content = CCNode::create();
    m_content->setContentSize({400.f, 252.f});
    m_mainLayer->addChild(m_content);

    auto const sessions = baconsistent::TrainingManager::get().legacySessions();
    if (!sessions.empty()) {
        m_selectedLegacyKey = sessions.front().legacyKey;
    }
    rebuild();
    return true;
}

void LegacyImportPopup::rebuild() {
    if (!m_content) return;
    m_content->removeAllChildrenWithCleanup(true);
    m_legacyToggle = nullptr;

    auto const sessions = baconsistent::TrainingManager::get().legacySessions();
    auto selected = std::find_if(sessions.begin(), sessions.end(), [this](auto const& item) {
        return item.legacyKey == m_selectedLegacyKey;
    });
    if (selected == sessions.end() && !sessions.empty()) {
        m_selectedLegacyKey = sessions.front().legacyKey;
        selected = sessions.begin();
    }

    auto const turkmen = baconsistent::ui::theme::isTurkmenistan();
    auto const udmurt = baconsistent::ui::theme::isUdmurtia();
    auto const tatar = baconsistent::ui::theme::isTatarstan();
    auto const recoveryTitle = turkmen ? "ARCHIVE OF PREVIOUS FIVE-YEAR PLAN"
        : (udmurt ? "UDMURT LEGACY ARCHIVE" : (tatar ? "TATAR LEGACY ARCHIVE" : "LEGACY SESSION RECOVERY"));
    m_content->addChild(label(
        recoveryTitle,
        "bigFont.fnt", turkmen ? .31f : (udmurt || tatar ? .39f : .43f), {200.f, 230.f}, accentColor()
    ));

    if (turkmen || udmurt || tatar) {
        auto flagPath = Mod::get()->expandSpriteName(udmurt ? "ud-flag.png" : (tatar ? "tt-flag.png" : "tm-flag.png"));
        if (auto* flag = CCSprite::create(flagPath.c_str())) {
            auto const s = flag->getContentSize();
            if (s.width > 0.f && s.height > 0.f) flag->setScale(std::min(28.f / s.width, 18.f / s.height));
            flag->setPosition({32.f, 230.f});
            m_content->addChild(flag, 5);
        }
    }

    auto* list = card({364.f, 126.f}, false);
    list->setPosition({18.f, 82.f});
    m_content->addChild(list);

    if (sessions.empty()) {
        auto* empty = label(
            "No pre-profile Baconsistent sessions found.\nLegacy data is never deleted by this recovery tool.",
            "chatFont.fnt", .48f, {182.f, 63.f}, mutedColor()
        );
        empty->setAlignment(kCCTextAlignmentCenter);
        list->addChild(empty);
    }
    else {
        auto* scroll = ScrollLayer::create(CCSize{348.f, 110.f}, true, true);
        scroll->setPosition({8.f, 8.f});
        scroll->setCancelTouchLimit(5.f);
        float const rowHeight = 39.f;
        auto const contentHeight = std::max(110.f, static_cast<float>(sessions.size()) * rowHeight);
        scroll->m_contentLayer->setContentSize({348.f, contentHeight});

        auto* rows = CCMenu::create();
        rows->setPosition({0.f, 0.f});
        rows->setContentSize({348.f, contentHeight});
        for (std::size_t i = 0; i < sessions.size(); ++i) {
            auto const& session = sessions[i];
            auto const isSelected = session.legacyKey == m_selectedLegacyKey;
            auto* root = CCNode::create();
            root->setContentSize({340.f, 35.f});
            if (auto* bg = themedSprite(isSelected ? "stage-row-selected.png"_spr : "stage-row.png"_spr, 340.f, 35.f)) {
                bg->setPosition({170.f, 17.5f});
                root->addChild(bg);
            }
            root->addChild(label(shortName(session.name, 30), "bigFont.fnt", .28f, {10.f, 23.f}, isSelected ? accentColor() : textColor(), {0.f, .5f}));
            root->addChild(label(
                fmt::format("{}  •  {} StartPos  •  Round {}{}", session.legacyKey, session.startPosCount, session.round, session.alreadyImported ? "  •  IMPORTED" : ""),
                "chatFont.fnt", .34f, {10.f, 10.f}, session.alreadyImported ? successColor() : mutedColor(), {0.f, .5f}
            ));
            auto* item = CCMenuItemSpriteExtra::create(root, this, menu_selector(LegacyImportPopup::onSelect));
            item->setTag(static_cast<int>(i));
            item->setPosition({174.f, contentHeight - 19.5f - static_cast<float>(i) * rowHeight});
            rows->addChild(item);
        }
        scroll->m_contentLayer->addChild(rows);
        scroll->scrollToTop();
        list->addChild(scroll, 2);
    }

    if (selected != sessions.end()) {
        m_content->addChild(label(
            selected->alreadyImported ? "A recovered profile already exists. You can import another copy if needed." : "Choose the percentage mode for the recovered profile.",
            "chatFont.fnt", .38f, {200.f, 68.f}, selected->alreadyImported ? successColor() : mutedColor()
        ));

        auto* toggleMenu = CCMenu::create();
        toggleMenu->setPosition({0.f, 0.f});
        m_legacyToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(LegacyImportPopup::onToggle), .60f);
        m_legacyToggle->setPosition({86.f, 45.f});
        m_legacyToggle->toggle(true);
        toggleMenu->addChild(m_legacyToggle);
        m_content->addChild(toggleMenu, 4);
        m_content->addChild(label("2.1 percentages", "bigFont.fnt", .27f, {104.f, 45.f}, textColor(), {0.f, .5f}));

        auto* actions = CCMenu::create();
        actions->setPosition({0.f, 0.f});
        auto* import = button(selected->alreadyImported ? "IMPORT AGAIN" : "IMPORT", this, menu_selector(LegacyImportPopup::onImport), 122.f, baconColor());
        import->setPosition({310.f, 43.f});
        actions->addChild(import);
        m_content->addChild(actions, 4);
    }

    m_content->addChild(label(
        "Import copies old progress; the original legacy savedata is kept untouched.",
        "chatFont.fnt", .35f, {200.f, 18.f}, mutedColor()
    ));
}

void LegacyImportPopup::onSelect(CCObject* sender) {
    if (!sender) return;
    auto const sessions = baconsistent::TrainingManager::get().legacySessions();
    auto const index = static_cast<std::size_t>(std::max(0, static_cast<CCNode*>(sender)->getTag()));
    if (index >= sessions.size()) return;
    m_selectedLegacyKey = sessions[index].legacyKey;
    rebuild();
}

void LegacyImportPopup::onToggle(CCObject*) {
    // CCMenuItemToggler updates itself before this callback.
}

void LegacyImportPopup::onImport(CCObject*) {
    if (m_selectedLegacyKey.empty()) return;
    auto const sessions = baconsistent::TrainingManager::get().legacySessions();
    auto it = std::find_if(sessions.begin(), sessions.end(), [this](auto const& item) {
        return item.legacyKey == m_selectedLegacyKey;
    });
    if (it == sessions.end()) return;

    auto imported = baconsistent::TrainingManager::get().importLegacySession(
        it->legacyKey,
        it->name,
        m_legacyToggle && m_legacyToggle->isOn()
    );
    if (!imported) {
        FLAlertLayer::create(
            "Import failed",
            "The legacy session is incomplete or its StartPos layout is invalid.",
            "OK"
        )->show();
        return;
    }

    if (m_onChanged) m_onChanged();
    FLAlertLayer::create(
        "Legacy session recovered",
        fmt::format("<cg>{}</c> was imported as a new profile. The old savedata was left untouched.", it->name),
        "OK"
    )->show();
    onClose(nullptr);
}
