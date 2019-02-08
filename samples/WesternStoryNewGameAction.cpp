//
// Created by engin on 25.01.2019.
//

#include "WesternStoryStartupAction.h"

std::vector<LimonAPI::ParameterRequest> WesternStoryStartupAction::getParameters() {
    return std::vector<LimonAPI::ParameterRequest>();
}

bool WesternStoryStartupAction::run(std::vector<LimonAPI::ParameterRequest> parameters[[gnu::unused]]) {
    if(this->hasRun) {
        return false;
    }
    //this action is for showing text when player is spawned.

    textID1 = limonAPI->addGuiText("./Data/Fonts/InsaneRodeo.ttf", 64, "firstText", "They are here", glm::vec3(170, 170, 50), glm::vec2(0.5f, 0.2f), 0.0f);
    std::vector<LimonAPI::ParameterRequest> emptyParamList;
    limonAPI->addTimedEvent( 4000, std::bind(&WesternStoryStartupAction::showMessages1, this, std::placeholders::_1), emptyParamList);
    limonAPI->addTimedEvent(8000, std::bind(&WesternStoryStartupAction::showMessages2, this, std::placeholders::_1), emptyParamList);
    this->hasRun = true;
    return true;
}

std::vector<LimonAPI::ParameterRequest> WesternStoryStartupAction::getResults() {
    return std::vector<LimonAPI::ParameterRequest>();
}

std::string WesternStoryStartupAction::getName() const {
    return "WesternStoryStartup";
}

void WesternStoryStartupAction::showMessages1(const std::vector<LimonAPI::ParameterRequest> &emptyParamList [[gnu::unused]]) {
    limonAPI->removeGuiElement(textID1);
    textID2 = limonAPI->addGuiText("./Data/Fonts/InsaneRodeo.ttf", 64, "secondText", "I should find the grave. get the gold and skip it", glm::vec3(170, 170, 50), glm::vec2(0.5f, 0.2f), 0.0f);
}

void WesternStoryStartupAction::showMessages2(const std::vector<LimonAPI::ParameterRequest> &emptyParamList [[gnu::unused]]) {
    limonAPI->removeGuiElement(textID2);
}
