#pragma once

#include <Geode/Geode.hpp>

#include <array>
#include <chrono>
#include <string>

//All dates are whole days in UTC
namespace DateUtils {
    inline std::chrono::year_month_day fromUnix(int64_t seconds) {
        return std::chrono::year_month_day{
            std::chrono::floor<std::chrono::days>(std::chrono::sys_seconds{std::chrono::seconds{seconds}})
        };
    }

    //Midnight at the start of the day
    inline int64_t toUnix(std::chrono::year_month_day date) {
        return std::chrono::sys_days{date}.time_since_epoch() / std::chrono::seconds{1};
    }

    //Last second of the day
    inline int64_t toUnixEndOfDay(std::chrono::year_month_day date) {
        return toUnix(date) + 60 * 60 * 24 - 1;
    }

    inline std::chrono::year_month_day today() {
        return std::chrono::year_month_day{std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())};
    }

    inline char const* monthName(std::chrono::month month, bool shortName = false) {
        static constexpr std::array<char const*, 12> names = {
            "January", "February", "March", "April", "May", "June",
            "July", "August", "September", "October", "November", "December"
        };
        static constexpr std::array<char const*, 12> shortNames = {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };
        unsigned index = static_cast<unsigned>(month) - 1;
        return shortName ? shortNames[index] : names[index];
    }

    //e.g. "Sep 27, 2026", or "Any" when unset
    inline std::string format(unsigned int seconds) {
        if (seconds == 0) return "Any";

        auto date = fromUnix(seconds);
        return fmt::format(
            "{} {}, {}",
            monthName(date.month(), true),
            static_cast<unsigned>(date.day()),
            static_cast<int>(date.year())
        );
    }
}
