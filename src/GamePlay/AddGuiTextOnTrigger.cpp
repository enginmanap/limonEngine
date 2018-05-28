//
// Created by engin on 26.05.2018.
//

#include <iostream>
#include "AddGuiTextOnTrigger.h"


TriggerRegister<AddGuiTextOnTrigger> AddGuiTextOnTrigger::reg("AddGuiTextOnTrigger");

std::vector<LimonAPI::ParameterRequest> AddGuiTextOnTrigger::getParameters() {
    std::vector<LimonAPI::ParameterRequest> parameters;
    LimonAPI::ParameterRequest pr;
    pr.valueType = LimonAPI::ParameterRequest::ValueTypes::STRING;
    pr.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::FREE_TEXT;
    pr.description = "Text message to add as GUI element";
    parameters.push_back(pr);

    std::cout << "Map elements:" << std::endl;
    for (auto it = getMap()->begin(); it!= getMap()->end(); it++) {
        std::cout << it->first + ", " << std::endl;
    }
    return parameters;
}

bool AddGuiTextOnTrigger::run(std::vector<LimonAPI::ParameterRequest> parameters) {
    LimonAPI::addGuiText("Data/Fonts/Helvetica-Normal.ttf", 32, std::string(parameters[0].value.stringValue), glm::vec3(0,255,255), glm::vec2(40,40), 0.0f);
    return false;
}
