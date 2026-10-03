#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include "../Comments/CommentManager.hpp"
#include "../Comments/QueueVisualizer.hpp"

#include <Geode/modify/MenuLayer.hpp>
class $modify(MyMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        
        auto menu = this->getChildByIDRecursive("right-side-menu");
        if (!menu) return false;

        auto spriteBase = CCSprite::createWithSpriteFrameName("geode.loader/baseCircle_Big_Green.png");

        auto spriteIcon1= CCSprite::createWithSpriteFrameName("particle_206_001.png");
        spriteIcon1->setScale(1.75f);
        spriteIcon1->setRotation(-15);
        spriteBase->addChildAtPosition(spriteIcon1, Anchor::Center, {-2, -2});

        auto spriteIcon2 = CCSprite::createWithSpriteFrameName("geode.loader/search.png");
        spriteIcon2->setScale(0.5f);
        spriteIcon2->setRotation(-10);
        spriteBase->addChildAtPosition(spriteIcon2, Anchor::Center, {5, 3});
        
        auto popupButton = geode::Button::createWithNode(
            spriteBase,
            [this](geode::Button* button) {
                QueueVisualizerPopup::create()->show();
            }
        );
        popupButton->setID("popup-butotn"_spr);
        menu->addChild(popupButton);
        menu->updateLayout();



        if (!Mod::get()->getSavedValue<bool>("init", false)) {
            auto alert = FLAlertLayer::create(
                "Notice",
                "Make sure to read all <cj>info</c> labels before using <cg>Comment Manager</c>. This mod will "
                "<cr>ireversibly delete comments</c>.",
                "OK"
            );
            
            alert->m_scene = this;
            alert->show();
            Mod::get()->setSavedValue<bool>("init", true);
        }


        return true;
    }
};