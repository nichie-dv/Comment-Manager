#pragma once

enum BaseTogglerType {
    DisableComments,
    UseWordlist,
    CaseSensitive,
    UseWhitelist,
    DeleteBlocked
};

enum AdvancedTogglerType {
    Persistent,
    UseDates,
    UseRegex,
    KeepLogs
};

//helper for fmt format
inline int format_as(BaseTogglerType type) { return static_cast<int>(type); }
inline int format_as(AdvancedTogglerType type) { return static_cast<int>(type); }
