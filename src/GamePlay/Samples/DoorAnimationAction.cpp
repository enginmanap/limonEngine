//
// Created by engin on 20.05.2018.
//

#include <c++/8.2.1/iostream>
#include "DoorAnimationAction.h"
#include "../LimonAPI.h"

TriggerRegister<DoorAnimationAction> DoorAnimationAction::reg("DoorAnimationAction");

std::vector<LimonAPI::ParameterRequest> DoorAnimationAction::getParameters() {
    std::vector<LimonAPI::ParameterRequest> parameters;
    LimonAPI::ParameterRequest param1;
    param1.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::MODEL;
    param1.description = "Model to animate";
    parameters.push_back(param1);

    LimonAPI::ParameterRequest param2;
    param2.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::ANIMATION;
    param2.description = "Animation to apply";
    parameters.push_back(param2);

    LimonAPI::ParameterRequest param3;
    param3.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::SWITCH;
    param3.description = "Is animation looped";
    param3.isSet = true;
    parameters.push_back(param3);

    LimonAPI::ParameterRequest param4;
    param4.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::FREE_TEXT;
    param4.description = "Sound to play";
    param4.isSet = true;
    parameters.push_back(param4);

    LimonAPI::ParameterRequest param5;
    param5.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::TRIGGER;
    param5.description = "Wait for trigger finish";
    param5.isSet = false;
    parameters.push_back(param5);

    LimonAPI::ParameterRequest param6;
    param6.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::FREE_NUMBER;
    param6.description = "wait time";
    param6.isSet = true;
    parameters.push_back(param6);

    return parameters;
}

bool DoorAnimationAction::run(std::vector<LimonAPI::ParameterRequest> parameters) {

    if(parameters.size() != 6 ) {
        std::cerr << "Parameters for DoorAnimationAction is missing, skipping." << std::endl;
        return false;
    }
    this->stateResetTime = parameters[5].value.longValue;

    if(parameters[4].value.longValue != 0) {
        std::vector<LimonAPI::ParameterRequest>enterResult = limonAPI->getResultOfTrigger(
                static_cast<uint32_t>(parameters[4].value.longValue), 2);
        std::vector<LimonAPI::ParameterRequest>exitResult  = limonAPI->getResultOfTrigger(
                static_cast<uint32_t>(parameters[4].value.longValue), 3);
        if(enterResult.size() == 1 && enterResult[0].value.boolValue == true &&
                exitResult.size() == 1 && exitResult[0].value.boolValue == false
                ) {
            std::cout << "skipping not to interfere" << std::endl;
            return true;//skip animation
        }
        //if entered, but not exited another trigger, skip animation, that one should handle it
    }

    std::string* sound = nullptr;

    if(parameters[3].value.stringValue[0] != '\0') {
        sound = new std::string(parameters[3].value.stringValue);
    }
    limonAPI->animateModel(static_cast<uint32_t>(parameters[0].value.longValue),
                           static_cast<uint32_t>(parameters[1].value.longValue),
                           parameters[2].value.boolValue,
                           sound);
    delete sound;

    this->animationRun = true;//set this animation run.

    //set timer for this animation run reset.
    std::function<void(const std::vector<LimonAPI::ParameterRequest>&)> methodToCall = std::bind(&DoorAnimationAction::resetAnimationRun, this, std::placeholders::_1);
    std::vector<LimonAPI::ParameterRequest> emptyParams;

    limonAPI->addTimedEvent(this->stateResetTime, methodToCall, emptyParams);

    return true;
}


std::vector<LimonAPI::ParameterRequest> DoorAnimationAction::getResults() {
    std::vector<LimonAPI::ParameterRequest> result;
    LimonAPI::ParameterRequest request;
    request.value.boolValue = this->animationRun;
    result.push_back(request);
    return result;
}

DoorAnimationAction::DoorAnimationAction(LimonAPI *limonAPI) : TriggerInterface(limonAPI) {}

void DoorAnimationAction::resetAnimationRun(const std::vector<LimonAPI::ParameterRequest> &) {
    this->animationRun = false;
    std::cout << "animation reset by timer" << std::endl;
}
