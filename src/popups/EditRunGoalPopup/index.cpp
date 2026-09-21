#include "index.hpp"

EditRunGoalPopup* EditRunGoalPopup::create(Profile const& profile, Range const& range)
{
    auto ret = new EditRunGoalPopup();
    if (ret->init(profile, range))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool EditRunGoalPopup::init(Profile const& profile, Range const& range)
{
    if (!Popup::init(260, 185, "GJ_square01_custom.png"_spr))
        return false;
    m_profileId = profile.id;
    m_rangeId = range.id;
    m_profileGoal = profile.requiredPasses;
    setTitle("Run Target");

    auto run = CCLabelBMFont::create(fmt::format("{:.2f}% - {:.2f}%", range.from, range.to).c_str(), "bigFont.fnt");
    run->setScale(.45f);
    run->setPosition({130.f, 139.f});
    m_mainLayer->addChild(run);

    m_input = TextInput::create(85.f, "20", "bigFont.fnt");
    m_input->setFilter("0123456789");
    m_input->setMaxCharCount(4);
    m_input->setString(std::to_string(bacon::passGoal(profile, range)));
    m_input->setPosition({130.f, 102.f});
    m_mainLayer->addChild(m_input);

    auto hint = CCLabelBMFont::create(fmt::format("Completed: {} | Default: {}", range.completionCounter, m_profileGoal).c_str(), "bigFont.fnt");
    hint->limitLabelWidth(234.f, .32f, .2f);
    hint->setPosition({130.f, 66.f});
    m_mainLayer->addChild(hint);

    auto menu = CCMenu::create();
    menu->setPosition({0.f, 0.f});
    for (int delta : {-1, 1})
    {
        auto sprite = ButtonSprite::create(delta < 0 ? "-" : "+", 0, 0,
            "goldFont.fnt", "GJ_button_01.png", 0.f, .8f);
        sprite->setScale(.65f);
        auto button = CCMenuItemSpriteExtra::create(sprite, this, menu_selector(EditRunGoalPopup::onStep));
        button->setTag(delta);
        button->setPosition({130.f + 70.f * delta, 102.f});
        menu->addChild(button);
    }
    auto defaultSprite = ButtonSprite::create("Default", 0, 0, "goldFont.fnt", "GJ_button_01.png", 0.f, .8f);
    defaultSprite->setScale(.65f);
    auto defaultButton = CCMenuItemSpriteExtra::create(defaultSprite, this, menu_selector(EditRunGoalPopup::onDefault));
    defaultButton->setPosition({78.f, 27.f});
    menu->addChild(defaultButton);
    auto saveSprite = ButtonSprite::create("Save", 0, 0, "goldFont.fnt", "GJ_button_01.png", 0.f, .8f);
    saveSprite->setScale(.65f);
    auto save = CCMenuItemSpriteExtra::create(saveSprite, this, menu_selector(EditRunGoalPopup::onSave));
    save->setPosition({181.f, 27.f});
    menu->addChild(save);
    m_mainLayer->addChild(menu);
    return true;
}

void EditRunGoalPopup::onStep(CCObject* sender)
{
    auto button = typeinfo_cast<CCMenuItemSpriteExtra*>(sender);
    if (!button)
        return;
    int target = utils::numFromString<int>(m_input->getString()).unwrapOr(m_profileGoal);
    m_input->setString(std::to_string(std::clamp(target + button->getTag(), 1, bacon::maxPasses)));
}

void EditRunGoalPopup::onDefault(CCObject*)
{
    m_input->setString(std::to_string(m_profileGoal));
}

void EditRunGoalPopup::onSave(CCObject*)
{
    const int target = utils::numFromString<int>(m_input->getString()).unwrapOr(0);
    if (target < 1 || target > bacon::maxPasses)
    {
        FLAlertLayer::create("Run Target", "Enter a number from 1 to 9999.", "OK")->show();
        return;
    }
    auto profile = GlobalStore::get()->getProfileById(m_profileId);
    if (!profile)
        return;
    for (auto& stage : profile->data.stages)
        for (auto& range : stage.ranges)
            if (range.id == m_rangeId)
            {
                bacon::setRunGoal(*profile, range, target == profile->requiredPasses ? 0 : target);
                GlobalStore::get()->updateProfile(*profile);
                this->onClose(nullptr);
                StageRangesChangedEvent().send();
                return;
            }
}
