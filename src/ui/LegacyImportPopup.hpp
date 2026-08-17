#pragma once

#include <Geode/Geode.hpp>

#include <functional>
#include <string>

class LegacyImportPopup : public geode::Popup {
protected:
    bool init(std::function<void()> onChanged);

    void rebuild();
    void onSelect(cocos2d::CCObject* sender);
    void onToggle(cocos2d::CCObject* sender);
    void onImport(cocos2d::CCObject* sender);

    std::function<void()> m_onChanged;
    std::string m_selectedLegacyKey;
    cocos2d::CCNode* m_content = nullptr;
    CCMenuItemToggler* m_legacyToggle = nullptr;

public:
    static LegacyImportPopup* create(std::function<void()> onChanged);
};
