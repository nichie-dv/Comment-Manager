#include "CommentManager.hpp"

#include <Geode/Geode.hpp>
#include <arc/prelude.hpp>

using namespace geode::prelude;

#include "WordlistPopup.hpp"
#include "WhitelistPopup.hpp"
#include "AdvancedSettingsPopup.hpp"

#include "Enums.hpp"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <fmt/chrono.h>
#include <regex>


#include "../Util/OptionsSaveLoad.hpp"
#include "../Util/Structures.hpp"
#include "../Util/DefaultWordlist.hpp"
#include "../Util/DateUtils.hpp"



//Manager popup


static std::string buildInfoMarkdown(LevelCommentOptions const& options, bool queued) {
    auto state = [](bool on) { return on ? "<cg>On</c>" : "<cr>Off</c>"; };

    auto enabledWords = std::count_if(options.m_wordlist.begin(), options.m_wordlist.end(), [](auto const& w) { return w.m_enabled; });
    auto enabledUsers = std::count_if(options.m_whitelist.begin(), options.m_whitelist.end(), [](auto const& u) { return u.m_enabled; });

    std::string md = fmt::format(
        "\xC2\xA0\n\n"
        "<cy>{}</c> by <cj>{}</c> (ID {}), {}.\n\n"
        "Comments are checked top to bottom, and the first rule that applies decides what happens to them.\n\n",
        options.m_levelName, options.m_creatorName, options.m_levelID,
        queued ? "<cg>queued</c>" : "<cr>not queued</c>"
    );

    md += "## Filters\n\n";
    md += fmt::format(
        "<cg>Use Whitelist</c> ({}): comments from whitelisted users are <cg>never deleted</c>. "
        "{} of {} users enabled.\n\n",
        state(options.m_useWhitelist), enabledUsers, options.m_whitelist.size()
    );
    md += fmt::format(
        "<cr>Disable Comments</c> ({}): deletes <cr>every comment</c>, except from whitelisted users when the whitelist is on.\n\n",
        state(options.m_disableComments)
    );
    md += fmt::format(
        "<co>Delete Blocked</c> ({}): deletes comments from users on your blocked list.\n\n",
        state(options.m_deleteBlocked)
    );
    md += fmt::format(
        "<cy>Use Wordlist</c> ({}): deletes comments containing any enabled word. "
        "{} of {} words enabled.{}\n\n",
        state(options.m_useWordlist), enabledWords, options.m_wordlist.size(),
        options.m_useRegex ? " <cf>Ignored while regex is on.</c>" : ""
    );
    md += fmt::format(
        "<cp>Case Sensitive</c> ({}): whether the wordlist and regex care about capital letters.\n\n",
        state(options.m_caseSensitive)
    );

    md += "## Advanced\n\n";
    md += fmt::format(
        "<cy>Persistent</c> ({}): keep this level in the queue after it's processed.\n\n",
        state(options.m_persistent)
    );
    md += fmt::format(
        "<cg>Use Regex</c> ({}): match comments against your regex instead of the wordlist. {}\n\n",
        state(options.m_useRegex),
        options.m_regex.empty() ? "<cf>No regex set.</c>" : fmt::format("Current: `{}`", options.m_regex)
    );
    md += fmt::format(
        "<cj>Use Date Range</c> ({}): only check comments posted between <cf>{}</c> and <cf>{}</c>. "
        "Dates are approximate since GD only gives a comment's age.\n\n",
        state(options.m_useDates), DateUtils::format(options.m_startDate), DateUtils::format(options.m_endDate)
    );
    md += fmt::format(
        "<co>Keep Logs</c> ({}): save every deleted comment to a log file for this level.\n\n",
        state(options.m_keepLogs)
    );

    md += "## Queue\n\n"
          "<cg>Schedule</c> adds this level to the queue. Comments are deleted one at a time using the delay "
          "from the mod settings. Progress can be watched in the <cj>queue visualizer</c>.\n";

    return md;
}

