//
// Created by engin on 28.05.2018.
//

#include "ChangeWorldOnTrigger.h"

TriggerRegister<ChangeWorldOnTrigger> ChangeWorldOnTrigger::reg("ChangeWorldOnTrigger");


std::vector<LimonTypes::GenericParameter> ChangeWorldOnTrigger::getParameters() {
    std::vector<LimonTypes::GenericParameter> parameters;

    LimonTypes::GenericParameter pr;
    pr.valueType = LimonTypes::GenericParameter::ValueTypes::STRING;
    pr.requestType = LimonTypes::GenericParameter::RequestParameterTypes::FREE_TEXT;
    pr.description = "World file path";
    parameters.push_back(pr);

    LimonTypes::GenericParameter pr2;
    pr2.valueType = LimonTypes::GenericParameter::ValueTypes::BOOLEAN;
    pr2.requestType = LimonTypes::GenericParameter::RequestParameterTypes::SWITCH;
    pr2.description = "Force new";
    parameters.push_back(pr2);

    return parameters;
}

bool ChangeWorldOnTrigger::run(std::vector<LimonTypes::GenericParameter> parameters) {
    if(parameters[1].value.boolValue) {
        if (limonAPI->loadAndSwitchWorld(parameters[0].value.stringValue)) {
            worldChanged = true;
            return true;
        } else {
            return false;
        }
    } else {
        if (limonAPI->returnToWorld(parameters[0].value.stringValue)) {
            worldChanged = true;
            return true;
        } else {
            return false;
        }
    }
}

std::vector<LimonTypes::GenericParameter> ChangeWorldOnTrigger::getResults() {
    std::vector<LimonTypes::GenericParameter> temp;
    LimonTypes::GenericParameter pr;
    pr.valueType = LimonTypes::GenericParameter::ValueTypes::BOOLEAN;
    pr.requestType = LimonTypes::GenericParameter::RequestParameterTypes::SWITCH;
    pr.description = "Is world changed";
    pr.value.boolValue = true;
    temp.push_back(pr);

    return temp;
}

ChangeWorldOnTrigger::ChangeWorldOnTrigger(LimonAPI *limonAPI) : TriggerInterface(limonAPI) {}

