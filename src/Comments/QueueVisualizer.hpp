#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include "../Util/Structures.hpp"

class QueueVisualizerEntry : public CCNodeRGBA, public CCActionTweenDelegate {
public:
    static QueueVisualizerEntry* create(LevelCommentOptions data);
    
    void setTasksCompleted(int tasks);
    void setTasksToDo(int tasks);

    void setProgress(float porgress);
    void setProgressVisible(bool visible);

    
    void playRemoveAnimation(ccColor3B color, std::string const& iconFrame);
    void playFadeOutAnimation();

protected:
    bool init(LevelCommentOptions data);

    void update(float dt) override;

    void beginRemove();
    void updateTweenAction(float value, char const* key) override;
    void finishRemove();

    bool m_removing = false;
    ccColor3B m_removeColor = { 255, 255, 255 };

    int m_tasksCompleted = 0;
    int m_tasksToDo = 0;

    float m_targetProgress = 0;
    float m_displayedProgress = 0;

    LevelCommentOptions m_data;

    geode::NineSlice* m_background = nullptr;
    CCSprite* m_removeIcon = nullptr;
    geode::ProgressBar* m_progressBar = nullptr;
    geode::Label* m_progressLabel = nullptr;
};



class QueueVisualizerPopup : public geode::Popup {
public:
    static QueueVisualizerPopup* create();
    void addAutoRemoveEntry(QueueVisualizerEntry* entry);

protected:
    bool init() override;
    void update(float dt) override;
    void updateScrollbar();

    ScrollLayer* m_scrollLayer = nullptr;
    Scrollbar* m_scrollbar = nullptr;

    std::vector<QueueVisualizerEntry*> m_entries;
};