CommentManagerPopup* CommentManagerPopup::create(GJGameLevel* level) {
    auto ret = new CommentManagerPopup();
    if (ret->init(level)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool CommentManagerPopup::init(GJGameLevel* level) {
    if (!geode::Popup::init({440, 280})) return false;

    this->setID("comment-settings-popup"_spr);
    this->m_level = level;



    //Load saved options, or the defaults if this level has none yet
    LevelCommentOptions defaults;

    defaults.m_levelID = level->m_levelID;
    defaults.m_levelName = level->m_levelName;
    defaults.m_creatorName = level->m_creatorName;
    defaults.m_wordlist = DefaultWordlist;
    this->m_commentOptions = GetSavedOptions(defaults);
    this->setTitle("Comment Manager");
    this->m_title->setID("title");

    this->m_buttonMenu->setID("main-button-menu");
    this->m_closeBtn->setID("close-button");
    this->m_bgSprite->setID("background-sprite");

    //Default settings button
    auto resetButton = geode::Button::createWithSpriteFrameName(
        "GJ_updateBtn_001.png",
        [this](geode::Button* button) {
            geode::createQuickPopup(
                "Reset Settings",
                "Are you sure you want to <cr>reset all "
                "<cr>settings</c> to their default values?",
                "Yes", "Cancel",
                [this](FLAlertLayer* popup, bool cancel) {
                    if (cancel) return;

                    //Reset settings here
                    LevelCommentOptions options;
                    options.m_levelID = this->m_commentOptions.m_levelID;
                    options.m_levelName = this->m_commentOptions.m_levelName;
                    options.m_creatorName = this->m_commentOptions.m_creatorName;
                    options.m_wordlist = DefaultWordlist;

                    this->m_commentOptions = options;



                    this->m_disableCommentsToggler->toggle(options.m_disableComments);
                    this->m_useWordlistToggler->toggle(options.m_useWordlist);
                    this->m_caseSensitiveToggler->toggle(options.m_caseSensitive);
                    this->m_useWhitelistToggler->toggle(options.m_useWhitelist);
                    
                    SetSavedOptions(options);
                }, true, true
            );
        }
    );
    resetButton->setScale(0.8f);
    resetButton->setID("reset-button");
    this->m_buttonMenu->addChildAtPosition(resetButton, Anchor::BottomLeft);


    //Toggles
    CCMenu* togglerMenu = CCMenu::create();
    togglerMenu->setID("toggler-menu");
    togglerMenu->setZOrder(12);
    togglerMenu->setLayout(AxisLayout::create(Axis::Column)
        ->setAutoGrowAxis(true)
        ->setAxisReverse(true)
    );
    this->m_mainLayer->addChildAtPosition(togglerMenu, Anchor::Left, {30, 10});


    //They all need to be wrapped in a node to include label

    //disable
    {
        
        auto base = CCMenu::create();
        base->setContentSize({35, 35});
        base->setID(fmt::format("toggler-base-{}", BaseTogglerType::DisableComments));
        
        this->m_disableCommentsToggler = CCMenuItemToggler::createWithStandardSprites(
            this,
            menu_selector(CommentManagerPopup::onTogglerActivated),
            1
        );
        this->m_disableCommentsToggler->setID("disable-comments");
        this->m_disableCommentsToggler->setTag(BaseTogglerType::DisableComments);
        this->m_disableCommentsToggler->toggle(this->m_commentOptions.m_disableComments);
        this->m_disableCommentsToggler->setCascadeColorEnabled(true);
        base->addChildAtPosition(this->m_disableCommentsToggler, Anchor::Center);

        auto label = geode::Label::create("Disable Comments", "bigFont.fnt");
        label->setID("toggler-label");
        label->setAlignment(geode::Label::Alignment::Left);
        label->setAnchorPoint({0, 0.5f});
        label->setScale(0.75f);

        base->addChildAtPosition(label, Anchor::Center, {20, 0});

        togglerMenu->addChild(base);
    }

    //wordlist
    {

        auto base = CCMenu::create();
        base->setContentSize({35, 35});
        base->setID(fmt::format("toggler-base-{}", BaseTogglerType::UseWordlist));

        this->m_useWordlistToggler = CCMenuItemToggler::createWithStandardSprites(
            this,
            menu_selector(CommentManagerPopup::onTogglerActivated),
            1
        );
        this->m_useWordlistToggler->setID("use-wordlist");
        this->m_useWordlistToggler->setTag(BaseTogglerType::UseWordlist);
        this->m_useWordlistToggler->toggle(this->m_commentOptions.m_useWordlist);
        this->m_useWordlistToggler->setCascadeColorEnabled(true);
        base->addChildAtPosition(this->m_useWordlistToggler, Anchor::Center);

        auto label = geode::Label::create("Use Wordlist", "bigFont.fnt");
        label->setID("toggler-label");
        label->setAlignment(geode::Label::Alignment::Left);
        label->setAnchorPoint({0, 0.5f});
        label->setScale(0.75f);

        base->addChildAtPosition(label, Anchor::Center, {20, 0});

        togglerMenu->addChild(base);
    }


    //casing
    {

        auto base = CCMenu::create();
        base->setContentSize({35, 35});
        base->setID(fmt::format("toggler-base-{}", BaseTogglerType::CaseSensitive));

        this->m_caseSensitiveToggler = CCMenuItemToggler::createWithStandardSprites(
            this,
            menu_selector(CommentManagerPopup::onTogglerActivated),
            1
        );
        this->m_caseSensitiveToggler->setID("case-sensitive");
        this->m_caseSensitiveToggler->setTag(BaseTogglerType::CaseSensitive);
        this->m_caseSensitiveToggler->toggle(this->m_commentOptions.m_caseSensitive);
        this->m_caseSensitiveToggler->setCascadeColorEnabled(true);
        base->addChildAtPosition(this->m_caseSensitiveToggler, Anchor::Center);

        auto label = geode::Label::create("Case Sensitive", "bigFont.fnt");
        label->setID("toggler-label");
        label->setAlignment(geode::Label::Alignment::Left);
        label->setAnchorPoint({0, 0.5f});
        label->setScale(0.75f);

        base->addChildAtPosition(label, Anchor::Center, {20, 0});

        togglerMenu->addChild(base);
    }


    //regex
    {

        auto base = CCMenu::create();
        base->setContentSize({35, 35});
        base->setID(fmt::format("toggler-base-{}", BaseTogglerType::UseWhitelist));

        this->m_useWhitelistToggler = CCMenuItemToggler::createWithStandardSprites(
            this,
            menu_selector(CommentManagerPopup::onTogglerActivated),
            1
        );
        this->m_useWhitelistToggler->setID("use-whitelist");
        this->m_useWhitelistToggler->setTag(BaseTogglerType::UseWhitelist);
        this->m_useWhitelistToggler->toggle(this->m_commentOptions.m_useWhitelist);
        this->m_useWhitelistToggler->setCascadeColorEnabled(true);
        base->addChildAtPosition(this->m_useWhitelistToggler, Anchor::Center);

        auto label = geode::Label::create("Use Whitelist", "bigFont.fnt");
        label->setID("toggler-label");
        label->setAlignment(geode::Label::Alignment::Left);
        label->setAnchorPoint({0, 0.5f});
        label->setScale(0.75f);

        base->addChildAtPosition(label, Anchor::Center, {20, 0});

        togglerMenu->addChild(base);
    }

    //blocked users
    {

        auto base = CCMenu::create();
        base->setContentSize({35, 35});
        base->setID(fmt::format("toggler-base-{}", BaseTogglerType::DeleteBlocked));

        this->m_deleteBlockedToggler = CCMenuItemToggler::createWithStandardSprites(
            this,
            menu_selector(CommentManagerPopup::onTogglerActivated),
            1
        );
        this->m_deleteBlockedToggler->setID("delete-blocked");
        this->m_deleteBlockedToggler->setTag(BaseTogglerType::DeleteBlocked);
        this->m_deleteBlockedToggler->toggle(this->m_commentOptions.m_deleteBlocked);
        this->m_deleteBlockedToggler->setCascadeColorEnabled(true);
        base->addChildAtPosition(this->m_deleteBlockedToggler, Anchor::Center);

        auto label = geode::Label::create("Delete Blocked", "bigFont.fnt");
        label->setID("toggler-label");
        label->setAlignment(geode::Label::Alignment::Left);
        label->setAnchorPoint({0, 0.5f});
        label->setScale(0.75f);

        base->addChildAtPosition(label, Anchor::Center, {20, 0});

        togglerMenu->addChild(base);
    }


    


    togglerMenu->updateLayout();



    //Wordlist editor
    auto editWordlistButton = geode::Button::createWithNode(
        ButtonSprite::create("Edit Wordlist", "bigFont.fnt", "GJ_button_05.png", 0.5f),
        [this](geode::Button* button) {
            WordlistPopup::create(this)->show();
        }
    );
    editWordlistButton->setID("edit-wordlist-button");
    this->m_buttonMenu->addChildAtPosition(editWordlistButton, Anchor::BottomRight, {-7.5f - (editWordlistButton->getContentWidth() / 2), 27});


    //Whitelist editor
    auto editWhitelistButton = geode::Button::createWithNode(
        ButtonSprite::create("Edit Whitelist", "bigFont.fnt", "GJ_button_05.png", 0.5f),
        [this](geode::Button* button) {
            WhitelistPopup::create(this)->show();
        }
    );
    editWhitelistButton->setID("edit-whitelist-button");
    this->m_buttonMenu->addChildAtPosition(editWhitelistButton, Anchor::BottomRight, {-7.5f - (editWhitelistButton->getContentWidth() / 2), 70});
    
    //Advanced settings
    auto advancedSettingsButton = geode::Button::createWithNode(
        ButtonSprite::create("Advanced", "bigFont.fnt", "GJ_button_05.png", 0.5f),
        [this](geode::Button* button) {
            AdvancedSettingsPopup::create(this)->show();
        }
    );
    advancedSettingsButton->setID("advanced-settings-button");
    this->m_buttonMenu->addChildAtPosition(advancedSettingsButton, Anchor::BottomRight, {-7.5f - (advancedSettingsButton->getContentWidth() / 2), 113});




    //Schedule/unschedule this level in the queue
    bool queued = CommentManager::get()->isLevelInQueue(this->m_commentOptions.m_levelID);
    auto scheduleSprite = ButtonSprite::create(
        queued ? "Unschedule" : "Schedule", 150, true, "bigFont.fnt",
        queued ? "GJ_button_06.png" : "GJ_button_01.png", 30, 0.6f
    );
    auto scheduleButton = geode::Button::createWithNode(
        scheduleSprite,
        [this, scheduleSprite](geode::Button* button) {
            auto manager = CommentManager::get();
            int levelID = this->m_commentOptions.m_levelID;

            if (manager->isLevelInQueue(levelID)) {
                manager->requestRemoveQueueItem(levelID);
                Notification::create("Removed from the queue", NotificationIcon::Info)->show();
            } else {
                auto const& options = this->m_commentOptions;
                if (
                    options.m_useDates
                    && options.m_startDate != 0
                    && options.m_endDate != 0
                    && options.m_startDate > options.m_endDate
                ) {
                    Notification::create("Start date is after the end date", NotificationIcon::Error)->show();
                    return;
                }

                SetSavedOptions(options);
                manager->tryInsertQueueItem(options);
                Notification::create("Added to the queue", NotificationIcon::Success)->show();
            }

            bool nowQueued = manager->isLevelInQueue(levelID);
            scheduleSprite->setString(nowQueued ? "Unschedule" : "Schedule");
            scheduleSprite->updateBGImage(nowQueued ? "GJ_button_06.png" : "GJ_button_01.png");
        }
    );
    scheduleButton->setID("schedule-button");
    this->m_buttonMenu->addChildAtPosition(scheduleButton, Anchor::Bottom, {-55, 25});



    auto infoPopupIcon = geode::Button::createWithSpriteFrameName(
        "GJ_infoIcon_001.png",
        [this](geode::Button* button) {
            bool queued = CommentManager::get()->isLevelInQueue(this->m_commentOptions.m_levelID);
            auto popup = MDPopup::create("Comment Manager Help", buildInfoMarkdown(this->m_commentOptions, queued), "OK");


            //Bug causes text to disappear too early
            auto area = popup->m_mainLayer->getChildByType<MDTextArea>(0);
            auto content = area ? area->getScrollLayer()->m_contentLayer->getChildByType<CCMenu>(0) : nullptr;
            if (content) {
                for (auto child : CCArrayExt<CCNode*>(content->getChildren())) {
                    if (auto label = typeinfo_cast<CCLabelBMFont*>(child)) {
                        label->setAnchorPoint({0, 0});
                    }
                }
            }
            popup->show();
        }
    );
    infoPopupIcon->setID("info-popup-button");
    infoPopupIcon->setZOrder(10);
    this->m_buttonMenu->addChildAtPosition(infoPopupIcon, Anchor::TopRight);

    return true;
}

//Get which toggler it is via tags
void CommentManagerPopup::onTogglerActivated(CCObject* sender) {
    switch (sender->getTag()) {
        case BaseTogglerType::DisableComments:
        this->m_commentOptions.m_disableComments = !this->m_commentOptions.m_disableComments;
        break;

        case BaseTogglerType::UseWordlist:
        this->m_commentOptions.m_useWordlist = !this->m_commentOptions.m_useWordlist;
        this->m_caseSensitiveToggler->setEnabled(this->m_commentOptions.m_useWordlist);
        if (this->m_commentOptions.m_useWordlist) {
            this->m_caseSensitiveToggler->setColor({ 255, 255, 255 });
        } else {
            this->m_caseSensitiveToggler->setColor({ 150, 150, 150 });
        }
        break;

        case BaseTogglerType::CaseSensitive:
        this->m_commentOptions.m_caseSensitive = !this->m_commentOptions.m_caseSensitive;
        break;

        case BaseTogglerType::UseWhitelist:
        this->m_commentOptions.m_useWhitelist = !this->m_commentOptions.m_useWhitelist;
        break;

        case BaseTogglerType::DeleteBlocked:
        this->m_commentOptions.m_deleteBlocked = !this->m_commentOptions.m_deleteBlocked;
        break;

        
    }
    SetSavedOptions(this->m_commentOptions);
}





//Manager
CommentManager* CommentManager::get() {
    static CommentManager instance;
    return &instance;
}

void CommentManager::tryInsertQueueItem(LevelCommentOptions options) {
    std::lock_guard lock(this->m_queueMutex);
    if (this->isLevelInQueue(options.m_levelID)) return;

    this->m_completedLevels.erase(options.m_levelID); 
    this->m_levelQueue.push_back(options);
    this->m_wakeQueue = true;

    log::debug("Added {} to the queue (persistent: {})", options.m_levelID, options.m_persistent);

    if (options.m_persistent) this->savePersistentQueue();
}

void CommentManager::updateQueueItem(LevelCommentOptions const& options) {
    std::lock_guard lock(this->m_queueMutex);
    for (auto& item : this->m_levelQueue) {
        if (item.m_levelID != options.m_levelID) continue;

        bool wasPersistent = item.m_persistent;
        item = options;
        if (wasPersistent || options.m_persistent) this->savePersistentQueue();
        return;
    }
}

std::optional<LevelCommentOptions> CommentManager::getLevelOptions(int levelID) {
    std::lock_guard lock(this->m_queueMutex);
    for (auto const& item : this->m_levelQueue) {
        if (item.m_levelID == levelID) return item;
    }

    auto it = this->m_levelOptions.find(levelID);
    if (it != this->m_levelOptions.end()) return it->second;
    return std::nullopt;
}

void CommentManager::setLevelOptions(LevelCommentOptions const& options) {
    std::lock_guard lock(this->m_queueMutex);
    this->m_levelOptions[options.m_levelID] = options;
    this->updateQueueItem(options); //Saves to "queue" if it's a persistent queued level
}

void CommentManager::forgetLevelOptions(int levelID) {
    std::lock_guard lock(this->m_queueMutex);
    this->m_levelOptions.erase(levelID);
}

void CommentManager::savePersistentQueue() {
    std::lock_guard lock(this->m_queueMutex);

    std::vector<LevelCommentOptions> persistent;
    for (auto const& item : this->m_levelQueue) {
        if (item.m_persistent) persistent.push_back(item);
    }
    Mod::get()->setSavedValue("queue", persistent);
}

void CommentManager::loadPersistentQueue() {
    auto& saved = Mod::get()->getSaveContainer();

    auto list = Mod::get()->getSavedValue<matjson::Value>("queue");
    if (!list.isArray()) return;


    for (auto const& entry : list) {
        auto options = entry.as<LevelCommentOptions>();

        if (options.isErr()) {
            log::warn("Skipping unreadable queue entry: {}", options.unwrapErr());
            continue;
        }

        if (!options.unwrap().m_persistent) continue;

        this->tryInsertQueueItem(options.unwrap());
        log::info("Restored persistent queue item {}", options.unwrap().m_levelID);
    }

    this->savePersistentQueue();
}

geode::Result<LevelCommentOptions, std::string> CommentManager::tryGetQueueItem(int levelID) {
    std::lock_guard lock(this->m_queueMutex);

    for (LevelCommentOptions options : this->m_levelQueue) {
        if (options.m_levelID == levelID) return Ok(options);
    }

    return Err("not found");
}

geode::Result<QueueItemProgress, bool> CommentManager::tryGetQueueProgress(int levelID) {
    if (this->m_progressItem == nullptr) return Err(false);
    if (levelID == this->m_progressItem->m_currentLevelID) return Ok(*this->m_progressItem);
    else return Err(false);
}

void CommentManager::tryRemoveQueueItem(LevelCommentOptions options) {
    std::lock_guard lock(this->m_queueMutex);
    if (!this->isLevelInQueue(options.m_levelID)) return;

    bool wasPersistent = false;
    std::erase_if(this->m_levelQueue, [&options, &wasPersistent](auto const& item) {
        if (item.m_levelID != options.m_levelID) return false;
        wasPersistent = item.m_persistent;
        return true;
    });

    if (wasPersistent) this->savePersistentQueue();
}

void CommentManager::requestRemoveQueueItem(int levelID) {

    {
        std::lock_guard lock(this->m_queueMutex);
        bool wasPersistent = false;
        std::erase_if(this->m_levelQueue, [levelID, &wasPersistent](auto const& item) {
            if (item.m_levelID != levelID) return false;
            wasPersistent = item.m_persistent;
            return true;
        });

        if (wasPersistent) this->savePersistentQueue();
        log::debug("Removed {} from the queue", levelID);
    }

    if (this->m_processingLevelID == levelID) {
        this->m_cancelCurrent = true;
        log::debug("Cancelling processing for {}", levelID);
    }
}

bool CommentManager::isLevelInQueue(int levelID) {
    std::lock_guard lock(this->m_queueMutex);
    return std::ranges::any_of(this->m_levelQueue, [levelID](LevelCommentOptions options) {
        return options.m_levelID == levelID;
    });
}


arc::Future<void> CommentManager::processQueue() {
    while (true) {
        co_await this->runQueueOnceInBackground();

        int waited = 0;
        while (!this->m_wakeQueue && waited < static_cast<int>(this->m_queueTimeout * 1000)) {
            co_await arc::sleep(asp::Duration::fromMillis(250));
            waited += 250;
        }
        
        this->m_wakeQueue = false;
    }
    co_return;
}








namespace {
    //ex: "2026-09-27 17:29:15"
    std::string logTimestamp() {
        std::time_t now = std::time(nullptr);
        std::tm local{};

        #ifdef GEODE_IS_WINDOWS
        localtime_s(&local, &now);
        #else
        localtime_r(&now, &local);
        #endif
        
        return fmt::format("{:%Y-%m-%d %H:%M:%S}", local);
    }

    //keeps one file per level
    void appendCommentLog(int levelID, std::string const& line) {
        auto dir = Mod::get()->getSaveDir() / "logs";
        if (auto res = file::createDirectoryAll(dir); !res) {
            log::warn("Couldn't create the comment log folder: {}", res.unwrapErr());
            return;
        }

        //Geode's file utils can only overwrite, so append with a plain stream
        std::ofstream file(dir / fmt::format("{}.txt", levelID), std::ios::app);
        if (!file) {
            log::warn("Couldn't open the comment log for level {}", levelID);
            return;
        }
        file << line << '\n';
    }
}





//Decides which comments on a level get deleted, built once per level so the regex/wordlist aren't rebuilt per comment
class CommentFilter {
public:
    CommentFilter(LevelCommentOptions const& options, std::unordered_set<int> blockedAccounts) : m_options(options), m_blockedAccounts(std::move(blockedAccounts)) {

        //Regex
        if (options.m_useRegex && !options.m_regex.empty()) {
            auto flags = std::regex::ECMAScript;
            if (!options.m_caseSensitive) flags |= std::regex::icase;


            //Given the OK by 2 index staff to do this
            try {
                this->m_regex.emplace(options.m_regex, flags);
            } catch (std::regex_error const& e) {
                log::error("Invalid regex '{}' for level {}: {}", options.m_regex, options.m_levelID, e.what());
            }
        }

        //Wordlist
        for (auto const& word : options.m_wordlist) {
            if (!word.m_enabled || word.m_word.empty()) continue;
            this->m_words.push_back(options.m_caseSensitive ? word.m_word : geode::utils::string::toLower(word.m_word));
        }
    }

    //Returns why the comment should be deleted, or nothing if it should be kept
    std::optional<std::string> getDeleteReason(int64_t unixDate, int accountID, int userID, std::string const& content) const {
        //Whitelist
        if (this->m_options.m_useWhitelist) {
            for (auto const& user : this->m_options.m_whitelist) {
                if (!user.m_enabled) continue;
                if (accountID > 0 && user.m_accountID == static_cast<unsigned>(accountID)) return std::nullopt;
                if (userID > 0 && user.m_userID == static_cast<unsigned>(userID)) return std::nullopt;
            }
        }

        //Date range
        if (this->m_options.m_useDates) {
            if (unixDate == 0) return std::nullopt; //Couldn't read the date, play it safe
            if (this->m_options.m_startDate && unixDate < this->m_options.m_startDate) return std::nullopt;
            if (this->m_options.m_endDate && unixDate > this->m_options.m_endDate) return std::nullopt;
        }

        //Disable comments
        if (m_options.m_disableComments) return "comments disabled";

        //Blocked users
        if (m_options.m_deleteBlocked && m_blockedAccounts.contains(accountID)) return "blocked user";

        //Regex
        if (this->m_options.m_useRegex) {
            if (this->m_regex && std::regex_search(content, *this->m_regex)) return "regex match";
        }
        //Wordlist
        else if (this->m_options.m_useWordlist) {
            auto haystack = this->m_options.m_caseSensitive ? content : geode::utils::string::toLower(content);
            for (auto const& word : m_words) {
                if (haystack.find(word) != std::string::npos) return fmt::format("banned word '{}'", word);
            }
        }

        return std::nullopt;
    }

private:
    LevelCommentOptions const& m_options;
    std::unordered_set<int> m_blockedAccounts;
    std::optional<std::regex> m_regex;
    std::vector<std::string> m_words; //Enabled words, lowercased unless case sensitive
};





arc::Future<std::unordered_set<int>> CommentManager::getBlockedAccounts() {
    struct State {
        std::unordered_set<int> accounts;
        std::atomic<bool> done = false;
    };
    auto state = std::make_shared<State>();

    auto check = [state](bool requestIfMissing) {
        queueInMainThread([state, requestIfMissing] {
            auto GLM = GameLevelManager::get();
            auto list = GLM->getStoredUserList(UserListType::Blocked);
            if (!list) {
                if (requestIfMissing) GLM->getUserList(UserListType::Blocked);
                return;
            }

            for (auto user : CCArrayExt<GJUserScore*>(list)) state->accounts.insert(user->m_accountID);
            state->done = true;
        });
    };

    check(true);

    int waited = 0;
    while (!state->done) {
        if (waited >= static_cast<int>(this->m_pageResponseTimeout * 1000)) {
            log::warn("Couldn't load the blocked users list, blocked users won't be deleted");
            co_return std::unordered_set<int>{};
        }

        co_await arc::sleep(asp::Duration::fromMillis(500));
        waited += 500;
        check(false);
    }

    log::debug("Loaded {} blocked users", state->accounts.size());
    co_return state->accounts;
}




arc::Future<bool> CommentManager::runQueueOnceInBackground() {
    std::vector<LevelCommentOptions> queue;


    //Update setting values
    this->m_requestTimeout = Mod::get()->getSettingValue<float>("comment-delete-timeout");
    this->m_queueItemTimeout = Mod::get()->getSettingValue<float>("queue-item-timeout");
    this->m_queueTimeout = Mod::get()->getSettingValue<float>("queue-timeout");





    {
        std::lock_guard lock(this->m_queueMutex);
        queue = this->m_levelQueue;
    }

    for (auto item : queue) {
        if (!this->isLevelInQueue(item.m_levelID)) continue;

        this->m_cancelCurrent = false;
        this->m_processingLevelID = item.m_levelID;

        log::debug("Starting fetch for {}", item.m_levelID);

        delete this->m_progressItem;
        this->m_progressItem = new QueueItemProgress();
        this->m_progressItem->m_currentLevelID = item.m_levelID;


        //First populate a list of all comments on a level (best? no)
        auto GLM = GameLevelManager::get();
        GLM->m_levelCommentDelegate = this;


        //Get all pages
        int index = 0;
        while (!this->m_cancelCurrent) {
            log::debug("Attempting to fetch page {}", index);

            this->m_tempFilled = false;
            queueInMainThread([levelID = item.m_levelID, index] {
                GameLevelManager::get()->getLevelComments(levelID, index, 0, 0, CommentKeyType::Level);
            });

            if (co_await this->isCommentPageEmpty()) break;
            if (this->m_cancelCurrent) break;


            index++;
            co_await arc::sleep(asp::Duration::fromMillis(static_cast<int>(this->m_pageGetTimeout * 1000)));
        }

        //reset delegate
        if (GLM->m_levelCommentDelegate == this) GLM->m_levelCommentDelegate = nullptr;



        this->m_progressItem->m_tToDo = this->m_currentComments.size();
        if (!this->m_cancelCurrent) log::debug("Starting processing for {}", item.m_levelID);

        //Settings may have been changed since the queue was copied
        auto latestOptions = this->tryGetQueueItem(item.m_levelID);
        if (latestOptions.isOk()) item = latestOptions.unwrap();

        //Get blocked accounts
        std::unordered_set<int> blockedAccounts;
        if (item.m_deleteBlocked && !item.m_disableComments) blockedAccounts = co_await this->getBlockedAccounts();



        CommentFilter filter(item, std::move(blockedAccounts));
        bool loggedRunHeader = false;

        index = 0;
        for (auto comment : this->m_currentComments) {
            if (this->m_cancelCurrent) break;

            //Custom conversion
            auto uploadDate = comment.m_unixDate;

            //Level comments keep the author's info in m_userScore, not on the comment itself (cool)
            auto userScore = comment.m_comment->m_userScore;

            auto accountID = userScore ? userScore->m_accountID : comment.m_comment->m_accountID;
            auto userID = comment.m_comment->m_userID;
            auto username = userScore ? userScore->m_userName : comment.m_comment->m_userName;

            auto commentID = comment.m_comment->m_commentID;
            auto commentData = comment.m_comment->m_commentString;

            log::debug(
                "Processing comment {}/{}\n"
                "  ID: {} | Uploaded: {} ({} ago)\n"
                "  User: {} (account {}, user {})\n"
                "  Content: {}",
                index + 1, this->m_currentComments.size(),
                commentID, uploadDate, std::string(comment.m_comment->m_uploadDate),
                std::string(username), accountID, userID,
                std::string(commentData)
            );

            index++;
            this->m_progressItem->m_tCompleted++;
            this->m_progressItem->m_progress = (static_cast<float>(this->m_progressItem->m_tCompleted) / static_cast<float>(this->m_progressItem->m_tToDo)) * 100;

            auto reason = filter.getDeleteReason(uploadDate, accountID, userID, commentData);
            if (!reason) continue;

            if (item.m_keepLogs) {
                log::debug("Deleting comment {} by {} ({}): {}", commentID, std::string(username), *reason, std::string(commentData));

                //Run header in file, once per queue request
                if (!loggedRunHeader) {
                    appendCommentLog(item.m_levelID, fmt::format("\n=== {} ({}) - run started {} ===", item.m_levelName, item.m_levelID, logTimestamp()));
                    loggedRunHeader = true;
                }

                //append log
                appendCommentLog(item.m_levelID, fmt::format(
                    "[{}] #{} by {} (account {}, user {}) | posted ~{} ago | {}\n    {}",
                    logTimestamp(), commentID, std::string(username), accountID, userID,
                    std::string(comment.m_comment->m_uploadDate), *reason, std::string(commentData)
                ));
            }

            //Delete Comment
            queueInMainThread([commentID, levelID = item.m_levelID] {
                GameLevelManager::get()->deleteComment(commentID, CommentType::Level, levelID);
            });

            //Unsure how sophisticated robs bot detection is, but ill add a tiny bit of randomness to each request just in case
            float jitter = random::generate<float>(0, this->m_requestTimeout * 0.25f);
            co_await arc::sleep(asp::Duration::fromMillis(static_cast<int>((this->m_requestTimeout + jitter) * 1000)));
        }

        if (Mod::get()->getSettingValue<bool>("queue-update-notifications") && !item.m_persistent) {
            geode::queueInMainThread([levelID = this->m_processingLevelID.load()] {
                geode::Notification::create(
                    fmt::format("Level {} completed processing.", levelID),
                    NotificationIcon::Success
                )->show();
            });
        }

        //Flush storage
        this->m_currentComments.clear();
        delete this->m_progressItem;
        this->m_progressItem = nullptr;

        this->m_processingLevelID = -1;
        log::debug("flushing storage for {}", item.m_levelID);

        if (this->m_cancelCurrent) {
            this->m_cancelCurrent = false;
            log::debug("Processing for {} was cancelled", item.m_levelID);
            continue;
        }

        //Settings may have changed while this level was processing, so check the live copy
        auto latest = this->tryGetQueueItem(item.m_levelID);
        bool persistent = latest.isOk() ? latest.unwrap().m_persistent : item.m_persistent;

        if (!persistent) {

            //Delete queue item
            DeleteSavedOptions(item.m_levelID, item.m_levelName);

            {
                std::lock_guard lock(this->m_queueMutex);
                this->m_completedLevels.insert(item.m_levelID);
                this->tryRemoveQueueItem(item);
            }
        }


        co_await arc::sleep(asp::Duration::fromMillis(static_cast<int>(this->m_queueItemTimeout * 1000)));
    }

    co_return true;
}

arc::Future<bool> CommentManager::isCommentPageEmpty() {
    int waited = 0;
    while (!this->m_tempFilled) {
        if (waited >= static_cast<int>(this->m_pageResponseTimeout * 1000)) {
            log::warn("Timed out waiting for a comment page, stopping here");
            co_return true;
        }

        co_await arc::sleep(asp::Duration::fromMillis(100));
        waited += 100;
    }
    this->m_tempFilled = false;
    co_return this->m_lastPageEmpty.load();
}

void CommentManager::loadCommentsFinished(CCArray* comments, char const* key) {
    this->m_lastPageEmpty = !comments || comments->count() == 0;
    for (auto comment : CCArrayExt<GJComment*>(comments)) {
        CustomCommentData data{comment};
        data.convertDateToUnix();
        this->m_currentComments.push_back(data);
    }
    this->m_tempFilled = true;
}

void CommentManager::loadCommentsFailed(char const* key) {
    log::info("No more comment pages ({})", key ? key : "");
    this->m_lastPageEmpty = true;
    this->m_tempFilled = true;
}