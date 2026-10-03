#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include "QueueVisualizer.hpp"
#include "CommentManager.hpp"

#include "../Util/Structures.hpp"

#include <cmath>
#include <random>


//Random hue at fixed saturation/brightness so it never comes out dark or muddy
static ccColor3B randomBrightColor() {
    static std::mt19937 rng{std::random_device{}()};
    float h = std::uniform_real_distribution<float>(0, 6)(rng);
    float s = 0.75f;
    float v = 1;

    float c = v * s;
    float x = c * (1 - std::abs(std::fmod(h, 2) - 1));
    float m = v - c;

    float r = 0, g = 0, b = 0;
    switch (static_cast<int>(h)) {
        case 0: r = c; g = x; break;
        case 1: r = x; g = c; break;
        case 2: g = c; b = x; break;
        case 3: g = x; b = c; break;
        case 4: r = x; b = c; break;
        default: r = c; b = x; break;
    }

    return {
        static_cast<GLubyte>((r + m) * 255),
        static_cast<GLubyte>((g + m) * 255),
        static_cast<GLubyte>((b + m) * 255)
    };
}


QueueVisualizerEntry* QueueVisualizerEntry::create(LevelCommentOptions data) {
    auto ret = new QueueVisualizerEntry();
    if (ret->init(data)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool QueueVisualizerEntry::init(LevelCommentOptions data) {
    if (!CCNodeRGBA::init()) return false;
    this->setID(fmt::format("entry-'{}':'{}'", data.m_levelID, data.m_levelName));
    this->setContentSize({335, 75});
    this->setAnchorPoint({0.5f, 0.5f});

    this->m_data = data;

    auto nodeBG = geode::NineSlice::create("GJ_square02.png");
    nodeBG->setID("entry-bg");
    nodeBG->setZOrder(5);
    nodeBG->setContentSize(this->getContentSize());
    this->addChildAtPosition(nodeBG, Anchor::Center);
    this->m_background = nodeBG;

    auto buttonMenu = CCMenu::create();
    buttonMenu->setID("button-menu");
    buttonMenu->setZOrder(15);
    buttonMenu->setContentSize(this->getContentSize());
    this->addChildAtPosition(buttonMenu, Anchor::Center);

    auto mainNodeLabel = geode::Label::create(data.m_levelName.c_str(), "bigFont.fnt");
    mainNodeLabel->setID("main-label");
    mainNodeLabel->setZOrder(10);
    mainNodeLabel->setAnchorPoint({0, 1});
    mainNodeLabel->setLimitLabelWidth(240, 0.8f, 0.45f); //Room up to the trash button, without shrinking long names to nothing
    mainNodeLabel->setAlignment(geode::Label::Alignment::Left);
    this->addChildAtPosition(mainNodeLabel, Anchor::TopLeft, {10, -9});

    auto secondaryNodeLabel = geode::Label::create(fmt::format("{}: #{}", data.m_creatorName, data.m_levelID), "mdFont.fnt");
    secondaryNodeLabel->setID("secondary-label");
    secondaryNodeLabel->setZOrder(10);
    secondaryNodeLabel->setColor({ 150, 150, 150 });
    secondaryNodeLabel->setAnchorPoint({0, 1});
    secondaryNodeLabel->setLimitLabelWidth(110, 0.6f, 0.35f); //Stops before the progress bar
    secondaryNodeLabel->setAlignment(geode::Label::Alignment::Left);
    this->addChildAtPosition(secondaryNodeLabel, Anchor::TopLeft, {11, -37});

    if (data.m_persistent) {
        auto persistentNodeLabel = geode::Label::create("persistent", "mdFont.fnt");
        persistentNodeLabel->setID("persistent-label");
        persistentNodeLabel->setZOrder(10);
        persistentNodeLabel->setColor({ 201, 77, 201 });
        persistentNodeLabel->setAnchorPoint({0, 1});
        persistentNodeLabel->setLimitLabelWidth(110, 0.6f, 0.35f); //Stops before the progress bar
        persistentNodeLabel->setAlignment(geode::Label::Alignment::Left);
        this->addChildAtPosition(persistentNodeLabel, Anchor::TopLeft, {11, -52});
    }


    //Make sure this isnt visible until its current in the queue
    this->m_progressBar = geode::ProgressBar::create(ProgressBarStyle::Level);
    this->m_progressBar->setID("comment-progress-bar");
    this->m_progressBar->setZOrder(10);
    this->m_progressBar->setLayout(AnchorLayout::create());
    this->m_progressBar->setAnchorPoint({1, 0});
    this->m_progressBar->setFillColor(randomBrightColor());

    if (auto fill = typeinfo_cast<CCSprite*>(this->m_progressBar->getChildByIDRecursive("progress-bar-fill"))) {
        ccTexParams params = { GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT };
        fill->getTexture()->setTexParameters(&params);
    }
    this->addChildAtPosition(this->m_progressBar, Anchor::BottomRight, {-5, 5});


    this->m_progressLabel = geode::Label::create("bigFont.fnt");
    this->m_progressLabel->setID("progress-bar-label");
    this->m_progressLabel->setZOrder(10);
    this->m_progressLabel->setText(fmt::format("{}/{}", this->m_tasksCompleted, this->m_tasksToDo));
    this->m_progressLabel->setScale(0.3f);
    this->m_progressLabel->setAnchorPoint({1, 0.5f});
    this->addChildAtPosition(this->m_progressLabel, Anchor::BottomRight, {-215, 14});

    this->setProgressVisible(false);


    auto removeFromQueueButton = geode::Button::createWithSpriteFrameName(
        "GJ_trashBtn_001.png",
        [this](geode::Button* button) {
            geode::createQuickPopup(
                "Remove from queue",
                "Are you sure you want to <cr>delete</c> this level from the queue?",
                "Yes", "Cancel",
                [self = Ref(this)](FLAlertLayer* popup, bool cancel) {
                    if (cancel) return;

                    CommentManager::get()->requestRemoveQueueItem(self->m_data.m_levelID);
                    self->playRemoveAnimation({ 230, 70, 70 }, "GJ_deleteIcon_001.png");
                }
            );
        }
    );
    removeFromQueueButton->setScale(0.75f);
    buttonMenu->addChildAtPosition(removeFromQueueButton, Anchor::TopRight, {-20, -20});

    this->scheduleUpdate();


    return true;
}

void QueueVisualizerEntry::update(float dt) {
    CCNodeRGBA::update(dt);

    if (this->m_removing) return;

    //Level finished
    if (!CommentManager::get()->isLevelInQueue(this->m_data.m_levelID)) {
        if (CommentManager::get()->consumeCompleted(this->m_data.m_levelID)) {
            this->playRemoveAnimation({ 90, 230, 110 }, "GJ_completesIcon_001.png");
        } else {
            this->playFadeOutAnimation();
        }
        return;
    }

    auto res =CommentManager::get()->tryGetQueueProgress(this->m_data.m_levelID);
    if (res.isErr()) {
        //Not the level currently being processed (or it finished)
        if (this->m_tasksCompleted > 0) {
            this->m_tasksCompleted = 0;
            this->m_tasksToDo = 0;
            this->setProgress(0);
            this->m_displayedProgress = 0; //Snap back instead of animating down while hidden
            this->m_progressBar->updateProgress(0);
            this->setProgressVisible(false);
        }
        return;
    }

    //Update labels
    QueueItemProgress progress = res.unwrap();
    this->setProgress(progress.m_progress);
    this->setTasksToDo(progress.m_tToDo);
    this->setTasksCompleted(progress.m_tCompleted);

    //Ease the displayed progress toward the target (framerate independent)
    if (this->m_displayedProgress != this->m_targetProgress) {
        float t = 1 - std::exp(-8 * dt);
        this->m_displayedProgress += (this->m_targetProgress - this->m_displayedProgress) * t;
        if (std::abs(this->m_targetProgress - this->m_displayedProgress) < 0.05f) {
            this->m_displayedProgress = this->m_targetProgress;
        }
        this->m_progressBar->updateProgress(this->m_displayedProgress);
    }
}

//Removal animation timings (seconds)
constexpr float textFadeTime = 0.2f; //Text, bar and buttons fade out first
constexpr float shrinkTime = 0.3f;   //Then the card shrinks into a square
constexpr float holdTime = 0.9f;     //Icon stays on screen
constexpr float fadeTime = 0.15f;    //Everything fades out
constexpr float iconSquareSize = 60;

static float easeOutCubic(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return 1 - std::pow(1.0f - t, 3.0f);
}

//labels, menus and the progress bar are nested, so set opacity on the whole tree
static void setOpacityRecursive(CCNode* node, GLubyte opacity) {
    if (auto rgba = typeinfo_cast<CCRGBAProtocol*>(node)) rgba->setOpacity(opacity);
    for (auto child : CCArrayExt<CCNode*>(node->getChildren())) {
        setOpacityRecursive(child, opacity);
    }
}

void QueueVisualizerEntry::beginRemove() {
    this->m_removing = true;

    //no more trash button presses mid-animation
    if (auto menu = typeinfo_cast<CCMenu*>(this->getChildByID("button-menu"))) menu->setEnabled(false);
}

void QueueVisualizerEntry::playRemoveAnimation(ccColor3B color, std::string const& iconFrame) {
    if (this->m_removing) return;
    this->beginRemove();
    this->m_removeColor = color;

    this->m_removeIcon = CCSprite::createWithSpriteFrameName(iconFrame.c_str());
    this->m_removeIcon->setID("remove-icon");
    this->m_removeIcon->setZOrder(20);
    this->m_removeIcon->setScale(0);
    this->addChildAtPosition(this->m_removeIcon, Anchor::Center);

    this->runAction(CCSequence::create(
        CCActionTween::create(textFadeTime, "content-opacity", 255, 0),
        //The empty card shrinks into a square, and the icon pops in with a slight overshoot near the end
        CCSpawn::create(
            CCActionTween::create(shrinkTime, "shrink", 0, 1),
            CCSequence::create(
                CCDelayTime::create(shrinkTime * 0.7f),
                CCTargetedAction::create(this->m_removeIcon, CCEaseBackOut::create(CCScaleTo::create(0.25f, 1))),
                nullptr
            ),
            nullptr
        ),
        CCDelayTime::create(holdTime),

        //Fade the square and icon out, then remove
        CCActionTween::create(fadeTime, "square-opacity", 255, 0),
        CallFuncExt::create([this] { this->finishRemove(); }),
        nullptr
    ));
}

void QueueVisualizerEntry::playFadeOutAnimation() {
    if (this->m_removing) return;
    this->beginRemove();

    this->runAction(CCSequence::create(
        CCActionTween::create(fadeTime, "opacity", 255, 0),
        CallFuncExt::create([this] { this->finishRemove(); }),
        nullptr
    ));
}


void QueueVisualizerEntry::updateTweenAction(float value, char const* key) {
    std::string_view name = key;
    auto opacity = static_cast<GLubyte>(value);

    if (name == "opacity") {
        setOpacityRecursive(this, opacity);
    }
    else if (name == "content-opacity") {
        for (auto child : CCArrayExt<CCNode*>(this->getChildren())) {
            if (child == this->m_background || child == this->m_removeIcon) continue;
            setOpacityRecursive(child, opacity);
        }
    }
    else if (name == "shrink") {
        //Tints from white to the remove color as it shrinks
        float shrink = easeOutCubic(value);
        auto fullSize = this->getContentSize();
        this->m_background->setContentSize({
            fullSize.width + (iconSquareSize - fullSize.width) * shrink,
            fullSize.height + (iconSquareSize - fullSize.height) * shrink
        });
        auto tint = [shrink](GLubyte target) { return static_cast<GLubyte>(255 + (target - 255) * shrink); };
        this->m_background->setColor({ tint(this->m_removeColor.r), tint(this->m_removeColor.g), tint(this->m_removeColor.b) });
    }
    else if (name == "square-opacity") {
        this->m_background->setOpacity(opacity);
        if (this->m_removeIcon) this->m_removeIcon->setOpacity(opacity);
    }
}

void QueueVisualizerEntry::finishRemove() {
    Ref self = this;
    this->unscheduleUpdate();

    auto parent = this->getParent();
    this->removeFromParentAndCleanup(true);
    if (parent) parent->updateLayout();
}

void QueueVisualizerEntry::setTasksCompleted(int tasks) {
    this->m_tasksCompleted = tasks;
    if (this->m_tasksCompleted > 0 && this->m_tasksToDo > 0) {
        this->setProgressVisible(true);
        this->m_progressLabel->setText(fmt::format("{}/{}", this->m_tasksCompleted, this->m_tasksToDo));
    }
}

void QueueVisualizerEntry::setTasksToDo(int tasks) {
    this->m_tasksToDo = tasks;
}

void QueueVisualizerEntry::setProgress(float progress) {
    this->m_targetProgress = progress;
}

void QueueVisualizerEntry::setProgressVisible(bool visible) {
    this->m_progressBar->setVisible(visible);
    this->m_progressLabel->setVisible(visible);
}








QueueVisualizerPopup* QueueVisualizerPopup::create() {
    auto ret = new QueueVisualizerPopup();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}


bool QueueVisualizerPopup::init() {
    if (!geode::Popup::init({400, 280})) return false;
    this->setID("queue-vizualiser-popup"_spr);

    this->setTitle("Comment Manager Queue");

    this->m_closeBtn->setID("close-button");
    this->m_bgSprite->setID("background-sprite");
    this->m_title->setID("title");
    this->m_buttonMenu->setID("main-button-menu");

    this->m_scrollLayer = ScrollLayer::create({350, 230});
    this->m_scrollLayer->setID("scroll-layer");
    this->m_scrollLayer->setZOrder(15);
    this->m_scrollLayer->ignoreAnchorPointForPosition(false);
    this->m_mainLayer->addChildAtPosition(this->m_scrollLayer, Anchor::Center, {0, -10});

    auto scrollLayerBG = geode::NineSlice::create("square02_001.png");
    scrollLayerBG->setID("scroll-layer-bg");
    scrollLayerBG->setZOrder(10);
    scrollLayerBG->setContentSize({350, 230});
    scrollLayerBG->setOpacity(150);
    this->m_mainLayer->addChildAtPosition(scrollLayerBG, Anchor::Center, {0, -10});


    auto content = this->m_scrollLayer->m_contentLayer;
    content->setLayout(
        ColumnLayout::create()
            ->setAxisReverse(true)
            ->setAxisAlignment(AxisAlignment::End)
            ->setAutoGrowAxis(this->m_scrollLayer->getContentHeight())
            ->setGap(5)
            ->setPadding(Padding::vertical(7.5f))
            ->setCrossAxisOverflow(false)
    );

    for (auto const& item : CommentManager::get()->getQueue()) {
        if (auto entry = QueueVisualizerEntry::create(item)) {
            content->addChild(entry);
        }
    }

    content->updateLayout();
    this->m_scrollLayer->scrollToTop();




    this->m_scrollbar = Scrollbar::create(this->m_scrollLayer);
    this->m_scrollbar->setID("scrollbar");
    this->m_scrollbar->setZOrder(15);
    this->m_mainLayer->addChildAtPosition(this->m_scrollbar, Anchor::Center, {this->m_scrollLayer->getContentWidth() / 2 + 10, -10});
    this->updateScrollbar();

    this->scheduleUpdate();

    return true;
}

void QueueVisualizerPopup::update(float dt) {
    geode::Popup::update(dt);
    this->updateScrollbar();
}

void QueueVisualizerPopup::updateScrollbar() {
    bool visible = this->m_scrollLayer->m_contentLayer->getChildrenCount() > 3;
    if (this->m_scrollbar->isVisible() == visible) return;

    this->m_scrollbar->setVisible(visible);
    this->m_scrollbar->setTouchEnabled(visible);
}