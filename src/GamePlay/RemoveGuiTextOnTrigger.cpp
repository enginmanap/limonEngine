//
// Created by engin on 28.05.2018.
//

#include <iostream>
#include "RemoveGuiTextOnTrigger.h"


TriggerRegister<RemoveGuiTextOnTrigger> RemoveGuiTextOnTrigger::reg("RemoveGuiTextOnTrigger");

std::vector<LimonTypes::GenericParameter> RemoveGuiTextOnTrigger::getParameters() {
    std::vector<LimonTypes::GenericParameter> parameters;
    LimonTypes::GenericParameter pr;
    pr.valueType = LimonTypes::GenericParameter::ValueTypes::LONG_ARRAY;
    pr.requestType = LimonTypes::GenericParameter::RequestParameterTypes::TRIGGER;
    pr.description = "Text creator Trigger";
    parameters.push_back(pr);

    return parameters;
}

bool RemoveGuiTextOnTrigger::run(std::vector<LimonTypes::GenericParameter> parameters) {
    //when run, ask LimonAPI for current result of the action we are set for.
    std::vector<LimonTypes::GenericParameter> results = limonAPI->getResultOfTrigger(parameters[0].value.longValues[1], parameters[0].value.longValues[2]);
    if(results.size() > 0) {
        limonAPI->removeGuiElement(results[0].value.longValue);
        return true;
    }
    return false;
}

std::vector<LimonTypes::GenericParameter> RemoveGuiTextOnTrigger::getResults() {
    return std::vector<LimonTypes::GenericParameter>();
}

RemoveGuiTextOnTrigger::RemoveGuiTextOnTrigger(LimonAPI *limonAPI) : TriggerInterface(limonAPI) {}
