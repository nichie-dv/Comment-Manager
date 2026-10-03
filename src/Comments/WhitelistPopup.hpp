#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include "CommentManager.hpp"

#include "../Util/Structures.hpp"

//Whitelist entry

class WhitelistEntry : public CCNodeRGBA {
public:
    static WhitelistEntry* create(WhitelistedItem data);
    WhitelistedItem* getData();

    float m_yOffset = 0;

protected:
    bool init(WhitelistedItem data);
    void onNodeToggled(CCObject* sender);
    void onDeleteNode(CCObject* sender);

    CCMenu* m_buttonMenu = nullptr;
    geode::NineSlice* m_background = nullptr;
    CCMenuItemToggler* m_toggler = nullptr;

    WhitelistedItem m_data;
};



//Whitelist popup

class WhitelistPopup : public geode::Popup {
public:
    static WhitelistPopup* create(CommentManagerPopup* parent);
    void addNewEntry(WhitelistEntry* entry);
    void removeEntry(WhitelistEntry* entry);
    void updateContentSize();

protected:
    bool init(CommentManagerPopup* parent);

    LevelCommentOptions m_options;

    CommentManagerPopup* m_parentPopup = nullptr;
    geode::Scrollbar* m_scrollbar = nullptr;
    ScrollLayer* m_scrollLayer = nullptr;

    std::vector<WhitelistedItem*> m_whitelist;
};


//Add a user popup

class NewWhitelistUserPopup : public geode::Popup, public LevelManagerDelegate, public UserInfoDelegate {
public:
    static NewWhitelistUserPopup* create(WhitelistPopup* popup);
    ~NewWhitelistUserPopup() override;

    void fillFromScore(GJUserScore* score);

protected:
    bool init(WhitelistPopup* popup);

    void searchByUsername();
    void searchByAccountID();
    void setLoading(bool loading);

    //Username search results
    void loadLevelsFinished(CCArray* levels, char const* key) override;
    void loadLevelsFinished(CCArray* levels, char const* key, int type) override;
    void loadLevelsFailed(char const* key) override;
    void loadLevelsFailed(char const* key, int type) override;

    //Account ID lookup results
    void getUserInfoFinished(GJUserScore* score) override;
    void getUserInfoFailed(int id) override;

    geode::TextInput* m_usernameInput = nullptr;
    geode::TextInput* m_accountIDInput = nullptr;
    geode::LoadingSpinner* m_spinner = nullptr;

    WhitelistedItem m_data;
    std::string m_pendingUsername = "";
    int m_pendingAccountID = 0;
};


//Friends picker popup

class FriendPickerPopup : public geode::Popup, public UserListDelegate {
public:
    static FriendPickerPopup* create(std::function<void(GJUserScore*)> callback);
    ~FriendPickerPopup() override;

protected:
    bool init(std::function<void(GJUserScore*)> callback);

    void populate(CCArray* friends);
    void showMessage(std::string const& message);

    void getUserListFinished(CCArray* scores, UserListType type) override;
    void getUserListFailed(UserListType type, GJErrorCode errorType) override;

    std::function<void(GJUserScore*)> m_callback;

    geode::Scrollbar* m_scrollbar = nullptr;
    ScrollLayer* m_scrollLayer = nullptr;
    geode::LoadingSpinner* m_spinner = nullptr;
    geode::Label* m_messageLabel = nullptr;
};
