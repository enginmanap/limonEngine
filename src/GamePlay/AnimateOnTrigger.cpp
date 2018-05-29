//
// Created by engin on 20.05.2018.
//

#include "AnimateOnTrigger.h"
#include "LimonAPI.h"

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
    parameters.push_back(param3);

    return parameters;
}

bool AnimateOnTrigger::run(std::vector<LimonAPI::ParameterRequest> parameters) {
    LimonAPI::animateModel(static_cast<uint32_t>(parameters[0].value.longValue),
                           static_cast<uint32_t>(parameters[1].value.longValue),
                           parameters[2].value.boolValue);
    return true;
}


std::vector<LimonAPI::ParameterRequest> AnimateOnTrigger::getResults() {
    return std::vector<LimonAPI::ParameterRequest>();
}