#pragma once

#include <Geode/Geode.hpp>

#include <array>

class TrainingPopup : public geode::Popup {
protected:
    bool init() override;
    void onClose(cocos2d::CCObject* sender) override;

    void rebuildTabs();
    void rebuildPage();
    void buildStagesPage();
    void buildStatsPage();
    void buildRoundsPage();
    void buildProfilesPage();
    void buildUnavailablePage();

    void onTab(cocos2d::CCObject* sender);
    void onStage(cocos2d::CCObject* sender);
    void onRecommended(cocos2d::CCObject* sender);
    void onResetPart(cocos2d::CCObject* sender);
    void onResetRound(cocos2d::CCObject* sender);
    void onTargetMinus(cocos2d::CCObject* sender);
    void onTargetPlus(cocos2d::CCObject* sender);
    void onTargetReset(cocos2d::CCObject* sender);
    void onRepMinus(cocos2d::CCObject* sender);
    void onRepPlus(cocos2d::CCObject* sender);
    void onCompleteStage(cocos2d::CCObject* sender);
    void onSettings(cocos2d::CCObject* sender);
    void onProfilesTab(cocos2d::CCObject* sender);
    void onOpenProfileManager(cocos2d::CCObject* sender);
    void onStageEditor(cocos2d::CCObject* sender);

    cocos2d::CCNode* m_pageRoot = nullptr;
    cocos2d::CCMenu* m_tabMenu = nullptr;
    int m_activePage = 0;
    PauseLayer* m_pauseHost = nullptr;

public:
    static TrainingPopup* create(PauseLayer* host = nullptr);
};
