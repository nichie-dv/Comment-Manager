#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include "Structures.hpp"
#include "../Comments/CommentManager.hpp"

//Level settings live in memory (CommentManager). Only persistent queued levels
//get written to saved.json, as part of the "queue" array.

inline void SetSavedOptions(LevelCommentOptions options) {
    CommentManager::get()->setLevelOptions(options);
}

inline void DeleteSavedOptions(int levelID, std::string levelName) {
    CommentManager::get()->forgetLevelOptions(levelID);
}

//Falls back to the defaults if this level has no settings yet
inline LevelCommentOptions GetSavedOptions(LevelCommentOptions const& defaults) {
    LevelCommentOptions options = CommentManager::get()->getLevelOptions(defaults.m_levelID).value_or(defaults);

    //Keep the identifying fields current
    options.m_levelID = defaults.m_levelID;
    options.m_levelName = defaults.m_levelName;
    options.m_creatorName = defaults.m_creatorName;
    return options;
}
