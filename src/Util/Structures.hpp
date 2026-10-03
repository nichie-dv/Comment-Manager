#pragma once

#include <Geode/Geode.hpp>

#include <chrono>
#include <charconv>
#include <string_view>

using namespace geode::prelude;









struct BannedWord {
    std::string m_word = "";
    bool m_enabled = true;
};

inline std::vector<BannedWord> makeWordlist(std::initializer_list<std::string_view> words) {
    std::vector<BannedWord> out;
    out.reserve(words.size());

    for (auto word : words) out.push_back(BannedWord{std::string(word), true});
    
    return out;
}


template<>
struct matjson::Serialize<BannedWord> {
    static Result<BannedWord> fromJson(matjson::Value const& value) {
        GEODE_UNWRAP_INTO(std::string word, value["word"].asString());
        GEODE_UNWRAP_INTO(bool enabled, value["active"].asBool());

        return Ok(BannedWord{word, enabled});
    }

    static matjson::Value toJson(BannedWord const& value) {
        return matjson::makeObject({
            {"word", value.m_word},
            {"active", value.m_enabled}
        });
    }
};








struct WhitelistedItem {
    unsigned int m_accountID = 0;
    unsigned int m_userID = 0;

    bool m_enabled = true;

    std::string m_username;
};

template<>
struct matjson::Serialize<WhitelistedItem> {
    static Result<WhitelistedItem> fromJson(matjson::Value const& value) {
        GEODE_UNWRAP_INTO(unsigned int accountID, value["account-id"].as<unsigned int>());
        GEODE_UNWRAP_INTO(unsigned int userID, value["user-id"].as<unsigned int>());
        bool enabled = value["active"].asBool().unwrapOr(true); //Older saves don't have this
        GEODE_UNWRAP_INTO(std::string username, value["username"].asString());

        return Ok(WhitelistedItem{accountID, userID, enabled, username});
    }

    static matjson::Value toJson(WhitelistedItem const& value) {
        return matjson::makeObject({
            {"account-id", value.m_accountID},
            {"user-id", value.m_userID},
            {"active", value.m_enabled},
            {"username", value.m_username}
        });
    }
};













struct LevelCommentOptions {
    int m_levelID = -1;
    std::string m_levelName = "unknown";
    std::string m_creatorName = "unknown";

    bool m_disableComments = false;

    bool m_useWordlist = true;
    bool m_caseSensitive = false;
    bool m_useWhitelist = true;
    bool m_deleteBlocked = true;

    bool m_persistent = false;
    bool m_keepLogs = false;
    bool m_useRegex = false;
    bool m_useDates = false;
    
    //Timestamps
    unsigned int m_startDate = 0;
    unsigned int m_endDate = 0;

    std::string m_regex = ""; //Single line banned word

    std::vector<WhitelistedItem> m_whitelist;
    std::vector<BannedWord> m_wordlist;
};

template<>
struct matjson::Serialize<LevelCommentOptions> {
    static Result<LevelCommentOptions> fromJson(matjson::Value const& value) {
        LevelCommentOptions out;
        GEODE_UNWRAP_INTO(out.m_levelID, value["level-id"].as<int>());

        //Everything else falls back to the struct default, so older saves still load when fields get added
        out.m_levelName= value["level-name"].asString().unwrapOr(out.m_levelName);
        out.m_creatorName = value["creator-name"].asString().unwrapOr(out.m_creatorName);

        out.m_disableComments = value["disable-comments"].asBool().unwrapOr(out.m_disableComments);
        out.m_useWordlist = value["use-wordlist"].asBool().unwrapOr(out.m_useWordlist);
        out.m_caseSensitive = value["case-sensitive"].asBool().unwrapOr(out.m_caseSensitive);
        out.m_useWhitelist = value["use-whitelist"].asBool().unwrapOr(out.m_useWhitelist);
        out.m_deleteBlocked = value["delete-blocked"].asBool().unwrapOr(out.m_deleteBlocked);

        out.m_persistent = value["persistent"].asBool().unwrapOr(out.m_persistent);
        out.m_keepLogs = value["logs"].asBool().unwrapOr(out.m_keepLogs);
        out.m_useRegex = value["use-regex"].asBool().unwrapOr(out.m_useRegex);
        out.m_useDates = value["use-dates"].asBool().unwrapOr(out.m_useDates);

        out.m_regex = value["regex"].asString().unwrapOr(out.m_regex);
        out.m_startDate = value["start-date"].as<unsigned int>().unwrapOr(out.m_startDate);
        out.m_endDate = value["end-date"].as<unsigned int>().unwrapOr(out.m_endDate);
        out.m_whitelist = value["whitelist"].as<std::vector<WhitelistedItem>>().unwrapOr(out.m_whitelist);
        out.m_wordlist = value["wordlist"].as<std::vector<BannedWord>>().unwrapOr(out.m_wordlist);

        return Ok(out);
    }

    static matjson::Value toJson(LevelCommentOptions const& value) {
        return matjson::makeObject({
            {"level-id", value.m_levelID},
            {"level-name", value.m_levelName},
            {"creator-name", value.m_creatorName},
            
            {"disable-comments", value.m_disableComments},
            {"use-wordlist", value.m_useWordlist},
            {"case-sensitive", value.m_caseSensitive},
            {"use-whitelist", value.m_useWhitelist},
            {"delete-blocked", value.m_deleteBlocked},

            {"persistent", value.m_persistent},
            {"logs", value.m_keepLogs},
            {"use-regex", value.m_useRegex},
            {"use-dates", value.m_useDates},

            {"start-date", value.m_startDate},
            {"end-date", value.m_endDate},
            {"regex", value.m_regex},
            {"whitelist", value.m_whitelist},
            {"wordlist", value.m_wordlist}
        });
    }
};










struct CustomCommentData {
   GJComment* m_comment;
   int64_t m_unixDate = 0;

    //GD only gives a relative age like "5 hours" or "2 years", so this is approximate
    //(months/years are treated as 30/365 days)
    void convertDateToUnix() {
        std::string age = this->m_comment->m_uploadDate;

        int64_t amount = 0;
        auto [ptr, ec] = std::from_chars(age.data(), age.data() + age.size(), amount);
        if (ec != std::errc()) {
            log::warn("Couldn't parse comment date '{}'", age);
            this->m_unixDate = 0;
            return;
        }

        std::string_view u(ptr, age.data() + age.size() - ptr);
        while (!u.empty() && u.front() == ' ') u.remove_prefix(1);
        int64_t seconds = 0;
        if (u.starts_with("second")) seconds = 1;
        else if (u.starts_with("minute")) seconds = 60;
        else if (u.starts_with("hour")) seconds = 60 * 60;
        else if (u.starts_with("day")) seconds = 60 * 60 * 24;
        else if (u.starts_with("week")) seconds = 60 * 60 * 24 * 7;
        else if (u.starts_with("month")) seconds = 60 * 60 * 24 * 30;
        else if (u.starts_with("year")) seconds = 60 * 60 * 24 * 365;
        else log::warn("Unknown comment date unit '{}'", u);

        int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        this->m_unixDate = now - amount * seconds;
    }
};







struct QueueItemProgress {
    int m_currentLevelID = -1;
    float m_progress = 0;

    int m_tCompleted = 0;
    int m_tToDo = 0;
};