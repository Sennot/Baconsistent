#include "../runtime/TrainingManager.hpp"
#include "../ui/PauseProgressBar.hpp"
#include "../ui/Theme.hpp"
#include "../ui/TrainingPopup.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>

#include <algorithm>
#include <string>

using namespace geode::prelude;

class $modify(BaconsistentPauseLayer, PauseLayer) {
    struct Fields {
        bool practiceBarCaptured = false;
        bool baconBarApplied = false;
        std::string originalTitle;
        std::string originalValue;
        cocos2d::CCRect originalFillRect;
        cocos2d::ccColor3B originalFillColor{255, 255, 255};
        float originalFillWidth = 0.f;
    };

    void capturePracticeBar() {
        if (m_fields->practiceBarCaptured) {
            return;
        }

        auto* bar = this->getChildByID("practice-progress-bar");
        auto* title = typeinfo_cast<CCLabelBMFont*>(this->getChildByID("practice-mode-label"));
        auto* value = typeinfo_cast<CCLabelBMFont*>(this->getChildByID("practice-progress-label"));
        if (!bar || !title || !value || !bar->getChildren() || bar->getChildrenCount() == 0) {
            return;
        }

        auto* fill = typeinfo_cast<CCSprite*>(bar->getChildren()->objectAtIndex(0));
        if (!fill) {
            return;
        }

        m_fields->originalTitle = title->getString();
        m_fields->originalValue = value->getString();
        m_fields->originalFillRect = fill->getTextureRect();
        m_fields->originalFillColor = fill->getColor();
        m_fields->originalFillWidth = fill->getContentWidth();
        m_fields->practiceBarCaptured = true;
    }

    void restorePracticeBar() {
        if (!m_fields->practiceBarCaptured || !m_fields->baconBarApplied) {
            return;
        }

        auto* bar = this->getChildByID("practice-progress-bar");
        auto* title = typeinfo_cast<CCLabelBMFont*>(this->getChildByID("practice-mode-label"));
        auto* value = typeinfo_cast<CCLabelBMFont*>(this->getChildByID("practice-progress-label"));
        if (!bar || !title || !value || !bar->getChildren() || bar->getChildrenCount() == 0) {
            return;
        }

        auto* fill = typeinfo_cast<CCSprite*>(bar->getChildren()->objectAtIndex(0));
        if (!fill) {
            return;
        }

        title->setString(m_fields->originalTitle.c_str());
        value->setString(m_fields->originalValue.c_str());
        fill->setContentWidth(m_fields->originalFillWidth);
        fill->setTextureRect(m_fields->originalFillRect);
        fill->setColor(m_fields->originalFillColor);
        m_fields->baconBarApplied = false;
    }

    void customSetup() {
        PauseLayer::customSetup();

        // The StartPos counter lives entirely in PauseLayer by reusing GD's
        // existing Practice Mode progress area. Refresh it while paused so
        // external StartPos switchers can change m_startPosObject live.
        capturePracticeBar();
        updateBaconProgress(0.f);
        this->schedule(schedule_selector(BaconsistentPauseLayer::updateBaconProgress), 0.10f);

        // Do not use absolute pause coordinates. Pause menus are a shared UI
        // surface and other mods routinely add buttons here. A compact branded
        // icon participates in Geode's existing menu layout instead.
        auto leftMenu = this->getChildByID("left-button-menu");
        if (!leftMenu) {
            log::warn("Baconsistent: left-button-menu not found; skipping pause button to avoid UI overlap");
            return;
        }

        auto const logoPath = baconsistent::ui::theme::pauseIconResource();
        auto logo = CCSprite::create(logoPath.c_str());
        if (!logo) {
            log::error("Baconsistent: pause-icon.png failed to load");
            return;
        }

        // The source art is intentionally high resolution, but the
        // shared PauseLayer menu expects compact items. Give Geode a real 40x40
        // layout item instead of scaling a huge menu item after insertion.
        auto iconRoot = CCNode::create();
        iconRoot->setContentSize({40.f, 40.f});
        auto const logoSize = logo->getContentSize();
        if (logoSize.width > 0.f && logoSize.height > 0.f) {
            logo->setScale(std::min(36.f / logoSize.width, 36.f / logoSize.height));
        }
        logo->setPosition({20.f, 20.f});
        iconRoot->addChild(logo);

        auto button = CCMenuItemSpriteExtra::create(
            iconRoot,
            this,
            menu_selector(BaconsistentPauseLayer::onBaconsistent)
        );
        button->m_baseScale = 1.f;
        button->setID("pause-button"_spr);
        leftMenu->addChild(button);
        leftMenu->updateLayout();
    }

    void updateBaconProgress(float) {
        if (baconsistent::ui::updatePauseProgressBar(this)) {
            m_fields->baconBarApplied = true;
        }
        else {
            // If the current level becomes unbound (or the feature is disabled)
            // while this PauseLayer is still open, restore GD's original
            // Practice UI instead of leaving a stale Baconsistent bar behind.
            restorePracticeBar();
        }
    }

    void onBaconsistent(CCObject*) {
        if (!baconsistent::TrainingManager::get().loaded()) {
            FLAlertLayer::create(
                "Baconsistent",
                "Baconsistent supports classic percentage-based levels. Platformer mode is not supported yet.",
                "OK"
            )->show();
            return;
        }

        if (auto popup = TrainingPopup::create(this)) {
            popup->show();
        }
    }
};
