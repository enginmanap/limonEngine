//
// Created by engin on 28.05.2018.
//

#include <iostream>
#include "UpdateGuiTextOnTrigger.h"

TriggerRegister<UpdateGuiTextOnTrigger> UpdateGuiTextOnTrigger::reg("UpdateGuiTextOnTrigger");


std::vector<LimonTypes::GenericParameter> UpdateGuiTextOnTrigger::getParameters() {
    std::vector<LimonTypes::GenericParameter> parameters;
    LimonTypes::GenericParameter pr;
    pr.valueType = LimonTypes::GenericParameter::ValueTypes::LONG;
    pr.requestType = LimonTypes::GenericParameter::RequestParameterTypes::GUI_TEXT;
    pr.description = "GUI Text element";
    parameters.push_back(pr);

    LimonTypes::GenericParameter pr2;
    pr2.valueType = LimonTypes::GenericParameter::ValueTypes::STRING;
    pr2.requestType = LimonTypes::GenericParameter::RequestParameterTypes::FREE_TEXT;
    pr2.description = "New Text";
    parameters.push_back(pr2);

    return parameters;
}

bool UpdateGuiTextOnTrigger::run(std::vector<LimonTypes::GenericParameter> parameters) {
    return limonAPI->updateGuiText(parameters[0].value.longValue, parameters[1].value.stringValue);
}

std::vector<LimonTypes::GenericParameter> UpdateGuiTextOnTrigger::getResults() {
    return std::vector<LimonTypes::GenericParameter>();
}

UpdateGuiTextOnTrigger::UpdateGuiTextOnTrigger(LimonAPI *limonAPI) : TriggerInterface(limonAPI) {}

