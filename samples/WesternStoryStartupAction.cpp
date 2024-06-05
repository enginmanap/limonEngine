//
// Created by engin on 25.01.2019.
//

#include "WesternStoryStartupAction.h"

std::vector<LimonTypes::GenericParameter> WesternStoryStartupAction::getParameters() {
    return std::vector<LimonTypes::GenericParameter>();
}

bool WesternStoryStartupAction::run(std::vector<LimonTypes::GenericParameter> parameters[[gnu::unused]]) {
    if(this->hasRun) {
        return false;
    }
    //this action is for showing text when player is spawned.

    textID1 = limonAPI->addGuiText("./Data/Fonts/InsaneRodeo.ttf", 64, "firstText", "They are here", glm::vec3(170, 170, 50), glm::vec2(0.5f, 0.2f), 0.0f);
    std::vector<LimonTypes::GenericParameter> emptyParamList;
    limonAPI->addTimedEvent( 4000, false, std::bind(&WesternStoryStartupAction::showMessages1, this, std::placeholders::_1), emptyParamList);
    limonAPI->addTimedEvent(8000, false, std::bind(&WesternStoryStartupAction::showMessages2, this, std::placeholders::_1), emptyParamList);
    this->hasRun = true;
    return true;
}

std::vector<LimonTypes::GenericParameter> WesternStoryStartupAction::getResults() {
    return std::vector<LimonTypes::GenericParameter>();
}

std::string WesternStoryStartupAction::getName() const {
    return "WesternStoryStartup";
}

void WesternStoryStartupAction::showMessages1(const std::vector<LimonTypes::GenericParameter> &emptyParamList [[gnu::unused]]) {
    limonAPI->removeGuiElement(textID1);
    textID2 = limonAPI->addGuiText("./Data/Fonts/InsaneRodeo.ttf", 64, "secondText", "I should find the grave. get the gold and skip it", glm::vec3(170, 170, 50), glm::vec2(0.5f, 0.2f), 0.0f);
}

void WesternStoryStartupAction::showMessages2(const std::vector<LimonTypes::GenericParameter> &emptyParamList [[gnu::unused]]) {
    limonAPI->removeGuiElement(textID2);
}
