#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include "Util/Structures.hpp"
#include "Util/DefaultWordlist.hpp"

#include "Comments/CommentManager.hpp"
#include "Comments/QueueVisualizer.hpp"

$on_mod(Loaded) {
    

    CommentManager::get()->loadPersistentQueue();
    CommentManager::get()->QueueHandle = async::spawn(CommentManager::get()->processQueue());
    log::info("Starting Comment Manager Queue");

    listenForKeybindSettingPresses("open-queue-keybind", [](Keybind const&, bool down, bool repeat, double) {
        if (!down || repeat) return false;

        auto scene = CCScene::get();
        if (!scene || scene->getChildByType<QueueVisualizerPopup>(0)) return false;

        QueueVisualizerPopup::create()->show();
        return true;
    });
}

$on_game(Exiting) {
    CommentManager::get()->QueueHandle.abort();
    log::info("Aborting comment queue.");
}
