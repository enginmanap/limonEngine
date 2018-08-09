//
// Created by engin on 28.05.2018.
//

#include "ChangeWorldOnTrigger.h"

TriggerRegister<ChangeWorldOnTrigger> ChangeWorldOnTrigger::reg("ChangeWorldOnTrigger");


std::vector<LimonAPI::ParameterRequest> ChangeWorldOnTrigger::getParameters() {
    std::vector<LimonAPI::ParameterRequest> parameters;

    LimonAPI::ParameterRequest pr;
    pr.valueType = LimonAPI::ParameterRequest::ValueTypes::STRING;
    pr.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::FREE_TEXT;
    pr.description = "World file path";
    parameters.push_back(pr);

    LimonAPI::ParameterRequest pr2;
    pr2.valueType = LimonAPI::ParameterRequest::ValueTypes::BOOLEAN;
    pr2.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::SWITCH;
    pr2.description = "Force new";
    parameters.push_back(pr2);

    return parameters;
}

bool ChangeWorldOnTrigger::run(std::vector<LimonAPI::ParameterRequest> parameters) {
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

std::vector<LimonAPI::ParameterRequest> ChangeWorldOnTrigger::getResults() {
    std::vector<LimonAPI::ParameterRequest> temp;
    LimonAPI::ParameterRequest pr;
    pr.valueType = LimonAPI::ParameterRequest::ValueTypes::BOOLEAN;
    pr.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::SWITCH;
    pr.description = "Is world changed";
    pr.value.boolValue = true;
    temp.push_back(pr);

    return temp;
}

ChangeWorldOnTrigger::ChangeWorldOnTrigger(LimonAPI *limonAPI) : TriggerInterface(limonAPI) {}

