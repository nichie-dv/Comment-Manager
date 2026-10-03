#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include "../Util/Structures.hpp"

#include <atomic>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>


//Manager popup

class CommentManagerPopup : public geode::Popup {
public:
    static CommentManagerPopup* create(GJGameLevel* level);
    LevelCommentOptions getOptions() { return this->m_commentOptions; }
    void setOptions(LevelCommentOptions options) { this->m_commentOptions = std::move(options); }
    GJGameLevel* getLevel() { return this->m_level; }

protected:
    bool init(GJGameLevel* level);
    
    void onTogglerActivated(CCObject* sender);


    CCMenuItemToggler* m_disableCommentsToggler = nullptr;
    CCMenuItemToggler* m_useWordlistToggler = nullptr;
    CCMenuItemToggler* m_caseSensitiveToggler = nullptr;
    CCMenuItemToggler* m_useWhitelistToggler = nullptr;
    CCMenuItemToggler* m_deleteBlockedToggler = nullptr;


    GJGameLevel* m_level;
    LevelCommentOptions m_commentOptions;
};


//Actual comment manager

class CommentManager : public LevelCommentDelegate {
public:
    static CommentManager* get();

    void tryInsertQueueItem(LevelCommentOptions options);
    void updateQueueItem(LevelCommentOptions const& options); //Replaces the queued copy, if the level is queued
    void loadPersistentQueue(); //Re-queues persistent levels from saved.json, call once on load

    //Per-level settings: the queued copy if there is one, otherwise whatever was set this session
    std::optional<LevelCommentOptions> getLevelOptions(int levelID);
    void setLevelOptions(LevelCommentOptions const& options);
    void forgetLevelOptions(int levelID);
    geode::Result<LevelCommentOptions, std::string> tryGetQueueItem(int levelID);
    void tryRemoveQueueItem(LevelCommentOptions options);
    void requestRemoveQueueItem(int levelID);

    geode::Result<QueueItemProgress, bool> tryGetQueueProgress(int levelID);

    bool isLevelInQueue(int levelID);

    //True (once) if the level left the queue because it finished, not because it was removed/cancelled
    bool consumeCompleted(int levelID) {
        std::lock_guard lock(this->m_queueMutex);
        return this->m_completedLevels.erase(levelID) > 0;
    }
    std::vector<LevelCommentOptions> getQueue() {
        std::lock_guard lock(this->m_queueMutex);
        return this->m_levelQueue; //Copy so callers don't hold the lock
    }
    int getQueueSize() {
        std::lock_guard lock(this->m_queueMutex);
        return this->m_levelQueue.size();
    }
    
    QueueItemProgress* m_progressItem = nullptr;
    
    arc::Future<void> processQueue();
    arc::TaskHandle<void> QueueHandle;

    arc::TaskHandle<bool> CurrentQueueTaskHandle; //for aborting runQueueOnceInBackground()


protected:
    arc::Future<bool> runQueueOnceInBackground();
    arc::Future<bool> isCommentPageEmpty();
    arc::Future<std::unordered_set<int>> getBlockedAccounts(); //Account IDs from GD's blocked list, empty if it can't be loaded

    //Only persistent levels are written to saved.json, the rest live in memory
    void savePersistentQueue();



    //Getting comments from delegate
    void loadCommentsFinished(CCArray* comments, char const* key) override;
    void loadCommentsFailed(char const* key) override; 

    std::vector<LevelCommentOptions> m_levelQueue;
    std::recursive_mutex m_queueMutex;
    std::unordered_set<int> m_completedLevels;
    std::unordered_map<int, LevelCommentOptions> m_levelOptions;

    std::atomic<int> m_processingLevelID = -1;
    std::atomic<bool> m_cancelCurrent = false;
    std::atomic<bool> m_wakeQueue = false;

    //empty between processings
    std::vector<CustomCommentData> m_currentComments;
    std::atomic<bool> m_lastPageEmpty = false;
    std::atomic<bool> m_tempFilled = false;

    float m_pageGetTimeout = 4;
    float m_pageResponseTimeout = 15;
    float m_requestTimeout = 1.25f;
    float m_queueItemTimeout = 5;
    float m_queueTimeout = 60;
};