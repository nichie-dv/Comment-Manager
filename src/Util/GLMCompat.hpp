#pragma once

#include <Geode/Geode.hpp>


namespace glm_compat {

    //Inlined on every platform except mac
    //format taken from the older bindings' inline body
    inline std::string getCommentKey(int id, int page, int mode, CommentKeyType type) {
        return fmt::format("comment_{}_{}_{}_{}", id, page, mode, static_cast<int>(type));
    }

    //No mac address in 2.2091. Returning nullptr makes callers fall back to requesting the list
    inline cocos2d::CCArray* getStoredUserList(UserListType type) {
        #ifdef GEODE_IS_MACOS
        return nullptr;

        #else
        return GameLevelManager::get()->getStoredUserList(type);

        #endif
    }
}
