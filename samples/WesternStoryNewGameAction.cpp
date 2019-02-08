//
// Created by engin on 25.01.2019.
//

#include "WesternStoryNewGameAction.h"

std::vector<LimonAPI::ParameterRequest> WesternStoryNewGameAction::getParameters() {
    return std::vector<LimonAPI::ParameterRequest>();
}

bool WesternStoryNewGameAction::run(std::vector<LimonAPI::ParameterRequest> parameters[[gnu::unused]]) {
    /**
     * when player clicks new game do the following:
     * 1) Remove "New Game" and "Quit" buttons
     * 2) Animate the 6 texts
     * 3) after 6 text animation finished animate 2 texts
     * 4) After 2 text animation finished load the world
     *
     * for steps 3 and 4, we will need timed events
     */

    /**
     * Map Data:
     * Newgame -> 6
     * Quit    -> 7
     *
     * letter Lines (1-6) -> 5,93, 94, 95, 96, 98
     * thoughts  (1,2) -> 99, 100
     *
     * scroolFade animation -> id: 0, duration: 720/60
     * fadeInOut animation -> id:1, duration: 300/60
     *
     */

    limonAPI->removeGuiElement(6);//remove New game
    limonAPI->removeGuiElement(7);// remove Quit

    //animate letter
    limonAPI->animateModel(5,0,false, nullptr);
    limonAPI->animateModel(93,0,false, nullptr);
    limonAPI->animateModel(94,0,false, nullptr);
    limonAPI->animateModel(95,0,false, nullptr);
    limonAPI->animateModel(96,0,false, nullptr);
    limonAPI->animateModel(98,0,false, nullptr);

    //now register timed events
    std::vector<LimonAPI::ParameterRequest> emptyParamList;
    limonAPI->addTimedEvent(730 * 1000 / 60, std::bind(&WesternStoryNewGameAction::animateThoughts, this, std::placeholders::_1), emptyParamList);
    limonAPI->addTimedEvent((730 + 310) *  1000 / 60, std::bind(&WesternStoryNewGameAction::switchWorld, this, std::placeholders::_1), emptyParamList);
    return true;
}

void WesternStoryNewGameAction::animateThoughts(const std::vector<LimonAPI::ParameterRequest> &emptyParamList [[gnu::unused]]) {
    //animate letter
    limonAPI->removeGuiElement(5);
    limonAPI->removeGuiElement(93);
    limonAPI->removeGuiElement(94);
    limonAPI->removeGuiElement(95);
    limonAPI->removeGuiElement(96);
    limonAPI->removeGuiElement(98);

    limonAPI->animateModel(99,1,false, nullptr);
    limonAPI->animateModel(100,1,false, nullptr);
}

void WesternStoryNewGameAction::switchWorld(const std::vector<LimonAPI::ParameterRequest> &emptyParamList [[gnu::unused]]) {
    limonAPI->loadAndSwitchWorld("./Data/Maps/Western006.xml");
}

std::vector<LimonAPI::ParameterRequest> WesternStoryNewGameAction::getResults() {
    return std::vector<LimonAPI::ParameterRequest>();
}

std::string WesternStoryNewGameAction::getName() const {
    return "WesternStoryNewGameAction";
}