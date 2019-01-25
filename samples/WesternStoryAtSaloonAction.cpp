//
// Created by engin on 25.01.2019.
//

#include <iostream>
#include "WesternStoryAtSaloonAction.h"

std::vector<LimonAPI::ParameterRequest> WesternStoryAtSaloonAction::getParameters() {
    std::vector<LimonAPI::ParameterRequest> parameters;
    LimonAPI::ParameterRequest param;
    param.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::TRIGGER;
    param.description = "Check Trigger before";
    param.isSet = false;
    parameters.push_back(param);

    LimonAPI::ParameterRequest param2;
    param2.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::SWITCH;
    param2.description = "Should be run?";
    param2.isSet = true;
    parameters.push_back(param2);

    LimonAPI::ParameterRequest param3;
    param3.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::MODEL;
    param3.description = "Object to remove";
    param3.isSet = false;
    parameters.push_back(param3);

    return parameters;
}

bool WesternStoryAtSaloonAction::run(std::vector<LimonAPI::ParameterRequest> parameters[[gnu::unused]]) {
    //this action is for showing text after player takes the map from the room
    if(this->hasRun) {
        return false;
    }
    if(parameters.size() != 3) {
        std::cerr << this->getName() << " didn't get the parameters, it won't run. " << std::endl;
        return false;
    }

    std::vector<LimonAPI::ParameterRequest>triggerResult = limonAPI->getResultOfTrigger(
            static_cast<uint32_t>(parameters[0].value.longValues[1]),
            static_cast<uint32_t>(parameters[0].value.longValues[2]));

    bool shouldBeRun = parameters[1].value.boolValue;
    if(triggerResult.empty()) {
        std::cerr << "trigger result was empty, it should have contained a bool" << std::endl;
        return false;
    }
    bool isRun = triggerResult[0].value.boolValue;
    if(shouldBeRun != isRun) {
        //skip because the requirement didn't fulfilled.
        return false;
    }

    uint32_t objectID = static_cast<uint32_t>(parameters[2].value.longValue);

    if(!limonAPI->removeObject(objectID)) {
        std::cerr << "object remove failed!" << std::endl;
    }

    textID1 = limonAPI->addGuiText("./Data/Fonts/InsaneRodeo.ttf", 64, "firstText", "They didn't take it", glm::vec3(170, 170, 50), glm::vec2(0.5f, 0.2f), 0.0f);
    std::vector<LimonAPI::ParameterRequest> emptyParamList;
    limonAPI->addTimedEvent( 4000, std::bind(&WesternStoryAtSaloonAction::showMessages1, this, std::placeholders::_1), emptyParamList);
    limonAPI->addTimedEvent( 6000, std::bind(&WesternStoryAtSaloonAction::showMessages2, this, std::placeholders::_1), emptyParamList);
    limonAPI->addTimedEvent( 8000, std::bind(&WesternStoryAtSaloonAction::showMessages3, this, std::placeholders::_1), emptyParamList);
    this->hasRun = true;
    return true;
}

std::vector<LimonAPI::ParameterRequest> WesternStoryAtSaloonAction::getResults() {
    std::vector<LimonAPI::ParameterRequest> result;
    LimonAPI::ParameterRequest hasRunParameter;
    hasRunParameter.valueType = LimonAPI::ParameterRequest::ValueTypes::BOOLEAN;
    hasRunParameter.value.boolValue = hasRun;
    result.push_back(hasRunParameter);
    return result;
}

std::string WesternStoryAtSaloonAction::getName() const {
    return "WesternStoryAtSaloon";
}

void WesternStoryAtSaloonAction::showMessages1(const std::vector<LimonAPI::ParameterRequest> &emptyParamList [[gnu::unused]]) {
    limonAPI->removeGuiElement(textID1);
    textID2 = limonAPI->addGuiText("./Data/Fonts/InsaneRodeo.ttf", 64, "secondText", "Time to skip town", glm::vec3(170, 170, 50), glm::vec2(0.5f, 0.2f), 0.0f);
}

void WesternStoryAtSaloonAction::showMessages2(const std::vector<LimonAPI::ParameterRequest> &emptyParamList [[gnu::unused]]) {
    glm::vec3 trainPosition = glm::vec3(82.581146f, 6.7320518f, 36.58247f);
    limonAPI->playSound("./Data/Sounds/steam-train-whistle.wav", trainPosition);

}

void WesternStoryAtSaloonAction::showMessages3(const std::vector<LimonAPI::ParameterRequest> &emptyParamList [[gnu::unused]]) {
    limonAPI->removeGuiElement(textID2);
}
