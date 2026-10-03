#include "WordlistPopup.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include "CommentManager.hpp"

#include "../Util/OptionsSaveLoad.hpp"
#include "../Util/Structures.hpp"
#include "../Util/DefaultWordlist.hpp"

//Entry

WordlistEntry* WordlistEntry::create(BannedWord data) {
    auto ret = new WordlistEntry();
    if (ret->init(data)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool WordlistEntry::init(BannedWord data) {
    if (!CCNode::init()) return false;

    this->m_data = std::move(data);

    this->setID("wordlist-entry-node");
    this->setContentSize({330, 40});
    this->setAnchorPoint({0.5f, 0.5f});
    this->setCascadeColorEnabled(true);

    this->m_background = geode::NineSlice::create("GJ_square05.png");
    this->m_background->setScaleMultiplier(0.85f);
    this->m_background->setID("node-background");
    this->m_background->setZOrder(5);
    this->m_background->setContentSize(this->getContentSize());
    this->addChildAtPosition(this->m_background, Anchor::Center);
    


    this->m_buttonMenu = CCMenu::create();
    this->m_buttonMenu->setID("node-button-menu");
    this->m_buttonMenu->setZOrder(6);
    this->m_buttonMenu->setContentSize(this->getContentSize());
    this->m_buttonMenu->setCascadeColorEnabled(true);
    this->addChildAtPosition(this->m_buttonMenu, Anchor::Center);

    


    auto label = geode::Label::create(this->m_data.m_word.c_str(), "mdFontB.fnt");
    label->setID("node-word-label");
    label->setZOrder(7);
    label->setAnchorPoint({0, 0.5f});
    label->setLimitLabelWidth(145, 0.9f);
    this->addChildAtPosition(label, Anchor::Left, {10, 0});


    this->m_toggler = CCMenuItemToggler::createWithStandardSprites(
        this,
        menu_selector(WordlistEntry::onNodeToggled),
        0.75f
    );
    this->m_toggler->setID("word-toggler");
    this->m_toggler->setZOrder(9);
    this->m_toggler->toggle(this->m_data.m_enabled);
    if (this->m_data.m_enabled) {
        this->setColor({ 255, 255, 255 });
    } else {
        this->setColor({ 150, 150, 150 });
    }

    this->m_buttonMenu->addChildAtPosition(this->m_toggler, Anchor::Right, {-50, 0});



    auto deleteButtonSpr = CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");
    deleteButtonSpr->setScale(0.65f);

    auto deleteButton = CCMenuItemSpriteExtra::create(
        deleteButtonSpr,
        this,
        menu_selector(WordlistEntry::onDeleteNode)
    );
    deleteButton->setID("delete-node-button");
    deleteButton->setZOrder(10);
    deleteButton->setCascadeColorEnabled(false);
    this->m_buttonMenu->addChildAtPosition(deleteButton, Anchor::Right, {-20, 0});


    return true;
}

void WordlistEntry::onNodeToggled(CCObject* sender) {
    this->m_data.m_enabled = !this->m_data.m_enabled;

    //Do other things here too
    if (this->m_data.m_enabled) {
        this->setColor({ 255, 255, 255 });
    } else {
        this->setColor({ 150, 150, 150 });
    }
    
}

void WordlistEntry::onDeleteNode(CCObject* sender) {
    WordlistPopup* popup = nullptr;

    //Looks nasty but who gaf
    if ((popup = static_cast<WordlistPopup*>(this
    ->getParent()                                   //content layer
    ->getParent()                                   //scroll layer
    ->getParent()                                   //cclayer
    ->getParent()))) {                              //popup
        popup->removeEntry(this);
        popup->updateContentSize();
    } else {
        log::error("Wordlist entry node missing parent, cannot resize popup content layer.");
    }
}

BannedWord* WordlistEntry::getData() {
    return &this->m_data;
}



//Popup


WordlistPopup* WordlistPopup::create(CommentManagerPopup* parent) {
    auto ret = new WordlistPopup();
    if (ret->init(parent)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool WordlistPopup::init(CommentManagerPopup* parent) {
    if (!geode::Popup::init({370, 290}, "GJ_square04.png")) return false;

    this->setID("edit-wordlist-popup"_spr);
    this->m_parentPopup = parent;

    this->m_bgSprite->setID("background-sprite");
    if (this->m_title) this->m_title->setID("title");
    this->m_buttonMenu->setID("main-button-menu");

    //Replaced by an "OK" button at the bottom
    this->m_closeBtn->removeFromParentAndCleanup(true);


    //The parent popup holds the loaded options, so edits here stay in sync with it
    this->m_options = this->m_parentPopup->getOptions();
    

    auto newCloseButton = geode::Button::createWithNode(
        ButtonSprite::create("OK"),
        [this](geode::Button* button) {
            //Save data
            std::vector<BannedWord> output;
            output.reserve(this->m_wordlist.size());
            for (auto* word : this->m_wordlist) output.push_back(*word);

            this->m_options = this->m_parentPopup->getOptions();
            this->m_options.m_wordlist = output;
            this->m_parentPopup->setOptions(this->m_options);
            SetSavedOptions(this->m_options);

            this->onClose(button);
        }
    );
    newCloseButton->setID("close-button");
    newCloseButton->setZOrder(10);
    this->m_buttonMenu->addChildAtPosition(newCloseButton, Anchor::Bottom, {0, 25});

    auto infoPopupButton = geode::Button::createWithSpriteFrameName(
        "GJ_infoIcon_001.png",
        [this](geode::Button* button) {
            FLAlertLayer::create(
                "Wordlist Help",
                "Comments containing any phrase on this list will be deleted. "
                "The <cf>*</c> character is a <cp>wildcard</c>, meaning any symbol or character can take its place. "
                "<cy>Example</c>: <cj>b**k</c> will match with <cg>book</c>, <cg>bark</c>, <cg>b00k</c>, but will not match with <cr>broke</c>, <cr>bart</c>, <cr>b8888k</c>...",
                "OK"
            )->show();
        }
    );
    infoPopupButton->setID("info-popup-button");
    infoPopupButton->setZOrder(10);
    this->m_buttonMenu->addChildAtPosition(infoPopupButton, Anchor::TopRight);


   
    auto clearAllButton = geode::Button::createWithSprite(
        "N_deleteAllBtn_01.png"_spr,
        [this](geode::Button* button) {
            geode::createQuickPopup(
                "Delete all words",
                "Are you sure you want to <cr>delete all words</c>? "
                "The default wordlist can be recreated using the <cy>reset button</c>.",
                "Yes", "Cancel",
                [this](FLAlertLayer* popup, bool cancel) {
                    if (cancel) return;

                    this->m_wordlist.clear();
                    this->m_scrollLayer->m_contentLayer->removeAllChildrenWithCleanup(true);
                    this->updateContentSize();
                }, true, true
            );
        }
    );
    clearAllButton->setID("clear-all-button");
    clearAllButton->setZOrder(10);
    clearAllButton->setScale(0.75f);
    this->m_buttonMenu->addChildAtPosition(clearAllButton, Anchor::BottomRight, {-25, 25});

    auto newWordButton = geode::Button::createWithSpriteFrameName(
        "GJ_plusBtn_001.png",
        [this](geode::Button* button) {
            NewWordPopup::create(this)->show();
        }
    );
    newWordButton->setScale(0.7f);
    newWordButton->setID("new-word-button");
    newWordButton->setZOrder(10);
    this->m_buttonMenu->addChildAtPosition(newWordButton, Anchor::BottomLeft, {25, 25});


    //Scrolling shi here
    this->m_scrollLayer = ScrollLayer::create({350, 230});
    this->m_scrollLayer->setID("scroll-layer");
    this->m_scrollLayer->setZOrder(15);
    this->m_scrollLayer->ignoreAnchorPointForPosition(false);
    this->m_mainLayer->addChildAtPosition(this->m_scrollLayer, Anchor::Center, {-4.25f, 18});

    this->m_scrollbar = geode::Scrollbar::create(this->m_scrollLayer);
    this->m_scrollbar->setID("scrollbar");
    this->m_scrollbar->setZOrder(15);
    this->m_scrollbar->setVisible(this->m_wordlist.size() > 5);
    this->m_mainLayer->addChildAtPosition(this->m_scrollbar, Anchor::Right, {-8.5f, 18});

    auto scrollLayerBG = geode::NineSlice::create("square02_001.png");
    scrollLayerBG->setID("scroll-layer-bg");
    scrollLayerBG->setZOrder(10);
    scrollLayerBG->setContentSize({350, 230});
    scrollLayerBG->setOpacity(150);
    this->m_mainLayer->addChildAtPosition(scrollLayerBG, Anchor::Center, {-4.25f, 18});


    for (auto const& word : this->m_options.m_wordlist) {
        this->addNewEntry(WordlistEntry::create(word));
    }

    return true;
}

void WordlistPopup::addNewEntry(WordlistEntry* entry) {
    auto* data = entry->getData();
    for (auto* existing : this->m_wordlist) {
        if (existing->m_word == data->m_word) return; //duplicate, skip entirely
    }

    this->m_wordlist.push_back(data);
    this->m_scrollLayer->m_contentLayer->addChild(entry);
    this->updateContentSize();
}

void WordlistPopup::removeEntry(WordlistEntry* entry) {
    std::erase(this->m_wordlist, entry->getData()); //before the node dies
    entry->removeFromParentAndCleanup(true);
}


void WordlistPopup::updateContentSize() {
    auto content = this->m_scrollLayer->m_contentLayer;

    constexpr float gap = 5;
    constexpr float padding = 10;

    float y = padding;
    if (content->getChildrenCount() > 0) {
        for (auto child : CCArrayExt<WordlistEntry*>(content->getChildren())) {
            float h = child->getContentHeight();
            child->m_yOffset = y + h / 2;
            y += h + gap;
        }
        y -= gap;
    }

    //Bottom
    y += padding;

    float newHeight = std::max(y, this->m_scrollLayer->getContentHeight());
    content->setContentHeight(newHeight);

    if (content->getChildrenCount() > 0) {
        for (auto child : CCArrayExt<WordlistEntry*>(content->getChildren())) {
            child->setPosition({ content->getContentWidth() / 2, newHeight - child->m_yOffset });
        }
    }
    
    this->m_scrollLayer->scrollToTop();
    this->m_scrollbar->setVisible(content->getChildrenCount() > 5);
}



//New Word

NewWordPopup* NewWordPopup::create(WordlistPopup* popup) {
    auto ret = new NewWordPopup();
    if (ret->init(popup)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool NewWordPopup::init(WordlistPopup* popup) {
    if (!geode::Popup::init({200, 100})) return false;

    this->setID("new-word-popup"_spr);

    //Create Button
    auto createButton = geode::Button::createWithNode(
        ButtonSprite::create("Add"),
        [this, popup](geode::Button* button) {
            popup->addNewEntry(WordlistEntry::create(this->m_data));
            this->onClose(button);
        }
    );
    createButton->setID("create-button");
    createButton->setZOrder(10);
    this->m_buttonMenu->addChildAtPosition(createButton, Anchor::Bottom, {0, 25});


    //input
    auto textInput = geode::TextInput::create(100, "...", "mdFontB.fnt");
    textInput->setID("text-input");
    textInput->setZOrder(10);
    textInput->setFilter("qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890-=+_;':\"`~!@#$%^&*()<>?,./\\|[]{}");
    textInput->setCallback(
        [this](std::string out) {
            this->m_data.m_word = out.c_str();
        }
    );
    this->m_buttonMenu->addChildAtPosition(textInput, Anchor::Center, {0, 15});


    return true;
}
