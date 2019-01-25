//
// Created by engin on 25.01.2019.
//

#include <iostream>
#include "WesternStoryAtUndertakerAction.h"

std::vector<LimonAPI::ParameterRequest> WesternStoryAtUndertakerAction::getParameters() {
    std::vector<LimonAPI::ParameterRequest> parameters;
    LimonAPI::ParameterRequest param;
    param.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::MODEL;
    param.description = "Object to remove";
    param.isSet = false;
    parameters.push_back(param);
    return parameters;
}

bool WesternStoryAtUndertakerAction::run(std::vector<LimonAPI::ParameterRequest> parameters[[gnu::unused]]) {
    //this action is when the player finds the gold in undertakers home
    if(this->hasRun) {
        return false;
    }
    if(parameters.size() != 1) {
        std::cerr << this->getName() << " didn't get the parameters, it won't run. " << std::endl;
        return false;
    }


    uint32_t objectID = static_cast<uint32_t>(parameters[0].value.longValue);

    if(!limonAPI->removeObject(objectID)) {
        std::cerr << "object remove failed!" << std::endl;
    }

    textID1 = limonAPI->addGuiText("./Data/Fonts/InsaneRodeo.ttf", 64, "firstText", "Bingo", glm::vec3(170, 170, 50), glm::vec2(0.5f, 0.2f), 0.0f);
    std::vector<LimonAPI::ParameterRequest> emptyParamList;
    limonAPI->addTimedEvent( 2000, std::bind(&WesternStoryAtUndertakerAction::showMessages1, this, std::placeholders::_1), emptyParamList);
    limonAPI->addTimedEvent(6000, std::bind(&WesternStoryAtUndertakerAction::showMessages2, this, std::placeholders::_1), emptyParamList);
    limonAPI->addTimedEvent(10000, std::bind(&WesternStoryAtUndertakerAction::showMessages3, this, std::placeholders::_1), emptyParamList);
    this->hasRun = true;
    return true;
}

std::vector<LimonAPI::ParameterRequest> WesternStoryAtUndertakerAction::getResults() {
    std::vector<LimonAPI::ParameterRequest> result;
    LimonAPI::ParameterRequest hasRunParameter;
    hasRunParameter.valueType = LimonAPI::ParameterRequest::ValueTypes::BOOLEAN;
    hasRunParameter.value.boolValue = hasRun;
    result.push_back(hasRunParameter);
    return result;
}

std::string WesternStoryAtUndertakerAction::getName() const {
    return "WesternStoryAtUndertaker";
}

void WesternStoryAtUndertakerAction::showMessages1(const std::vector<LimonAPI::ParameterRequest> &emptyParamList [[gnu::unused]]) {
    limonAPI->removeGuiElement(textID1);
    textID2 = limonAPI->addGuiText("./Data/Fonts/InsaneRodeo.ttf", 64, "secondText", "Wait a minute, where is the rest", glm::vec3(170, 170, 50), glm::vec2(0.5f, 0.2f), 0.0f);
}

void WesternStoryAtUndertakerAction::showMessages2(const std::vector<LimonAPI::ParameterRequest> &emptyParamList [[gnu::unused]]) {
    limonAPI->removeGuiElement(textID2);
    textID3 = limonAPI->addGuiText("./Data/Fonts/InsaneRodeo.ttf", 64, "secondText", "And I left the  map at the room, dammit", glm::vec3(170, 170, 50), glm::vec2(0.5f, 0.2f), 0.0f);
}

void WesternStoryAtUndertakerAction::showMessages3(const std::vector<LimonAPI::ParameterRequest> &emptyParamList [[gnu::unused]]) {
    limonAPI->removeGuiElement(textID3);
}
