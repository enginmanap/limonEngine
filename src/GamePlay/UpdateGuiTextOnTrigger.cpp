//
// Created by engin on 28.05.2018.
//

#include <iostream>
#include "UpdateGuiTextOnTrigger.h"



std::vector<LimonAPI::ParameterRequest> UpdateGuiTextOnTrigger::getParameters() {
    std::vector<LimonAPI::ParameterRequest> parameters;
    LimonAPI::ParameterRequest pr;
    pr.valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
    pr.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::GUI_TEXT;
    pr.description = "GUI Text element";
    parameters.push_back(pr);

    LimonAPI::ParameterRequest pr2;
    pr2.valueType = LimonAPI::ParameterRequest::ValueTypes::STRING;
    pr2.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::FREE_TEXT;
    pr2.description = "New Text";
    parameters.push_back(pr2);

    return parameters;
}

bool UpdateGuiTextOnTrigger::run(std::vector<LimonAPI::ParameterRequest> parameters) {
    if(limonAPI->updateGuiText(parameters[0].value.longValue, parameters[1].value.stringValue) == 0) {
        return true;
    } else {
        return false;
    }
}

std::vector<LimonAPI::ParameterRequest> UpdateGuiTextOnTrigger::getResults() {
    return std::vector<LimonAPI::ParameterRequest>();
}

UpdateGuiTextOnTrigger::UpdateGuiTextOnTrigger(LimonAPI *limonAPI) : TriggerInterface(limonAPI) {}

