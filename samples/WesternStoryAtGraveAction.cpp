//
// Created by engin on 25.01.2019.
//

#include <iostream>
#include "WesternStoryAtGraveAction.h"

std::vector<LimonAPI::ParameterRequest> WesternStoryAtGraveAction::getParameters() {
    std::vector<LimonAPI::ParameterRequest> parameters;
    LimonAPI::ParameterRequest param;
    param.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::TRIGGER;
    param.description = "Check Trigger before";
    param.isSet = false;
    parameters.push_back(param);
    LimonAPI::ParameterRequest param2;
    param2.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::SWITCH;
    param2.description = "Should Trigger be run?";
    param2.isSet = true;
    parameters.push_back(param2);
    return parameters;
}

bool WesternStoryAtGraveAction::run(std::vector<LimonAPI::ParameterRequest> parameters) {
    //this action is for showing text when player is at grave
    if(this->hasRun) {
        return false;
    }
    if(parameters.size() != 2) {
        std::cerr << this->getName() << " didn't get the parameters, it won't run. " << std::endl;
        return false;
    }

    std::vector<LimonAPI::ParameterRequest>triggerResult = limonAPI->getResultOfTrigger(
            static_cast<uint32_t>(parameters[0].value.longValues[1]),
            static_cast<uint32_t>(parameters[0].value.longValues[2]));

    bool shouldBeRun = parameters[1].value.boolValue;
    if(triggerResult.size() == 0) {
        std::cerr << "trigger result was empty, it should have contained a bool" << std::endl;
        return false;
    }
    bool isRun = triggerResult[0].value.boolValue;
    if(shouldBeRun != isRun) {
        //skip because the requirement didn't fulfilled.
        return false;
    }

    textID1 = limonAPI->addGuiText("./Data/Fonts/InsaneRodeo.ttf", 64, "firstText", "Gold is not here", glm::vec3(170, 170, 50), glm::vec2(0.5f, 0.2f), 0.0f);
    std::vector<LimonAPI::ParameterRequest> emptyParamList;
    limonAPI->addTimedEvent( 3000, std::bind(&WesternStoryAtGraveAction::showMessages1, this, std::placeholders::_1), emptyParamList);
    limonAPI->addTimedEvent(7000, std::bind(&WesternStoryAtGraveAction::showMessages2, this, std::placeholders::_1), emptyParamList);
    this->hasRun = true;
    return true;
}

std::vector<LimonAPI::ParameterRequest> WesternStoryAtGraveAction::getResults() {
    return std::vector<LimonAPI::ParameterRequest>();
}

std::string WesternStoryAtGraveAction::getName() const {
    return "WesternStoryAtGrave";
}

void WesternStoryAtGraveAction::showMessages1(const std::vector<LimonAPI::ParameterRequest> &emptyParamList [[gnu::unused]]) {
    limonAPI->removeGuiElement(textID1);
    textID2 = limonAPI->addGuiText("./Data/Fonts/InsaneRodeo.ttf", 64, "secondText", "undertaker must have taken them", glm::vec3(170, 170, 50), glm::vec2(0.5f, 0.2f), 0.0f);
}

void WesternStoryAtGraveAction::showMessages2(const std::vector<LimonAPI::ParameterRequest> &emptyParamList [[gnu::unused]]) {
    limonAPI->removeGuiElement(textID2);
}
