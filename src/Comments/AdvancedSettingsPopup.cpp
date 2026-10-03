#include "AdvancedSettingsPopup.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include "Enums.hpp"
#include "CalendarPopup.hpp"

#include "../Util/DateUtils.hpp"
#include "../Util/OptionsSaveLoad.hpp"

AdvancedSettingsPopup* AdvancedSettingsPopup::create(CommentManagerPopup* popup) {
    auto ret = new AdvancedSettingsPopup();
    if (ret->init(popup)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}


bool AdvancedSettingsPopup::init(CommentManagerPopup* parent) {
    if (!geode::Popup::init({370, 280})) return false;

    this->setID("advanced-settings-popup"_spr);
    this->setTitle("Advanced Settings");
    this->m_parentPopup = parent;
    this->m_options = parent->getOptions();

    this->m_closeBtn->setID("close-button");
    this->m_bgSprite->setID("background-sprite");
    this->m_title->setID("title");
    this->m_buttonMenu->setID("main-button-menu");


    //Toggles
    CCMenu* togglerMenu = CCMenu::create();
    togglerMenu->setID("toggler-menu");
    togglerMenu->setZOrder(12);
    togglerMenu->setScale(0.8f);
    togglerMenu->setLayout(AxisLayout::create(Axis::Column)
        ->setAutoGrowAxis(true)
        ->setAxisReverse(true)
    );
    this->m_mainLayer->addChildAtPosition(togglerMenu, Anchor::Left, {30, 45});

    this->m_persistentToggler = this->createToggler(
        togglerMenu, AdvancedTogglerType::Persistent, "persistent", "Persistent", this->m_options.m_persistent
    );
    this->m_useRegexToggler = this->createToggler(
        togglerMenu, AdvancedTogglerType::UseRegex, "use-regex", "Use Regex", this->m_options.m_useRegex
    );
    this->m_useDatesToggler = this->createToggler(
        togglerMenu, AdvancedTogglerType::UseDates, "use-dates", "Use Date Range", this->m_options.m_useDates
    );
    this->m_keepLogsToggler = this->createToggler(
        togglerMenu, AdvancedTogglerType::KeepLogs, "keep-logs", "Keep Logs", this->m_options.m_keepLogs
    );

    togglerMenu->updateLayout();

    //Regex
    auto regexLabel = geode::Label::create("Regex", "goldFont.fnt");
    regexLabel->setID("regex-label");
    regexLabel->setScale(0.6f);
    this->m_mainLayer->addChildAtPosition(regexLabel, Anchor::Bottom, {0, 117});

    this->m_regexInput = geode::TextInput::create(300, "Regex...", "chatFont.fnt");
    this->m_regexInput->setID("regex-input");
    this->m_regexInput->setCommonFilter(CommonFilter::Any);
    this->m_regexInput->setString(this->m_options.m_regex);
    this->m_regexInput->setCallback([this](std::string const& text) {
        this->m_options.m_regex = text;
        this->save();
    });
    this->m_buttonMenu->addChildAtPosition(this->m_regexInput, Anchor::Bottom, {0, 90});

    this->updateRegexInput();


    //Date range
    auto dateRangeLabel = geode::Label::create("Date Range", "goldFont.fnt");
    dateRangeLabel->setID("date-range-label");
    dateRangeLabel->setScale(0.6f);
    this->m_mainLayer->addChildAtPosition(dateRangeLabel, Anchor::Bottom, {0, 55});

    this->m_startDateSprite = ButtonSprite::create("", 140, true, "bigFont.fnt", "GJ_button_05.png", 30, 0.5f);
    this->m_startDateButton = geode::Button::createWithNode(
        this->m_startDateSprite,
        [this](geode::Button* button) {
            CalendarPopup::create(
                "Start Date",
                this->m_options.m_startDate,
                0,
                this->m_options.m_endDate,
                [this](std::optional<std::chrono::year_month_day> date) {
                    this->m_options.m_startDate = date ? static_cast<unsigned int>(DateUtils::toUnix(*date)) : 0;
                    this->updateDateButtons();
                    this->save();
                }
            )->show();
        }
    );
    this->m_startDateButton->setID("start-date-button");
    this->m_buttonMenu->addChildAtPosition(this->m_startDateButton, Anchor::Bottom, {-80, 25});

    this->m_endDateSprite = ButtonSprite::create("", 140, true, "bigFont.fnt", "GJ_button_05.png", 30, 0.5f);
    this->m_endDateButton = geode::Button::createWithNode(
        this->m_endDateSprite,
        [this](geode::Button* button) {
            CalendarPopup::create(
                "End Date",
                this->m_options.m_endDate,
                this->m_options.m_startDate,

                0,

                [this](std::optional<std::chrono::year_month_day> date) {
                    this->m_options.m_endDate = date ? static_cast<unsigned int>(DateUtils::toUnixEndOfDay(*date)) : 0;
                    this->updateDateButtons();
                    this->save();
                }
            )->show();
        }
    );
    this->m_endDateButton->setID("end-date-button");
    this->m_buttonMenu->addChildAtPosition(this->m_endDateButton, Anchor::Bottom, {80, 25});

    this->updateDateButtons();


    auto infoPopupButton = geode::Button::createWithSpriteFrameName(
        "GJ_infoIcon_001.png",
        [this](geode::Button* button) {
            FLAlertLayer::create(
                "Advanced Help",
                "<cy>Persistent</c>: keep this level in the queue after it's processed.\n"
                "<cg>Use Regex</c>: match comments against your regex instead of the wordlist.\n"
                "<cj>Use Date Range</c>: only check comments posted between the start and end dates. "
                "Either date can be left as <cf>Any</c>.\n"
                "<co>Keep Logs</c>: save every deleted comment to a log file for this level.",
                "OK"
            )->show();
        }
    );
    infoPopupButton->setID("info-popup-button");
    infoPopupButton->setZOrder(10);
    this->m_buttonMenu->addChildAtPosition(infoPopupButton, Anchor::TopRight);

    return true;
}

CCMenuItemToggler* AdvancedSettingsPopup::createToggler(CCMenu* parent, int tag, char const* id, char const* text, bool value) {
    auto base = CCMenu::create();
    base->setContentSize({35, 35});
    base->setID(fmt::format("toggler-base-{}", tag));

    auto toggler = CCMenuItemToggler::createWithStandardSprites(
        this,
        menu_selector(AdvancedSettingsPopup::onTogglerActivated),
        1
    );
    toggler->setID(id);
    toggler->setTag(tag);
    toggler->toggle(value);
    toggler->setCascadeColorEnabled(true);
    base->addChildAtPosition(toggler, Anchor::Center);

    auto label = geode::Label::create(text, "bigFont.fnt");
    label->setID("toggler-label");
    label->setAlignment(geode::Label::Alignment::Left);
    label->setAnchorPoint({0, 0.5f});
    label->setScale(0.75f);
    base->addChildAtPosition(label, Anchor::Center, {20, 0});

    parent->addChild(base);
    return toggler;
}

void AdvancedSettingsPopup::onTogglerActivated(CCObject* sender) {
    switch (sender->getTag()) {
        case AdvancedTogglerType::Persistent:
        this->m_options.m_persistent = !this->m_options.m_persistent;
        break;

        case AdvancedTogglerType::UseRegex:
        this->m_options.m_useRegex = !this->m_options.m_useRegex;
        this->updateRegexInput();
        break;

        case AdvancedTogglerType::UseDates:
        this->m_options.m_useDates = !this->m_options.m_useDates;
        this->updateDateButtons();
        break;

        case AdvancedTogglerType::KeepLogs:
        this->m_options.m_keepLogs = !this->m_options.m_keepLogs;
        break;
    }
    this->save();
}

void AdvancedSettingsPopup::updateDateButtons() {
    this->m_startDateSprite->setString(fmt::format("From: {}", DateUtils::format(this->m_options.m_startDate)).c_str());
    this->m_endDateSprite->setString(fmt::format("To: {}", DateUtils::format(this->m_options.m_endDate)).c_str());


    bool enabled = this->m_options.m_useDates;
    ccColor3B color = enabled ? ccColor3B{ 255, 255, 255 } : ccColor3B{ 150, 150, 150 };
    this->m_startDateButton->setEnabled(enabled);
    this->m_endDateButton->setEnabled(enabled);
    this->m_startDateSprite->setColor(color);
    this->m_endDateSprite->setColor(color);
}

void AdvancedSettingsPopup::updateRegexInput() {
    this->m_regexInput->setEnabled(this->m_options.m_useRegex);
}

void AdvancedSettingsPopup::save() {
    auto options = this->m_parentPopup->getOptions();
    options.m_persistent = this->m_options.m_persistent;
    options.m_useRegex = this->m_options.m_useRegex;
    options.m_regex = this->m_options.m_regex;
    options.m_useDates = this->m_options.m_useDates;
    options.m_startDate = this->m_options.m_startDate;
    options.m_endDate = this->m_options.m_endDate;
    options.m_keepLogs = this->m_options.m_keepLogs;

    this->m_parentPopup->setOptions(options);
    SetSavedOptions(options);
}
