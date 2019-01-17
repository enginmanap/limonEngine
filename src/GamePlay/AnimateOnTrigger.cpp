//
// Created by engin on 20.05.2018.
//

#include "AnimateOnTrigger.h"
#include "API/LimonAPI.h"

TriggerRegister<AnimateOnTrigger> AnimateOnTrigger::reg("AnimateOnTrigger");

std::vector<LimonAPI::ParameterRequest> AnimateOnTrigger::getParameters() {
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

    return parameters;
}

bool AnimateOnTrigger::run(std::vector<LimonAPI::ParameterRequest> parameters) {

    std::string* sound = nullptr;

    if(parameters[3].value.stringValue[0] != '\0') {
        sound = new std::string(parameters[3].value.stringValue);
    }
    limonAPI->animateModel(static_cast<uint32_t>(parameters[0].value.longValue),
                           static_cast<uint32_t>(parameters[1].value.longValue),
                           parameters[2].value.boolValue,
                           sound);
    delete sound;
    return true;
}


std::vector<LimonAPI::ParameterRequest> AnimateOnTrigger::getResults() {
    return std::vector<LimonAPI::ParameterRequest>();
}

AnimateOnTrigger::AnimateOnTrigger(LimonAPI *limonAPI) : TriggerInterface(limonAPI) {}
