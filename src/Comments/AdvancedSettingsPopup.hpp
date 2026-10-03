#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include "CommentManager.hpp"

#include "../Util/Structures.hpp"

class AdvancedSettingsPopup : public geode::Popup {
public:
    static AdvancedSettingsPopup* create(CommentManagerPopup* popup);

protected:
    bool init(CommentManagerPopup* popup);

    CCMenuItemToggler* createToggler(CCMenu* parent, int tag, char const* id, char const* text, bool value);
    void onTogglerActivated(CCObject* sender);
    void updateDateButtons();
    void updateRegexInput();
    void save();

    CommentManagerPopup* m_parentPopup = nullptr;
    LevelCommentOptions m_options;

    CCMenuItemToggler* m_persistentToggler = nullptr;
    CCMenuItemToggler* m_useDatesToggler = nullptr;
    CCMenuItemToggler* m_useRegexToggler = nullptr;
    CCMenuItemToggler* m_keepLogsToggler = nullptr;

    geode::TextInput* m_regexInput = nullptr;

    geode::Button* m_startDateButton = nullptr;
    geode::Button* m_endDateButton = nullptr;
    ButtonSprite* m_startDateSprite = nullptr;
    ButtonSprite* m_endDateSprite = nullptr;
};
