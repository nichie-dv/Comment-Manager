#include "CalendarPopup.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include "../Util/DateUtils.hpp"

using namespace std::chrono;

//Grid layout, measured from the top of the popup
constexpr float cellWidth = 30;
constexpr float cellHeight = 24;
constexpr float monthRowY = -45;
constexpr float weekdayRowY = -66;
constexpr float firstRowY = -86;

CalendarPopup* CalendarPopup::create(std::string const& title, unsigned int selected, unsigned int minDate, unsigned int maxDate, Callback callback) {
    auto ret = new CalendarPopup();
    if (ret->init(title, selected, minDate, maxDate, std::move(callback))) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool CalendarPopup::init(std::string const& title, unsigned int selected, unsigned int minDate, unsigned int maxDate, Callback callback) {
    if (!geode::Popup::init({250, 280}, "GJ_square04.png")) return false;

    this->setID("calendar-popup"_spr);
    this->setTitle(title);
    this->m_callback = std::move(callback);

    this->m_closeBtn->setID("close-button");
    this->m_bgSprite->setID("background-sprite");
    this->m_title->setID("title");
    this->m_buttonMenu->setID("main-button-menu");


    if (selected != 0) this->m_selected = DateUtils::fromUnix(selected);
    if (minDate != 0) this->m_minDate = DateUtils::fromUnix(minDate);
    if (maxDate != 0) this->m_maxDate = DateUtils::fromUnix(maxDate);

    //Open on the selected day's month, otherwise the current month
    auto start = this->m_selected.value_or(DateUtils::today());
    this->m_shownMonth = start.year() / start.month();


    //Month header
    auto prevSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    prevSpr->setScale(0.45f);
    auto prevButton = CCMenuItemExt::createSpriteExtra(
        prevSpr,
        [this](CCMenuItemSpriteExtra* sender) { this->changeMonth(-1); }
    );
    prevButton->setID("prev-month-button");
    this->m_buttonMenu->addChildAtPosition(prevButton, Anchor::Top, {-95, monthRowY});

    auto nextSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    nextSpr->setScale(0.45f);
    nextSpr->setFlipX(true);
    auto nextButton = CCMenuItemExt::createSpriteExtra(
        nextSpr,
        [this](CCMenuItemSpriteExtra* sender) { this->changeMonth(1); }
    );
    nextButton->setID("next-month-button");
    this->m_buttonMenu->addChildAtPosition(nextButton, Anchor::Top, {95, monthRowY});

    this->m_monthLabel = geode::Label::create("", "goldFont.fnt");
    this->m_monthLabel->setID("month-label");
    this->m_monthLabel->setScale(0.6f);
    this->m_mainLayer->addChildAtPosition(this->m_monthLabel, Anchor::Top, {0, monthRowY});


    //Weekday names, weeks start on Sunday
    constexpr std::array<char const*, 7> weekdays = {"S", "M", "T", "W", "R", "F", "S"};
    for (int i = 0; i < 7; i++) {
        auto label = geode::Label::create(weekdays[i], "bigFont.fnt");
        label->setID(fmt::format("label-{}", weekdays[i]));
        label->setScale(0.4f);
        label->setOpacity(160);
        this->m_mainLayer->addChildAtPosition(label, Anchor::Top, {(i - 3) * cellWidth, weekdayRowY});
    }


    //Day grid gets rebuilt every time the month changes
    this->m_dayMenu = CCMenu::create();
    this->m_dayMenu->setID("day-menu");
    this->m_dayMenu->setZOrder(10);
    this->m_dayMenu->setContentSize(this->m_mainLayer->getContentSize());
    this->m_dayMenu->setPosition({0, 0});
    this->m_mainLayer->addChild(this->m_dayMenu);


    //Footer
    this->m_selectedLabel = geode::Label::create("", "bigFont.fnt");
    this->m_selectedLabel->setID("selected-label");
    this->m_selectedLabel->setScale(0.4f);
    this->m_mainLayer->addChildAtPosition(this->m_selectedLabel, Anchor::Bottom, {0, 50});


    
    auto clearButton = geode::Button::createWithNode(
        ButtonSprite::create("Clear", "goldFont.fnt", "GJ_button_06.png", 0.7f),
        [this](geode::Button* button) {
            if (this->m_callback) this->m_callback(std::nullopt);
            this->onClose(button);
        }
    );
    clearButton->setID("clear-button");
    this->m_buttonMenu->addChildAtPosition(clearButton, Anchor::Bottom, {-50, 22});



    auto confirmButton = geode::Button::createWithNode(
        ButtonSprite::create("OK", "goldFont.fnt", "GJ_button_01.png", 0.7f),
        [this](geode::Button* button) {
            if (!this->m_selected) {
                Notification::create("Pick a day first", NotificationIcon::Error)->show();
                return;
            }
            if (this->m_callback) this->m_callback(this->m_selected);
            this->onClose(button);
        }
    );
    confirmButton->setID("confirm-button");
    this->m_buttonMenu->addChildAtPosition(confirmButton, Anchor::Bottom, {50, 22});


    this->buildMonth();

    return true;
}

bool CalendarPopup::isSelectable(year_month_day date) {
    if (this->m_minDate && sys_days{date} < sys_days{*this->m_minDate}) return false;
    if (this->m_maxDate && sys_days{date} > sys_days{*this->m_maxDate}) return false;
    return true;
}

void CalendarPopup::changeMonth(int delta) {
    this->m_shownMonth += months{delta};
    this->buildMonth();
}

void CalendarPopup::buildMonth() {
    this->m_dayMenu->removeAllChildrenWithCleanup(true);

    this->m_monthLabel->setString(fmt::format(
        "{} {}",
        DateUtils::monthName(this->m_shownMonth.month()),
        static_cast<int>(this->m_shownMonth.year())
    ).c_str());

    this->m_selectedLabel->setString(this->m_selected
        ? fmt::format("Selected: {}", DateUtils::format(DateUtils::toUnix(*this->m_selected))).c_str()
        : "No day selected"
    );

    auto size = this->m_dayMenu->getContentSize();
    auto today = DateUtils::today();

    //Column of the 1st (0 = Sunday), and how many days this month has (handles leap years)
    auto firstDay = this->m_shownMonth / day{1};
    unsigned firstColumn = weekday{sys_days{firstDay}}.c_encoding();
    unsigned dayCount = static_cast<unsigned>((this->m_shownMonth / last).day());

    for (unsigned d = 1; d <= dayCount; d++) {
        auto date = this->m_shownMonth / day{d};
        unsigned cell = firstColumn + d - 1;
        int column = cell % 7;
        int row = cell / 7;

        bool selectable = this->isSelectable(date);
        bool isSelected = this->m_selected && *this->m_selected == date;
        bool isToday = date == today;

        auto node = CCNode::create();
        node->setContentSize({cellWidth - 3, cellHeight - 3});

        auto bg = geode::NineSlice::create("square02_small.png");
        bg->setContentSize(node->getContentSize());
        bg->setOpacity(isSelected ? 255 : 60);
        if (isSelected) bg->setColor({ 40, 200, 70 });
        node->addChildAtPosition(bg, Anchor::Center);

        auto label = geode::Label::create(fmt::format("{}", d), "bigFont.fnt");
        label->setScale(0.4f);
        if (!selectable) label->setColor({ 110, 110, 110 });
        else if (isToday) label->setColor({ 255, 220, 60 });
        node->addChildAtPosition(label, Anchor::Center);

        auto dayButton = CCMenuItemExt::createSpriteExtra(
            node,
            [this, date](CCMenuItemSpriteExtra* sender) {
                this->m_selected = date;
                this->buildMonth();
            }
        );
        dayButton->setID(fmt::format("day-{}", d));
        dayButton->setEnabled(selectable);
        dayButton->setPosition({
            size.width / 2 + (column - 3) * cellWidth,
            size.height + firstRowY - row * cellHeight
        });
        this->m_dayMenu->addChild(dayButton);
    }
}
