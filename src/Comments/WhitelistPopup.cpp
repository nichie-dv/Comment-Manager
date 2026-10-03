#include "WhitelistPopup.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include "CommentManager.hpp"

#include "../Util/OptionsSaveLoad.hpp"
#include "../Util/Structures.hpp"

//Entry

WhitelistEntry* WhitelistEntry::create(WhitelistedItem data) {
    auto ret = new WhitelistEntry();
    if (ret->init(data)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool WhitelistEntry::init(WhitelistedItem data) {
    if (!CCNode::init()) return false;

    this->m_data = std::move(data);

    this->setID("whitelist-entry-node");
    this->setContentSize({330, 40});
    this->setAnchorPoint({0.5f, 0.5f});
    this->setCascadeColorEnabled(true);

    this->m_background = geode::NineSlice::create("GJ_square05.png");
    this->m_background->setScaleMultiplier(0.85f);
    this->m_background->setID("node-background");
    this->m_background->setZOrder(5);
    this->m_background->setContentSize(this->getContentSize());
    this->addChildAtPosition(this->m_background, Anchor::Center);



    this->m_buttonMenu = CCMenu::create();
    this->m_buttonMenu->setID("node-button-menu");
    this->m_buttonMenu->setZOrder(6);
    this->m_buttonMenu->setContentSize(this->getContentSize());
    this->m_buttonMenu->setCascadeColorEnabled(true);
    this->addChildAtPosition(this->m_buttonMenu, Anchor::Center);




    auto label = geode::Label::create(this->m_data.m_username.c_str(), "mdFontB.fnt");
    label->setID("node-username-label");
    label->setZOrder(7);
    label->setAnchorPoint({0, 0.5f});
    label->setLimitLabelWidth(145, 0.9f);
    this->addChildAtPosition(label, Anchor::Left, {10, 0});

    auto idLabel = geode::Label::create(fmt::format("#{}", this->m_data.m_accountID).c_str(), "mdFontB.fnt");
    idLabel->setID("node-account-id-label");
    idLabel->setZOrder(7);
    idLabel->setAnchorPoint({1, 0.5f});
    idLabel->setScale(0.6f);
    idLabel->setOpacity(150);
    this->addChildAtPosition(idLabel, Anchor::Right, {-75, 0});


    this->m_toggler = CCMenuItemToggler::createWithStandardSprites(
        this,
        menu_selector(WhitelistEntry::onNodeToggled),
        0.75f
    );
    this->m_toggler->setID("user-toggler");
    this->m_toggler->setZOrder(9);
    this->m_toggler->toggle(this->m_data.m_enabled);
    if (this->m_data.m_enabled) {
        this->setColor({ 255, 255, 255 });
    } else {
        this->setColor({ 150, 150, 150 });
    }

    this->m_buttonMenu->addChildAtPosition(this->m_toggler, Anchor::Right, {-50, 0});



    auto deleteButtonSpr = CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");
    deleteButtonSpr->setScale(0.65f);

    auto deleteButton = CCMenuItemSpriteExtra::create(
        deleteButtonSpr,
        this,
        menu_selector(WhitelistEntry::onDeleteNode)
    );
    deleteButton->setID("delete-node-button");
    deleteButton->setZOrder(10);
    deleteButton->setCascadeColorEnabled(false);
    this->m_buttonMenu->addChildAtPosition(deleteButton, Anchor::Right, {-20, 0});


    return true;
}

void WhitelistEntry::onNodeToggled(CCObject* sender) {
    this->m_data.m_enabled = !this->m_data.m_enabled;

    if (this->m_data.m_enabled) {
        this->setColor({ 255, 255, 255 });
    } else {
        this->setColor({ 150, 150, 150 });
    }
}

void WhitelistEntry::onDeleteNode(CCObject* sender) {
    WhitelistPopup* popup = nullptr;

    //Looks nasty but who gaf
    if ((popup = static_cast<WhitelistPopup*>(this
    ->getParent()                                   //content layer
    ->getParent()                                   //scroll layer
    ->getParent()                                   //cclayer
    ->getParent()))) {                              //popup
        popup->removeEntry(this);
        popup->updateContentSize();
    } else {
        log::error("Whitelist entry node missing parent, cannot resize popup content layer.");
    }
}

WhitelistedItem* WhitelistEntry::getData() {
    return &this->m_data;
}



//Popup


WhitelistPopup* WhitelistPopup::create(CommentManagerPopup* parent) {
    auto ret = new WhitelistPopup();
    if (ret->init(parent)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool WhitelistPopup::init(CommentManagerPopup* parent) {
    if (!geode::Popup::init({370, 290}, "GJ_square04.png")) return false;

    this->setID("edit-whitelist-popup"_spr);
    this->m_parentPopup = parent;

    this->m_bgSprite->setID("background-sprite");
    if (this->m_title) this->m_title->setID("title");
    this->m_buttonMenu->setID("main-button-menu");

    //Replaced by an "OK" button at the bottom
    this->m_closeBtn->removeFromParentAndCleanup(true);


    //The parent popup holds the loaded options, so edits here stay in sync with it
    this->m_options = this->m_parentPopup->getOptions();


    auto newCloseButton = geode::Button::createWithNode(
        ButtonSprite::create("OK"),
        [this](geode::Button* button) {
            //Save data
            std::vector<WhitelistedItem> output;
            output.reserve(this->m_whitelist.size());
            for (auto* user : this->m_whitelist) output.push_back(*user);

            this->m_options = this->m_parentPopup->getOptions();
            this->m_options.m_whitelist = output;
            this->m_parentPopup->setOptions(this->m_options);
            SetSavedOptions(this->m_options);

            this->onClose(button);
        }
    );
    newCloseButton->setID("close-button");
    newCloseButton->setZOrder(10);
    this->m_buttonMenu->addChildAtPosition(newCloseButton, Anchor::Bottom, {0, 25});

    auto infoPopupButton = geode::Button::createWithSpriteFrameName(
        "GJ_infoIcon_001.png",
        [this](geode::Button* button) {
            FLAlertLayer::create(
                "Whitelist Help",
                "Comments from users on this list will <cg>never be deleted</c>, "
                "even if they contain words from the <cy>wordlist</c>. "
                "This can be disabled in the <cf>advanced</c> tab.",
                "OK"
            )->show();
        }
    );
    infoPopupButton->setID("info-popup-button");
    infoPopupButton->setZOrder(10);
    this->m_buttonMenu->addChildAtPosition(infoPopupButton, Anchor::TopRight);



    auto clearAllButton = geode::Button::createWithSprite(
        "N_deleteAllBtn_01.png"_spr,
        [this](geode::Button* button) {
            geode::createQuickPopup(
                "Remove all users",
                "Are you sure you want to <cr>remove all users</c> from the whitelist?",
                "Yes", "Cancel",
                [this](FLAlertLayer* popup, bool cancel) {
                    if (cancel) return;

                    this->m_whitelist.clear();
                    this->m_scrollLayer->m_contentLayer->removeAllChildrenWithCleanup(true);
                    this->updateContentSize();
                }, true, true
            );
        }
    );
    clearAllButton->setID("clear-all-button");
    clearAllButton->setZOrder(10);
    clearAllButton->setScale(0.75f);
    this->m_buttonMenu->addChildAtPosition(clearAllButton, Anchor::BottomRight, {-25, 25});

    auto newUserButton = geode::Button::createWithSpriteFrameName(
        "GJ_plusBtn_001.png",
        [this](geode::Button* button) {
            NewWhitelistUserPopup::create(this)->show();
        }
    );
    newUserButton->setScale(0.7f);
    newUserButton->setID("new-user-button");
    newUserButton->setZOrder(10);
    this->m_buttonMenu->addChildAtPosition(newUserButton, Anchor::BottomLeft, {25, 25});


    //Scrolling shi here
    this->m_scrollLayer = ScrollLayer::create({350, 230});
    this->m_scrollLayer->setID("scroll-layer");
    this->m_scrollLayer->setZOrder(15);
    this->m_scrollLayer->ignoreAnchorPointForPosition(false);
    this->m_mainLayer->addChildAtPosition(this->m_scrollLayer, Anchor::Center, {-4.25f, 18});

    this->m_scrollbar = geode::Scrollbar::create(this->m_scrollLayer);
    this->m_scrollbar->setID("scrollbar");
    this->m_scrollbar->setZOrder(15);
    this->m_scrollbar->setVisible(this->m_whitelist.size() > 5);
    this->m_mainLayer->addChildAtPosition(this->m_scrollbar, Anchor::Right, {-8.5f, 18});

    auto scrollLayerBG = geode::NineSlice::create("square02_001.png");
    scrollLayerBG->setID("scroll-layer-bg");
    scrollLayerBG->setZOrder(10);
    scrollLayerBG->setContentSize({350, 230});
    scrollLayerBG->setOpacity(150);
    this->m_mainLayer->addChildAtPosition(scrollLayerBG, Anchor::Center, {-4.25f, 18});


    for (auto const& user : this->m_options.m_whitelist) {
        this->addNewEntry(WhitelistEntry::create(user));
    }

    return true;
}

void WhitelistPopup::addNewEntry(WhitelistEntry* entry) {
    auto* data = entry->getData();
    for (auto* existing : this->m_whitelist) {
        if (existing->m_accountID == data->m_accountID) return; // duplicate, skip entirely
    }

    this->m_whitelist.push_back(data);
    this->m_scrollLayer->m_contentLayer->addChild(entry);
    this->updateContentSize();
}

void WhitelistPopup::removeEntry(WhitelistEntry* entry) {
    std::erase(this->m_whitelist, entry->getData()); // before the node dies
    entry->removeFromParentAndCleanup(true);
}


void WhitelistPopup::updateContentSize() {
    auto content = this->m_scrollLayer->m_contentLayer;

    constexpr float gap = 5;
    constexpr float padding = 10;

    float y = padding;
    if (content->getChildrenCount() > 0) {
        for (auto child : CCArrayExt<WhitelistEntry*>(content->getChildren())) {
            float h = child->getContentHeight();
            child->m_yOffset = y + h / 2;
            y += h + gap;
        }
        y -= gap;
    }

    //Bottom
    y += padding;

    float newHeight = std::max(y, this->m_scrollLayer->getContentHeight());
    content->setContentHeight(newHeight);

    if (content->getChildrenCount() > 0) {
        for (auto child : CCArrayExt<WhitelistEntry*>(content->getChildren())) {
            child->setPosition({ content->getContentWidth() / 2, newHeight - child->m_yOffset });
        }
    }

    this->m_scrollLayer->scrollToTop();
    this->m_scrollbar->setVisible(content->getChildrenCount() > 5);
}



//New User

NewWhitelistUserPopup* NewWhitelistUserPopup::create(WhitelistPopup* popup) {
    auto ret = new NewWhitelistUserPopup();
    if (ret->init(popup)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

NewWhitelistUserPopup::~NewWhitelistUserPopup() {
    auto glm = GameLevelManager::get();
    if (glm->m_levelManagerDelegate == this) glm->m_levelManagerDelegate = nullptr;
    if (glm->m_userInfoDelegate == this) glm->m_userInfoDelegate = nullptr;
}

bool NewWhitelistUserPopup::init(WhitelistPopup* popup) {
    if (!geode::Popup::init({240, 160})) return false;

    this->setID("new-whitelisted-user-popup"_spr);

    this->m_closeBtn->setID("close-button");
    this->m_bgSprite->setID("background-sprite");
    if (this->m_title) this->m_title->setID("title");
    this->m_buttonMenu->setID("main-button-menu");

    //Create Button
    auto createButton = geode::Button::createWithNode(
        ButtonSprite::create("Add"),
        [this, popup](geode::Button* button) {
            if (this->m_data.m_accountID == 0 || this->m_data.m_username.empty()) {
                Notification::create("Search for the user first", NotificationIcon::Error)->show();
                return;
            }

            popup->addNewEntry(WhitelistEntry::create(this->m_data));
            this->onClose(button);
        }
    );
    createButton->setID("create-button");
    createButton->setZOrder(10);
    this->m_buttonMenu->addChildAtPosition(createButton, Anchor::Bottom, {40, 25});

    //Friends picker
    auto friendsButton = geode::Button::createWithNode(
        ButtonSprite::create("Friends", "goldFont.fnt", "GJ_button_04.png", 1),
        [this](geode::Button* button) {
            FriendPickerPopup::create([this](GJUserScore* score) {
                this->fillFromScore(score);
            })->show();
        }
    );
    friendsButton->setID("friends-button");
    friendsButton->setZOrder(10);
    friendsButton->setScale(0.8f);
    this->m_buttonMenu->addChildAtPosition(friendsButton, Anchor::Bottom, {-45, 25});


    //Username input
    this->m_usernameInput = geode::TextInput::create(140, "Username", "mdFontB.fnt");
    this->m_usernameInput->setID("username-input");
    this->m_usernameInput->setZOrder(10);
    this->m_usernameInput->setCommonFilter(CommonFilter::Name);
    this->m_usernameInput->setCallback(
        [this](std::string out) {
            //Any previous lookup no longer matches what's typed
            this->m_data.m_username = out;
            this->m_data.m_accountID = 0;
            this->m_data.m_userID = 0;
            this->m_accountIDInput->setString("");
        }
    );
    this->m_buttonMenu->addChildAtPosition(this->m_usernameInput, Anchor::Center, {-20, 35});

    auto usernameSearchSpr = CCSprite::createWithSpriteFrameName("gj_findBtn_001.png");
    usernameSearchSpr->setScale(0.75f);
    auto usernameSearchButton = CCMenuItemExt::createSpriteExtra(
        usernameSearchSpr,
        [this](CCMenuItemSpriteExtra* sender) { this->searchByUsername(); }
    );
    usernameSearchButton->setID("username-search-button");
    usernameSearchButton->setZOrder(10);
    this->m_buttonMenu->addChildAtPosition(usernameSearchButton, Anchor::Center, {80, 35});

    //Account ID input
    this->m_accountIDInput = geode::TextInput::create(140, "Account ID", "mdFontB.fnt");
    this->m_accountIDInput->setID("account-id-input");
    this->m_accountIDInput->setZOrder(10);
    this->m_accountIDInput->setCommonFilter(CommonFilter::Uint);
    this->m_accountIDInput->setCallback(
        [this](std::string out) {
            this->m_data.m_accountID = geode::utils::numFromString<unsigned int>(out).unwrapOr(0);
            this->m_data.m_userID = 0;
            this->m_data.m_username = "";
            this->m_usernameInput->setString("");
        }
    );
    this->m_buttonMenu->addChildAtPosition(this->m_accountIDInput, Anchor::Center, {-20, 0});

    auto accountSearchSpr = CCSprite::createWithSpriteFrameName("gj_findBtn_001.png");
    accountSearchSpr->setScale(0.75f);
    auto accountSearchButton = CCMenuItemExt::createSpriteExtra(
        accountSearchSpr,
        [this](CCMenuItemSpriteExtra* sender) { this->searchByAccountID(); }
    );
    accountSearchButton->setID("account-id-search-button");
    accountSearchButton->setZOrder(10);
    this->m_buttonMenu->addChildAtPosition(accountSearchButton, Anchor::Center, {80, 0});


    this->m_spinner = geode::LoadingSpinner::create(20);
    this->m_spinner->setID("loading-spinner");
    this->m_spinner->setZOrder(20);
    this->m_spinner->setVisible(false);
    this->m_mainLayer->addChildAtPosition(this->m_spinner, Anchor::Center, {0, -25});


    return true;
}

void NewWhitelistUserPopup::setLoading(bool loading) {
    this->m_spinner->setVisible(loading);
}

void NewWhitelistUserPopup::fillFromScore(GJUserScore* score) {
    if (!score) return;

    this->m_data.m_username = score->m_userName;
    this->m_data.m_accountID = static_cast<unsigned int>(score->m_accountID);
    this->m_data.m_userID = static_cast<unsigned int>(score->m_userID);

    //setString doesn't fire the input callbacks, so the data above stays intact
    this->m_usernameInput->setString(this->m_data.m_username);
    this->m_accountIDInput->setString(fmt::format("{}", this->m_data.m_accountID));
}

void NewWhitelistUserPopup::searchByUsername() {
    std::string username = this->m_usernameInput->getString();
    if (username.empty()) {
        Notification::create("Enter a username", NotificationIcon::Error)->show();
        return;
    }

    this->m_pendingUsername = username;
    this->setLoading(true);

    auto glm = GameLevelManager::get();
    glm->m_levelManagerDelegate = this;
    glm->getUsers(GJSearchObject::create(SearchType::Users, username));
}

void NewWhitelistUserPopup::searchByAccountID() {
    int accountID = geode::utils::numFromString<int>(this->m_accountIDInput->getString()).unwrapOr(0);
    if (accountID <= 0) {
        Notification::create("Enter a valid account ID", NotificationIcon::Error)->show();
        return;
    }

    this->m_pendingAccountID = accountID;
    this->setLoading(true);

    auto glm = GameLevelManager::get();
    glm->m_userInfoDelegate = this;
    glm->getGJUserInfo(accountID);
}

void NewWhitelistUserPopup::loadLevelsFinished(CCArray* levels, char const* key) {
    this->loadLevelsFinished(levels, key, -1);
}

void NewWhitelistUserPopup::loadLevelsFinished(CCArray* levels, char const* key, int type) {
    auto glm = GameLevelManager::get();
    if (glm->m_levelManagerDelegate == this) glm->m_levelManagerDelegate = nullptr;
    this->setLoading(false);

    GJUserScore* match = nullptr;
    if (levels) {
        //Prefer an exact (case-insensitive) name match, otherwise take the first result
        for (auto score : CCArrayExt<GJUserScore*>(levels)) {
            if (!match) match = score;
            if (geode::utils::string::equalsIgnoreCase(score->m_userName, this->m_pendingUsername)) {
                match = score;
                break;
            }
        }
    }

    if (!match) {
        Notification::create("User not found", NotificationIcon::Error)->show();
        return;
    }

    this->fillFromScore(match);
}

void NewWhitelistUserPopup::loadLevelsFailed(char const* key) {
    this->loadLevelsFailed(key, -1);
}

void NewWhitelistUserPopup::loadLevelsFailed(char const* key, int type) {
    auto glm = GameLevelManager::get();
    if (glm->m_levelManagerDelegate == this) glm->m_levelManagerDelegate = nullptr;
    this->setLoading(false);

    Notification::create("User not found", NotificationIcon::Error)->show();
}

void NewWhitelistUserPopup::getUserInfoFinished(GJUserScore* score) {
    //Other things (e.g. your own profile) can report here too, only take the one we asked for
    if (!score || score->m_accountID != this->m_pendingAccountID) return;

    auto glm = GameLevelManager::get();
    if (glm->m_userInfoDelegate == this) glm->m_userInfoDelegate = nullptr;
    this->setLoading(false);

    this->fillFromScore(score);
}

void NewWhitelistUserPopup::getUserInfoFailed(int id) {
    if (id != this->m_pendingAccountID) return;

    auto glm = GameLevelManager::get();
    if (glm->m_userInfoDelegate == this) glm->m_userInfoDelegate = nullptr;
    this->setLoading(false);

    Notification::create("Account not found", NotificationIcon::Error)->show();
}



//Friend picker

FriendPickerPopup* FriendPickerPopup::create(std::function<void(GJUserScore*)> callback) {
    auto ret = new FriendPickerPopup();
    if (ret->init(std::move(callback))) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

FriendPickerPopup::~FriendPickerPopup() {
    auto glm = GameLevelManager::get();
    if (glm->m_userListDelegate == this) glm->m_userListDelegate = nullptr;
}

bool FriendPickerPopup::init(std::function<void(GJUserScore*)> callback) {
    if (!geode::Popup::init({300, 250})) return false;

    this->setID("friend-picker-popup"_spr);
    this->setTitle("Pick a Friend");
    this->m_callback = std::move(callback);

    this->m_closeBtn->setID("close-button");
    this->m_bgSprite->setID("background-sprite");
    this->m_buttonMenu->setID("main-button-menu");


    this->m_scrollLayer = ScrollLayer::create({270, 185});
    this->m_scrollLayer->setID("scroll-layer");
    this->m_scrollLayer->setZOrder(15);
    this->m_scrollLayer->ignoreAnchorPointForPosition(false);
    this->m_mainLayer->addChildAtPosition(this->m_scrollLayer, Anchor::Center, {-4.25f, -10});

    this->m_scrollbar = geode::Scrollbar::create(this->m_scrollLayer);
    this->m_scrollbar->setID("scrollbar");
    this->m_scrollbar->setZOrder(15);
    this->m_scrollbar->setVisible(false);
    this->m_mainLayer->addChildAtPosition(this->m_scrollbar, Anchor::Right, {-11.5f, -10});

    auto scrollLayerBG = geode::NineSlice::create("square02_001.png");
    scrollLayerBG->setID("scroll-layer-bg");
    scrollLayerBG->setZOrder(10);
    scrollLayerBG->setContentSize({270, 185});
    scrollLayerBG->setOpacity(150);
    this->m_mainLayer->addChildAtPosition(scrollLayerBG, Anchor::Center, {-4.25f, -10});

    this->m_spinner = geode::LoadingSpinner::create(30);
    this->m_spinner->setID("loading-spinner");
    this->m_spinner->setZOrder(20);
    this->m_spinner->setVisible(false);
    this->m_mainLayer->addChildAtPosition(this->m_spinner, Anchor::Center, {0, -10});

    this->m_messageLabel = geode::Label::create("", "bigFont.fnt");
    this->m_messageLabel->setID("message-label");
    this->m_messageLabel->setZOrder(20);
    this->m_messageLabel->setScale(0.4f);
    this->m_messageLabel->setVisible(false);
    this->m_mainLayer->addChildAtPosition(this->m_messageLabel, Anchor::Center, {0, -10});


    if (GJAccountManager::get()->m_accountID <= 0) {
        this->showMessage("Log in to see your friends");
        return true;
    }

    //Use the cached list if the game already loaded it this session
    auto glm = GameLevelManager::get();
    if (auto stored = glm->getStoredUserList(UserListType::Friends)) {
        this->populate(stored);
    } else {
        this->m_spinner->setVisible(true);
        glm->m_userListDelegate = this;
        glm->getUserList(UserListType::Friends);
    }

    return true;
}

void FriendPickerPopup::showMessage(std::string const& message) {
    this->m_spinner->setVisible(false);
    this->m_messageLabel->setString(message.c_str());
    this->m_messageLabel->setVisible(true);
}

void FriendPickerPopup::populate(CCArray* friends) {
    this->m_spinner->setVisible(false);

    auto content = this->m_scrollLayer->m_contentLayer;
    content->removeAllChildrenWithCleanup(true);

    if (!friends || friends->count() == 0) {
        this->showMessage("No friends found");
        return;
    }

    constexpr float rowHeight = 30;
    constexpr float gap = 5;
    constexpr float padding = 7.5f;

    unsigned int count = friends->count();
    float totalHeight = padding * 2 + count * rowHeight + (count - 1) * gap;
    float newHeight = std::max(totalHeight, this->m_scrollLayer->getContentHeight());
    content->setContentHeight(newHeight);

    float y = newHeight - padding - rowHeight / 2;
    for (auto score : CCArrayExt<GJUserScore*>(friends)) {
        auto row = CCNode::create();
        row->setID("friend-row");
        row->setContentSize({250, rowHeight});
        row->setAnchorPoint({0.5f, 0.5f});

        auto bg = geode::NineSlice::create("GJ_square05.png");
        bg->setID("background");
        bg->setScaleMultiplier(0.85f);
        bg->setContentSize(row->getContentSize());
        row->addChildAtPosition(bg, Anchor::Center);

        auto label = geode::Label::create(std::string(score->m_userName), "mdFontB.fnt");
        label->setID("label");
        label->setAnchorPoint({0, 0.5f});
        label->setLimitLabelWidth(150, 0.8f);
        row->addChildAtPosition(label, Anchor::Left, {10, 0});

        auto menu = CCMenu::create();
        menu->setID("menu");
        menu->setContentSize(row->getContentSize());
        row->addChildAtPosition(menu, Anchor::Center);

        //Keep the score alive for the button callback
        Ref<GJUserScore> scoreRef = score;
        auto selectButton = geode::Button::createWithNode(
            ButtonSprite::create("Select", "goldFont.fnt", "GJ_button_01.png", 0.6f),
            [this, scoreRef](geode::Button* button) {
                if (this->m_callback) this->m_callback(scoreRef);
                this->onClose(button);
            }
        );
        selectButton->setID("select-button");
        selectButton->setScale(0.7f);
        menu->addChildAtPosition(selectButton, Anchor::Right, {-35, 0});

        row->setPosition({ content->getContentWidth() / 2, y });
        content->addChild(row);
        y -= rowHeight + gap;
    }

    this->m_scrollLayer->scrollToTop();
    this->m_scrollbar->setVisible(totalHeight > this->m_scrollLayer->getContentHeight());
}

void FriendPickerPopup::getUserListFinished(CCArray* scores, UserListType type) {
    if (type != UserListType::Friends) return;

    auto glm = GameLevelManager::get();
    if (glm->m_userListDelegate == this) glm->m_userListDelegate = nullptr;

    this->populate(scores);
}

void FriendPickerPopup::getUserListFailed(UserListType type, GJErrorCode errorType) {
    if (type != UserListType::Friends) return;

    auto glm = GameLevelManager::get();
    if (glm->m_userListDelegate == this) glm->m_userListDelegate = nullptr;

    if (errorType == GJErrorCode::NotFound) this->showMessage("No friends found");
    else this->showMessage("Couldn't load friends list");
}
