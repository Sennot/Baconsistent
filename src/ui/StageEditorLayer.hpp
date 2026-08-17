#pragma once

#include "../core/StageLayoutDraft.hpp"

#include <Geode/Geode.hpp>

#include <vector>

namespace baconsistent::ui {

class StageEditorLayer : public cocos2d::CCLayer {
public:
    static StageEditorLayer* create(PauseLayer* host);

protected:
    bool init(PauseLayer* host);
    bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void keyBackClicked() override;

private:
    enum class DragMode { None, Marker, Pan };

    void hidePauseUI();
    void restorePauseUI();
    void buildUI();
    void refreshVisuals();
    void refreshInfo();
    void focusSelected();
    void restoreWorldPosition();
    void closeEditor(bool save);
    void setSelectedMarker(int index, bool focus);
    [[nodiscard]] cocos2d::CCNode* findWorldRoot(PlayLayer* layer) const;
    [[nodiscard]] float levelXForMarker(int markerIndex) const;
    [[nodiscard]] double modernPercentForX(float x) const;
    [[nodiscard]] float screenXForLevelX(float x) const;
    [[nodiscard]] float levelXForScreenPoint(cocos2d::CCPoint screenPoint) const;
    [[nodiscard]] int nearestVisibleMarker(float screenX, float maxDistance) const;
    void panWorld(float screenDeltaX);

    void onSave(cocos2d::CCObject*);
    void onCancel(cocos2d::CCObject*);
    void onReset(cocos2d::CCObject*);
    void onPrevious(cocos2d::CCObject*);
    void onNext(cocos2d::CCObject*);
    void onFocus(cocos2d::CCObject*);

    PauseLayer* m_host = nullptr;
    PlayLayer* m_playLayer = nullptr;
    cocos2d::CCNode* m_worldRoot = nullptr;
    cocos2d::CCPoint m_originalWorldPosition{};
    bool m_haveOriginalWorldPosition = false;
    std::vector<cocos2d::CCNode*> m_hiddenPauseNodes;

    baconsistent::core::StageLayoutDraft m_draft;
    int m_selectedMarker = 0;
    DragMode m_dragMode = DragMode::None;
    cocos2d::CCPoint m_lastTouch{};

    cocos2d::CCDrawNode* m_draw = nullptr;
    cocos2d::CCNode* m_markerLabels = nullptr;
    cocos2d::CCLabelBMFont* m_titleLabel = nullptr;
    cocos2d::CCLabelBMFont* m_percentLabel = nullptr;
    cocos2d::CCLabelBMFont* m_stageLabel = nullptr;
    cocos2d::CCLabelBMFont* m_helpLabel = nullptr;
};

} // namespace baconsistent::ui
