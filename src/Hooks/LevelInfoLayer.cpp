#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include "../Comments/CommentManager.hpp"

#include <Geode/modify/LevelInfoLayer.hpp>
class $modify(MyLevelInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;
        
        auto base = CCSprite::createWithSpriteFrameName("GJ_chatBtn_001.png");
        auto icon = CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png");
        icon->setScale(1.1f);
        base->addChildAtPosition(icon, Anchor::Center);

        if (auto menu = this->getChildByID("left-side-menu"); menu->getChildByIDRecursive("delete-button")) {
            auto button = CCMenuItemSpriteExtra::create(
                base,
                this,
                menu_selector(MyLevelInfoLayer::onCommentSettingsPopup)
            );
            button->setID("delete-comments-button"_spr);

            menu->addChild(button);
            menu->updateLayout();
        }


        return true;
    }

    void onCommentSettingsPopup(CCObject*) {
        CommentManagerPopup::create(this->m_level)->show();
    }

};