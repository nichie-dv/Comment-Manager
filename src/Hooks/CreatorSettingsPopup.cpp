#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include "../Comments/CommentManager.hpp"

#include <Geode/modify/CreatorSettingsPopup.hpp>
class $modify(MyCreatorSettingsPopup, CreatorSettingsPopup) {
    bool init(GJGameLevel* level, CreatorSettingsDelegate* delegate) {
        if (!CreatorSettingsPopup::init(level, delegate)) return false;

        CCMenuItemSpriteExtra* descButton = nullptr;

        //This will not work if some api or something comes out
        //that lets us add custom buttons to that menu (hello)
        if (auto mainMenu = this->m_mainLayer->getChildByType<CCMenu>(0)) {
            for (auto child : mainMenu->getChildrenExt()) {
                auto button = typeinfo_cast<CCMenuItemSpriteExtra*>(child);
                if (button && typeinfo_cast<ButtonSprite*>(button->getNormalImage())) {
                    descButton = button;
                    break;
                }
            }
        }


        auto menu = descButton ? descButton->getParent() : nullptr;
        if (!menu) {
            log::warn("Couldn't add comment settings button");
            return true;
        }


        auto descSprite = static_cast<ButtonSprite*>(descButton->getNormalImage());
        auto sprite = ButtonSprite::create(
            "Comments",
            static_cast<int>(descSprite->m_width), static_cast<int>(descSprite->m_minWidth),
            descSprite->m_scale, descSprite->m_absolute,
            "bigFont.fnt", "GJ_button_01.png", descSprite->m_height
        );
        sprite->setScale(descSprite->getScale());

        auto button = CCMenuItemSpriteExtra::create(sprite, this, menu_selector(MyCreatorSettingsPopup::onCommentSettings));
        button->setID("comment-settings-button"_spr);
        menu->addChild(button);

        float spacing = (descButton->getScaledContentHeight() + 8.f) / 2.f;
        CCPoint center = descButton->getPosition();

        descButton->setPosition(center + CCPoint{0, spacing});
        button->setPosition(center - CCPoint{0, spacing});

        return true;
    }

    void onCommentSettings(CCObject*) {
        CommentManagerPopup::create(this->m_level)->show();
    }
};
