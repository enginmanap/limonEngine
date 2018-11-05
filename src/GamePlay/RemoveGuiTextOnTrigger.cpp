//
// Created by engin on 28.05.2018.
//

#include <iostream>
#include "RemoveGuiTextOnTrigger.h"


TriggerRegister<RemoveGuiTextOnTrigger> RemoveGuiTextOnTrigger::reg("RemoveGuiTextOnTrigger");

std::vector<LimonAPI::ParameterRequest> RemoveGuiTextOnTrigger::getParameters() {
    std::vector<LimonAPI::ParameterRequest> parameters;
    LimonAPI::ParameterRequest pr;
    pr.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG_ARRAY;
    pr.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::TRIGGER;
    pr.description = "Text creator Trigger";
    parameters.push_back(pr);

    return parameters;
}

bool RemoveGuiTextOnTrigger::run(std::vector<LimonAPI::ParameterRequest> parameters) {
    //when run, ask LimonAPI for current result of the action we are set for.
    std::vector<LimonAPI::ParameterRequest> results = limonAPI->getResultOfTrigger(parameters[0].value.longValues[1], parameters[0].value.longValues[2]);
    if(results.size() > 0) {
        limonAPI->removeGuiElement(results[0].value.longValue);
        return true;
    }
    return false;
}

std::vector<LimonAPI::ParameterRequest> RemoveGuiTextOnTrigger::getResults() {
    return std::vector<LimonAPI::ParameterRequest>();
}

RemoveGuiTextOnTrigger::RemoveGuiTextOnTrigger(LimonAPI *limonAPI) : TriggerInterface(limonAPI) {}
