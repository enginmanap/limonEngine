//
// Created by engin on 25.01.2019.
//

#include <iostream>
#include "WesternStoryAtTrainAction.h"

std::vector<LimonAPI::ParameterRequest> WesternStoryAtTrainAction::getParameters() {
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

    LimonAPI::ParameterRequest param3;
    param3.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::FREE_TEXT;
    param3.description = "World to load";
    param3.isSet = false;
    parameters.push_back(param3);

    return parameters;
}

bool WesternStoryAtTrainAction::run(std::vector<LimonAPI::ParameterRequest> parameters) {
    if(parameters.size() != 3) {
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

    limonAPI->loadAndSwitchWorld(parameters[2].value.stringValue);

    return true;
}

std::vector<LimonAPI::ParameterRequest> WesternStoryAtTrainAction::getResults() {
    return std::vector<LimonAPI::ParameterRequest>();
}

std::string WesternStoryAtTrainAction::getName() const {
    return "WesternStoryAtTrain";
}
