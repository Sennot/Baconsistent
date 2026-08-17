#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/TextInput.hpp>

#include <functional>

class ProfileCreatePopup : public geode::Popup {
protected:
    bool init(std::function<void()> onChanged);

    void onToggle(cocos2d::CCObject* sender);
    void onCreateProfile(cocos2d::CCObject* sender);

    geode::TextInput* m_nameInput = nullptr;
    CCMenuItemToggler* m_legacyToggle = nullptr;
    CCMenuItemToggler* m_bindToggle = nullptr;
    std::function<void()> m_onChanged;

public:
    static ProfileCreatePopup* create(std::function<void()> onChanged);
};
