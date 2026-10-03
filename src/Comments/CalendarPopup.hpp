#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <chrono>
#include <functional>
#include <optional>

//Month view date picker
class CalendarPopup : public geode::Popup {
public:
    using Callback = std::function<void(std::optional<std::chrono::year_month_day>)>;

    //minDate/maxDate limit which days can be picked, 0 means no limit
    static CalendarPopup* create(std::string const& title, unsigned int selected, unsigned int minDate, unsigned int maxDate, Callback callback);

protected:
    bool init(std::string const& title, unsigned int selected, unsigned int minDate, unsigned int maxDate, Callback callback);

    void buildMonth();
    void changeMonth(int delta);
    bool isSelectable(std::chrono::year_month_day date);

    Callback m_callback;

    std::chrono::year_month m_shownMonth;
    std::optional<std::chrono::year_month_day> m_selected;
    std::optional<std::chrono::year_month_day> m_minDate;
    std::optional<std::chrono::year_month_day> m_maxDate;

    CCMenu* m_dayMenu = nullptr;
    geode::Label* m_monthLabel = nullptr;
    geode::Label* m_selectedLabel = nullptr;
};
