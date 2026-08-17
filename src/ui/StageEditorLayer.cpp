#include "StageEditorLayer.hpp"

#include "Theme.hpp"
#include "../core/PercentageMath.hpp"
#include "../runtime/StartPosAnalyzer.hpp"
#include "../runtime/TrainingManager.hpp"

#include <algorithm>
#include <cmath>
#include <string>

using namespace geode::prelude;

namespace baconsistent::ui {
namespace {

ccColor4F color4(ccColor3B c, float a = 1.f) {
    return {
        static_cast<float>(c.r) / 255.f,
        static_cast<float>(c.g) / 255.f,
        static_cast<float>(c.b) / 255.f,
        a,
    };
}

CCLabelBMFont* makeLabel(
    std::string const& text,
    float scale,
    CCPoint position,
    ccColor3B color,
    CCPoint anchor = {.5f, .5f}
) {
    auto* label = CCLabelBMFont::create(text.c_str(), "bigFont.fnt");
    label->setScale(scale);
    label->setPosition(position);
    label->setColor(color);
    label->setAnchorPoint(anchor);
    return label;
}

CCSprite* sizedSprite(char const* file, float width, float height) {
    auto path = theme::resource(file);
    auto* sprite = CCSprite::create(path.c_str());
    if (!sprite) {
        return nullptr;
    }
    auto const size = sprite->getContentSize();
    if (size.width > 0.f && size.height > 0.f) {
        sprite->setScaleX(width / size.width);
        sprite->setScaleY(height / size.height);
    }
    theme::applySurfaceTint(sprite, file, std::string(file).find("dark") != std::string::npos);
    return sprite;
}

CCMenuItemSpriteExtra* textButton(
    std::string const& text,
    CCObject* target,
    SEL_MenuHandler selector,
    float width,
    bool active = false
) {
    auto const colors = theme::palette();
    auto* root = CCNode::create();
    root->setContentSize({width, 29.f});
    if (auto* bg = sizedSprite(active ? "tab-pill-active.png"_spr : "tab-pill.png"_spr, width, 29.f)) {
        bg->setPosition({width / 2.f, 14.5f});
        root->addChild(bg);
    }
    root->addChild(makeLabel(text, .28f, {width / 2.f, 14.5f}, active ? colors.accent : colors.text));
    return CCMenuItemSpriteExtra::create(root, target, selector);
}

std::string percentText(double value) {
    auto const rounded = std::round(value);
    if (std::abs(value - rounded) < .05) {
        return fmt::format("{:.0f}%", rounded);
    }
    return fmt::format("{:.1f}%", value);
}

std::string editorTitle() {
    if (theme::isTurkmenistan()) return "ASHGABAT STAGE EDITOR";
    if (theme::isUdmurtia()) return "IZHEVSK STAGE EDITOR";
    if (theme::isTatarstan()) return "KAZAN STAGE EDITOR";
    return "STAGE LAYOUT EDITOR";
}

} // namespace

StageEditorLayer* StageEditorLayer::create(PauseLayer* host) {
    auto* ret = new StageEditorLayer();
    if (ret->init(host)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool StageEditorLayer::init(PauseLayer* host) {
    if (!CCLayer::init() || !host) {
        return false;
    }

    auto& manager = TrainingManager::get();
    auto* play = PlayLayer::get();
    if (!play || !manager.hasActiveProfile() || manager.profileBoundaries21().size() < 3 ||
        manager.profileBoundaries21().size() != manager.profileBoundaries22().size() || play->m_levelLength <= 0.f) {
        return false;
    }

    m_host = host;
    m_playLayer = play;
    m_worldRoot = findWorldRoot(play);
    if (!m_worldRoot) {
        return false;
    }

    m_draft = core::StageLayoutDraft(manager.profileBoundaries21(), manager.profileBoundaries22());
    if (!m_draft.valid()) {
        return false;
    }

    m_selectedMarker = std::clamp(manager.selected(), 0, static_cast<int>(m_draft.markerCount()) - 1);
    m_originalWorldPosition = m_worldRoot->getPosition();
    m_haveOriginalWorldPosition = true;

    auto const win = CCDirector::sharedDirector()->getWinSize();
    setContentSize(win);
    setTouchEnabled(true);
    setTouchMode(cocos2d::kCCTouchesOneByOne);
    setTouchPriority(0);
    setKeypadEnabled(true);
    setID("stage-layout-editor"_spr);

    hidePauseUI();
    buildUI();
    focusSelected();
    refreshVisuals();
    return true;
}

CCNode* StageEditorLayer::findWorldRoot(PlayLayer* layer) const {
    if (!layer) {
        return nullptr;
    }

    CCNode* node = layer->m_player1 ? layer->m_player1->getParent() : nullptr;
    if (!node && layer->m_objects) {
        for (auto value : CCArrayExt<CCObject*>(layer->m_objects)) {
            if (auto* object = typeinfo_cast<GameObject*>(value)) {
                node = object->getParent();
                if (node) break;
            }
        }
    }
    if (!node) {
        return nullptr;
    }

    // The highest node below PlayLayer is the camera-transformed gameplay
    // root. Moving it while PauseLayer is active gives the editor a harmless
    // temporary pan without changing any level data.
    while (node->getParent() && node->getParent() != layer) {
        node = node->getParent();
    }
    return node;
}

void StageEditorLayer::hidePauseUI() {
    if (!m_host || !m_host->getChildren()) {
        return;
    }
    for (auto* node : CCArrayExt<CCNode*>(m_host->getChildren())) {
        if (node && node->isVisible()) {
            m_hiddenPauseNodes.push_back(node);
            node->setVisible(false);
        }
    }
}

void StageEditorLayer::restorePauseUI() {
    for (auto* node : m_hiddenPauseNodes) {
        if (node) node->setVisible(true);
    }
    m_hiddenPauseNodes.clear();
}

void StageEditorLayer::buildUI() {
    auto const win = getContentSize();
    auto const colors = theme::palette();

    // Soft top/bottom glass panels. The middle stays clear so the real level is
    // visible and stage lines read directly against gameplay geometry.
    auto* top = CCLayerColor::create(ccc4(colors.surface.r, colors.surface.g, colors.surface.b, 225), win.width, 49.f);
    top->setPosition({0.f, win.height - 49.f});
    addChild(top, 20);

    auto* bottom = CCLayerColor::create(ccc4(colors.surface.r, colors.surface.g, colors.surface.b, 220), win.width, 58.f);
    bottom->setPosition({0.f, 0.f});
    addChild(bottom, 20);

    m_draw = CCDrawNode::create();
    addChild(m_draw, 5);
    m_markerLabels = CCNode::create();
    addChild(m_markerLabels, 8);

    m_titleLabel = makeLabel(editorTitle(), .31f, {16.f, win.height - 16.f}, colors.text, {0.f, .5f});
    addChild(m_titleLabel, 24);
    m_percentLabel = makeLabel("", .27f, {16.f, win.height - 36.f}, colors.muted, {0.f, .5f});
    addChild(m_percentLabel, 24);

    m_stageLabel = makeLabel("", .29f, {win.width / 2.f, 38.f}, colors.text);
    addChild(m_stageLabel, 24);
    m_helpLabel = makeLabel("Drag line = move boundary   |   drag level = pan", .20f, {win.width / 2.f, 15.f}, colors.muted);
    addChild(m_helpLabel, 24);

    auto* menu = CCMenu::create();
    menu->setPosition({0.f, 0.f});
    menu->setContentSize(win);
    menu->setID("editor-controls"_spr);
    addChild(menu, 30);

    auto* cancel = textButton("Cancel", this, menu_selector(StageEditorLayer::onCancel), 70.f);
    auto* reset = textButton("Reset", this, menu_selector(StageEditorLayer::onReset), 66.f);
    auto* save = textButton("Save", this, menu_selector(StageEditorLayer::onSave), 66.f, true);
    cancel->setPosition({win.width - 186.f, win.height - 24.f});
    reset->setPosition({win.width - 112.f, win.height - 24.f});
    save->setPosition({win.width - 39.f, win.height - 24.f});
    menu->addChild(cancel);
    menu->addChild(reset);
    menu->addChild(save);

    auto* previous = textButton("<", this, menu_selector(StageEditorLayer::onPrevious), 35.f);
    auto* focus = textButton("Focus", this, menu_selector(StageEditorLayer::onFocus), 62.f);
    auto* next = textButton(">", this, menu_selector(StageEditorLayer::onNext), 35.f);
    previous->setPosition({45.f, 31.f});
    focus->setPosition({99.f, 31.f});
    next->setPosition({153.f, 31.f});
    menu->addChild(previous);
    menu->addChild(focus);
    menu->addChild(next);

    if (auto* emblem = CCSprite::create(theme::pauseIconResource().c_str())) {
        auto const size = emblem->getContentSize();
        if (size.width > 0.f && size.height > 0.f) {
            emblem->setScale(std::min(30.f / size.width, 30.f / size.height));
        }
        emblem->setPosition({win.width - 230.f, win.height - 24.f});
        addChild(emblem, 24);
    }
}

float StageEditorLayer::levelXForMarker(int markerIndex) const {
    if (!m_playLayer || markerIndex < 0 || markerIndex >= static_cast<int>(m_draft.markerCount())) {
        return 0.f;
    }
    auto const percent = m_draft.legacy21()[static_cast<std::size_t>(markerIndex) + 1];
    return static_cast<float>(percent * static_cast<double>(m_playLayer->m_levelLength) / 100.0);
}

double StageEditorLayer::modernPercentForX(float x) const {
    if (!m_playLayer || m_playLayer->m_levelLength <= 0.f) {
        return 0.0;
    }
    auto const total = m_playLayer->timeForPos({m_playLayer->m_levelLength, 0.f}, 0.f, 0.f, true, 0.f);
    if (total <= 0.f) {
        return core::legacy21PercentFromX(x, m_playLayer->m_levelLength);
    }
    auto const at = m_playLayer->timeForPos({x, 0.f}, 0.f, 0.f, true, 0.f);
    return core::modern22PercentFromTime(at, total);
}

float StageEditorLayer::screenXForLevelX(float x) const {
    if (!m_worldRoot) {
        return -10000.f;
    }
    auto const world = m_worldRoot->convertToWorldSpace({x, 0.f});
    return convertToNodeSpace(world).x;
}

float StageEditorLayer::levelXForScreenPoint(CCPoint screenPoint) const {
    if (!m_worldRoot) {
        return 0.f;
    }
    return m_worldRoot->convertToNodeSpace(screenPoint).x;
}

int StageEditorLayer::nearestVisibleMarker(float screenX, float maxDistance) const {
    auto const win = getContentSize();
    auto best = -1;
    auto bestDistance = maxDistance;
    for (int i = 0; i < static_cast<int>(m_draft.markerCount()); ++i) {
        auto const x = screenXForLevelX(levelXForMarker(i));
        if (x < -maxDistance || x > win.width + maxDistance) {
            continue;
        }
        auto const distance = std::abs(x - screenX);
        if (distance <= bestDistance) {
            bestDistance = distance;
            best = i;
        }
    }
    return best;
}

void StageEditorLayer::refreshVisuals() {
    if (!m_draw || !m_markerLabels) {
        return;
    }
    m_draw->clear();
    m_markerLabels->removeAllChildrenWithCleanup(true);

    auto const win = getContentSize();
    auto const colors = theme::palette();
    auto const topY = win.height - 53.f;
    auto const bottomY = 82.f;

    // Full-profile ruler. It stays on screen even when the selected physical
    // line is off-camera and doubles as quick navigation between boundaries.
    auto const rulerLeft = 20.f;
    auto const rulerRight = win.width - 20.f;
    auto const rulerY = 69.f;
    m_draw->drawSegment({rulerLeft, rulerY}, {rulerRight, rulerY}, 1.2f, color4(colors.muted, .60f));
    auto const& rulerValues = TrainingManager::get().percentageMode() == PercentageMode::Legacy21
        ? m_draft.legacy21()
        : m_draft.modern22();
    for (std::size_t boundary = 0; boundary < rulerValues.size(); ++boundary) {
        auto const x = rulerLeft + static_cast<float>(rulerValues[boundary] / 100.0) * (rulerRight - rulerLeft);
        auto const inner = boundary > 0 && boundary + 1 < rulerValues.size();
        auto const selected = inner && static_cast<int>(boundary - 1) == m_selectedMarker;
        m_draw->drawDot({x, rulerY}, selected ? 5.f : (inner ? 3.2f : 2.5f), color4(selected ? colors.accent : (inner ? colors.bacon : colors.muted), .95f));
    }

    // Fixed 0% and 100% rails are visible when their world positions enter the
    // current camera view, but only inner profile markers are draggable.
    for (double edge : {0.0, 100.0}) {
        auto const x = screenXForLevelX(static_cast<float>(edge * m_playLayer->m_levelLength / 100.0));
        if (x >= 0.f && x <= win.width) {
            m_draw->drawSegment({x, bottomY}, {x, topY}, 1.f, color4(colors.muted, .45f));
        }
    }

    auto const& shown = TrainingManager::get().percentageMode() == PercentageMode::Legacy21
        ? m_draft.legacy21()
        : m_draft.modern22();

    for (int i = 0; i < static_cast<int>(m_draft.markerCount()); ++i) {
        auto const x = screenXForLevelX(levelXForMarker(i));
        if (x < -25.f || x > win.width + 25.f) {
            continue;
        }
        auto const selected = i == m_selectedMarker;
        auto const color = selected ? colors.accent : colors.bacon;
        m_draw->drawSegment({x, bottomY}, {x, topY}, selected ? 2.2f : 1.15f, color4(color, selected ? .96f : .62f));
        m_draw->drawDot({x, (bottomY + topY) / 2.f}, selected ? 8.f : 5.f, color4(color, .95f));

        auto const boundaryIndex = static_cast<std::size_t>(i) + 1;
        auto* pct = makeLabel(percentText(shown[boundaryIndex]), selected ? .27f : .22f, {x, topY - 10.f}, selected ? colors.accent : colors.text);
        m_markerLabels->addChild(pct);
    }

    refreshInfo();
}

void StageEditorLayer::refreshInfo() {
    if (!m_percentLabel || !m_stageLabel || m_selectedMarker < 0 ||
        m_selectedMarker >= static_cast<int>(m_draft.markerCount())) {
        return;
    }
    auto const index = static_cast<std::size_t>(m_selectedMarker) + 1;
    auto const& p21 = m_draft.legacy21();
    auto const& p22 = m_draft.modern22();
    m_percentLabel->setString(fmt::format(
        "Line {}/{}   2.1 {}   2.2 {}{}",
        m_selectedMarker + 1,
        m_draft.markerCount(),
        percentText(p21[index]),
        percentText(p22[index]),
        m_draft.changed() ? "   *" : ""
    ).c_str());

    auto const& shown = TrainingManager::get().percentageMode() == PercentageMode::Legacy21 ? p21 : p22;
    m_stageLabel->setString(fmt::format(
        "{} -> {}    |    {} -> {}",
        percentText(shown[index - 1]), percentText(shown[index]),
        percentText(shown[index]), percentText(shown[index + 1])
    ).c_str());
}

void StageEditorLayer::focusSelected() {
    if (!m_worldRoot || !m_worldRoot->getParent() || m_selectedMarker < 0) {
        return;
    }
    auto const x = levelXForMarker(m_selectedMarker);
    auto const currentWorld = m_worldRoot->convertToWorldSpace({x, 0.f});
    auto const win = getContentSize();
    auto const targetWorld = CCPoint{win.width / 2.f, currentWorld.y};
    auto* parent = m_worldRoot->getParent();
    auto const currentParent = parent->convertToNodeSpace(currentWorld);
    auto const targetParent = parent->convertToNodeSpace(targetWorld);
    m_worldRoot->setPositionX(m_worldRoot->getPositionX() + targetParent.x - currentParent.x);
    refreshVisuals();
}

void StageEditorLayer::panWorld(float screenDeltaX) {
    if (!m_worldRoot || !m_worldRoot->getParent() || std::abs(screenDeltaX) < .001f) {
        return;
    }
    auto* parent = m_worldRoot->getParent();
    auto const a = parent->convertToNodeSpace({0.f, 0.f});
    auto const b = parent->convertToNodeSpace({screenDeltaX, 0.f});
    m_worldRoot->setPositionX(m_worldRoot->getPositionX() + (b.x - a.x));
    refreshVisuals();
}

bool StageEditorLayer::ccTouchBegan(CCTouch* touch, CCEvent*) {
    if (!touch) {
        return false;
    }
    auto const location = touch->getLocation();
    auto const win = getContentSize();
    // Let CCMenu own the actual toolbar areas.
    if (location.y >= win.height - 53.f || location.y <= 58.f) {
        return false;
    }

    // Click the always-visible ruler to select/focus a boundary even if its
    // physical line is currently off-camera.
    if (location.y <= 80.f) {
        auto const& shown = TrainingManager::get().percentageMode() == PercentageMode::Legacy21
            ? m_draft.legacy21()
            : m_draft.modern22();
        auto const rulerLeft = 20.f;
        auto const rulerRight = win.width - 20.f;
        auto best = 0;
        auto bestDistance = 100000.f;
        for (int i = 0; i < static_cast<int>(m_draft.markerCount()); ++i) {
            auto const x = rulerLeft + static_cast<float>(shown[static_cast<std::size_t>(i) + 1] / 100.0) * (rulerRight - rulerLeft);
            auto const distance = std::abs(location.x - x);
            if (distance < bestDistance) {
                bestDistance = distance;
                best = i;
            }
        }
        setSelectedMarker(best, true);
        return true;
    }

    m_lastTouch = location;
    if (auto const marker = nearestVisibleMarker(location.x, 20.f); marker >= 0) {
        m_selectedMarker = marker;
        m_dragMode = DragMode::Marker;
        refreshVisuals();
    }
    else {
        m_dragMode = DragMode::Pan;
    }
    return true;
}

void StageEditorLayer::ccTouchMoved(CCTouch* touch, CCEvent*) {
    if (!touch || m_dragMode == DragMode::None) {
        return;
    }
    auto const location = touch->getLocation();
    if (m_dragMode == DragMode::Pan) {
        panWorld(location.x - m_lastTouch.x);
    }
    else if (m_dragMode == DragMode::Marker && m_playLayer) {
        auto x = std::clamp(levelXForScreenPoint(location), 0.f, m_playLayer->m_levelLength);
        auto const index = static_cast<std::size_t>(m_selectedMarker) + 1;
        auto const& p21 = m_draft.legacy21();
        auto const minX = static_cast<float>((p21[index - 1] + .05) * m_playLayer->m_levelLength / 100.0);
        auto const maxX = static_cast<float>((p21[index + 1] - .05) * m_playLayer->m_levelLength / 100.0);
        x = std::clamp(x, minX, maxX);
        auto const legacy = core::legacy21PercentFromX(x, m_playLayer->m_levelLength);
        auto const modern = modernPercentForX(x);
        if (m_draft.moveMarker(static_cast<std::size_t>(m_selectedMarker), legacy, modern)) {
            refreshVisuals();
        }
    }
    m_lastTouch = location;
}

void StageEditorLayer::ccTouchEnded(CCTouch*, CCEvent*) {
    m_dragMode = DragMode::None;
}

void StageEditorLayer::ccTouchCancelled(CCTouch*, CCEvent*) {
    m_dragMode = DragMode::None;
}

void StageEditorLayer::setSelectedMarker(int index, bool focus) {
    if (m_draft.markerCount() == 0) {
        return;
    }
    auto const count = static_cast<int>(m_draft.markerCount());
    while (index < 0) index += count;
    while (index >= count) index -= count;
    m_selectedMarker = index;
    if (focus) focusSelected();
    else refreshVisuals();
}

void StageEditorLayer::restoreWorldPosition() {
    if (m_worldRoot && m_haveOriginalWorldPosition) {
        m_worldRoot->setPosition(m_originalWorldPosition);
    }
}

void StageEditorLayer::closeEditor(bool save) {
    if (save && m_draft.changed()) {
        if (!TrainingManager::get().commitEditedBoundaries(m_draft.legacy21(), m_draft.modern22())) {
            FLAlertLayer::create("Stage Editor", "Could not save this layout.", "OK")->show();
            return;
        }
    }
    restoreWorldPosition();
    restorePauseUI();
    removeFromParentAndCleanup(true);
}

void StageEditorLayer::onSave(CCObject*) { closeEditor(true); }
void StageEditorLayer::onCancel(CCObject*) { closeEditor(false); }
void StageEditorLayer::onReset(CCObject*) { m_draft.reset(); refreshVisuals(); focusSelected(); }
void StageEditorLayer::onPrevious(CCObject*) { setSelectedMarker(m_selectedMarker - 1, true); }
void StageEditorLayer::onNext(CCObject*) { setSelectedMarker(m_selectedMarker + 1, true); }
void StageEditorLayer::onFocus(CCObject*) { focusSelected(); }
void StageEditorLayer::keyBackClicked() { closeEditor(false); }

} // namespace baconsistent::ui
