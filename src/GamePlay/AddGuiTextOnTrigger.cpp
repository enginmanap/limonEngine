//
// Created by engin on 26.05.2018.
//

#include "AddGuiTextOnTrigger.h"

std::vector<LimonAPI::ParameterRequest> AddGuiTextOnTrigger::getParameters() {
    std::vector<LimonAPI::ParameterRequest> parameters;
    LimonAPI::ParameterRequest pr;
    pr.valueType = LimonAPI::ParameterRequest::ValueTypes::STRING;
    pr.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::FREE_TEXT;
    parameters.push_back(pr);
    return parameters;
}

bool AddGuiTextOnTrigger::run(std::vector<LimonAPI::ParameterRequest> parameters) {
    LimonAPI::addGuiText("Data/Fonts/Helvetica-Normal.ttf", 32, std::string(parameters[0].value.stringValue), glm::vec3(0,255,255), glm::vec2(40,40), 0.0f);
    return false;
}
