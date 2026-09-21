#pragma once
#include <Geode/Geode.hpp>
#include <Geode/loader/Event.hpp>
#include <vector>

#include "StageRangeCell.hpp"
#include "../../../../events/StageSwitchedEvent.hpp"
#include "../../../../events/StagesChangedEvent.hpp"
#include "../../../../events/StageRangesChangedEvent.hpp"
#include "../../../../events/UpdateScrollLayoutEvent.hpp"

#include "../../../../ui/RectNode.hpp"
#include "../../../../utils/getFirstUncheckedStage.hpp"
#include "../../../../utils/getMetaInfoFromStages.hpp"
#include "../../../../serialization/profile/index.hpp"
#include "../../../../store/GlobalStore.hpp"

using namespace geode::prelude;

enum StageListSortBy
{
  ASC,
  DESC
};

class StageListLayer : public CCLayer
{
private:
  CCSize m_contentSize;
  CCSprite *m_lockSpr = nullptr;
  ScrollLayer *m_scroll = nullptr;
  CCLayer *m_content = nullptr;

  CCMenu *m_buttonMenuLeft = nullptr;
  CCMenu *m_buttonMenuRight = nullptr;
  CCMenuItemSpriteExtra *m_buttonLeft = nullptr;
  CCMenuItemSpriteExtra *m_buttonRight = nullptr;

  // EventListener<EventFilter<StagesChangedEvent>>
  ListenerHandle m_listener;
  // EventListener<EventFilter<UpdateScrollLayoutEvent>>
  ListenerHandle m_listenerUpdateScrollLayout;
  ListenerHandle m_listenerStageRangesChanged;

  GJGameLevel *m_level = nullptr;
  Profile *m_profile = nullptr;
  std::vector<Stage> *m_stages = nullptr;
  Stage *m_stage = nullptr;
  Stage *m_uncheckedStage = nullptr;
  int m_currentIndex = 0;

  // Sort / Filters
  StageListSortBy m_sortBy = StageListSortBy::ASC;
  bool m_hideCompletedRuns = false;

  void onRefreshScheduled(float);
  void onPrevStage();
  void onNextStage();
  void onPrevStageBtn(CCObject *);
  void onNextStageBtn(CCObject *);

public:
  static StageListLayer *create(Stage *stage, GJGameLevel *level, const CCSize &contentSize);
  bool init(Stage *stage, GJGameLevel *level, const CCSize &contentSize);

  void reload();
  void drawArrows();
  void setSortBy(StageListSortBy);
  void setRunsVisabilityForCompleted(bool);

  void scrollToTop();

  ScrollLayer *getScrollLayer() const { return m_scroll; }
};
