#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include "CommentManager.hpp"

#include "../Util/Structures.hpp"

//Wordlist entry

class WordlistEntry : public CCNodeRGBA {
public:
    static WordlistEntry* create(BannedWord data);
    BannedWord* getData();

    float m_yOffset = 0;

protected:
    bool init(BannedWord data);
    void onNodeToggled(CCObject* sender);
    void onDeleteNode(CCObject* sender);
    
    CCMenu* m_buttonMenu = nullptr;
    geode::NineSlice* m_background = nullptr;
    CCMenuItemToggler* m_toggler = nullptr;

    BannedWord m_data;
};



//Wordlist popup

class WordlistPopup : public geode::Popup {
public:
    static WordlistPopup* create(CommentManagerPopup* parent);
    void addNewEntry(WordlistEntry* entry);
    void removeEntry(WordlistEntry* entry);
    void updateContentSize();

protected:
    bool init(CommentManagerPopup* parent);

    LevelCommentOptions m_options;

    CommentManagerPopup* m_parentPopup = nullptr;
    geode::Scrollbar* m_scrollbar = nullptr;
    ScrollLayer* m_scrollLayer = nullptr;

    std::vector<BannedWord*> m_wordlist;
};


//Add a word popup

class NewWordPopup : public geode::Popup {
public:
    static NewWordPopup* create(WordlistPopup* popup);

protected:
    bool init(WordlistPopup* popup);

    BannedWord m_data;
};
