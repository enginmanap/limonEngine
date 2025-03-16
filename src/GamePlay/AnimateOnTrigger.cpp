//
// Created by engin on 20.05.2018.
//

#include "AnimateOnTrigger.h"
#include "limonAPI/LimonAPI.h"

TriggerRegister<AnimateOnTrigger> AnimateOnTrigger::reg("AnimateOnTrigger");

std::vector<LimonTypes::GenericParameter> AnimateOnTrigger::getParameters() {
    std::vector<LimonTypes::GenericParameter> parameters;
    LimonTypes::GenericParameter param1;
    param1.requestType = LimonTypes::GenericParameter::RequestParameterTypes::MODEL;
    param1.description = "Model to animate";
    parameters.push_back(param1);

    LimonTypes::GenericParameter param2;
    param2.requestType = LimonTypes::GenericParameter::RequestParameterTypes::ANIMATION;
    param2.description = "Animation to apply";
    parameters.push_back(param2);

    LimonTypes::GenericParameter param3;
    param3.requestType = LimonTypes::GenericParameter::RequestParameterTypes::SWITCH;
    param3.description = "Is animation looped";
    param3.isSet = true;
    parameters.push_back(param3);

    LimonTypes::GenericParameter param4;
    param4.requestType = LimonTypes::GenericParameter::RequestParameterTypes::FREE_TEXT;
    param4.description = "Sound to play";
    param4.isSet = true;
    parameters.push_back(param4);

    return parameters;
}

bool AnimateOnTrigger::run(std::vector<LimonTypes::GenericParameter> parameters) {

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


std::vector<LimonTypes::GenericParameter> AnimateOnTrigger::getResults() {
    return std::vector<LimonTypes::GenericParameter>();
}

AnimateOnTrigger::AnimateOnTrigger(LimonAPI *limonAPI) : TriggerInterface(limonAPI) {}
