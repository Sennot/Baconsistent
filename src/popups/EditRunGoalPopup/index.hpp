#pragma once
#include <Geode/Geode.hpp>
#include "../../store/GlobalStore.hpp"
#include "../../events/StageRangesChangedEvent.hpp"
using namespace geode::prelude;

class EditRunGoalPopup : public geode::Popup
{
    std::string m_profileId;
    std::string m_rangeId;
    TextInput* m_input = nullptr;
    int m_profileGoal = 20;
    bool init(Profile const& profile, Range const& range);
    void onStep(CCObject* sender);
    void onDefault(CCObject*);
    void onSave(CCObject*);
public:
    static EditRunGoalPopup* create(Profile const& profile, Range const& range);
};
