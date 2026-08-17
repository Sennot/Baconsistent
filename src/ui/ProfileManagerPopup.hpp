#pragma once

#include <Geode/Geode.hpp>

#include <string>

class ProfileManagerPopup : public geode::Popup {
protected:
    bool init() override;

    void rebuild();
    void onSelectProfile(cocos2d::CCObject* sender);
    void onBindSelected(cocos2d::CCObject* sender);
    void onUnbindSelected(cocos2d::CCObject* sender);
    void onDeleteSelected(cocos2d::CCObject* sender);
    void onCreateProfile(cocos2d::CCObject* sender);
    void onLegacyImport(cocos2d::CCObject* sender);

    std::string m_selectedProfileId;
    cocos2d::CCNode* m_content = nullptr;

public:
    static ProfileManagerPopup* create();
};
