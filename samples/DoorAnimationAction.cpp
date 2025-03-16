//
// Created by engin on 20.05.2018.
//

#include <iostream>
#include "DoorAnimationAction.h"
#include "limonAPI/LimonAPI.h"

std::vector<LimonTypes::GenericParameter> DoorAnimationAction::getParameters() {
    std::vector<LimonTypes::GenericParameter> parameters;
    LimonTypes::GenericParameter param1;
    param1.requestType = LimonTypes::GenericParameter::RequestParameterTypes::MODEL;
    param1.description = "Model to animate";
    parameters.push_back(param1);

    LimonTypes::GenericParameter param2;
    param2.requestType = LimonTypes::GenericParameter::RequestParameterTypes::ANIMATION;
    param2.description = "Animation to apply";
    parameters.push_back(param2);

    LimonTypes::GenericParameter param3;
    param3.requestType = LimonTypes::GenericParameter::RequestParameterTypes::SWITCH;
    param3.description = "Is animation looped";
    param3.isSet = true;
    parameters.push_back(param3);

    LimonTypes::GenericParameter param4;
    param4.requestType = LimonTypes::GenericParameter::RequestParameterTypes::FREE_TEXT;
    param4.description = "Sound to play";
    param4.isSet = true;
    parameters.push_back(param4);

    LimonTypes::GenericParameter param5;
    param5.requestType = LimonTypes::GenericParameter::RequestParameterTypes::TRIGGER;
    param5.description = "Wait for trigger finish";
    param5.isSet = false;
    parameters.push_back(param5);

    LimonTypes::GenericParameter param6;
    param6.requestType = LimonTypes::GenericParameter::RequestParameterTypes::FREE_NUMBER;
    param6.description = "wait time";
    param6.isSet = true;
    parameters.push_back(param6);

    return parameters;
}

bool DoorAnimationAction::run(std::vector<LimonTypes::GenericParameter> parameters) {

    if(parameters.size() != 6 ) {
        std::cerr << "Parameters for DoorAnimationAction is missing, skipping." << std::endl;
        return false;
    }
    this->stateResetTime = parameters[5].value.longValue;

    if(parameters[4].value.longValues[0] != 0) {
        std::vector<LimonTypes::GenericParameter>enterResult = limonAPI->getResultOfTrigger(
                static_cast<uint32_t>(parameters[4].value.longValues[1]), 2);                   //is entered?
        std::vector<LimonTypes::GenericParameter>exitResult  = limonAPI->getResultOfTrigger(
                static_cast<uint32_t>(parameters[4].value.longValues[1]), 3);                   //is exited?
        if(enterResult.size() == 1 && enterResult[0].value.boolValue == true &&
                exitResult.size() == 1 && exitResult[0].value.boolValue == false
                ) {
            return false;//skip animation
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
    std::function<void(const std::vector<LimonTypes::GenericParameter>&)> methodToCall = std::bind(&DoorAnimationAction::resetAnimationRun, this, std::placeholders::_1);
    std::vector<LimonTypes::GenericParameter> emptyParams;

    limonAPI->addTimedEvent(this->stateResetTime, false, methodToCall, emptyParams);

    return true;
}


std::vector<LimonTypes::GenericParameter> DoorAnimationAction::getResults() {
    std::vector<LimonTypes::GenericParameter> result;
    LimonTypes::GenericParameter request;
    request.value.boolValue = this->animationRun;
    result.push_back(request);
    return result;
}

DoorAnimationAction::DoorAnimationAction(LimonAPI *limonAPI) : TriggerInterface(limonAPI) {}

void DoorAnimationAction::resetAnimationRun(const std::vector<LimonTypes::GenericParameter> &) {
    this->animationRun = false;
}
