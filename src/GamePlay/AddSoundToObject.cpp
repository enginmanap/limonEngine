//
// Created by engin on 21.07.2018.
//

#include "AddSoundToObject.h"

TriggerRegister<AddSoundToObject> AddSoundToObject::reg("AddSoundToObject");


AddSoundToObject::AddSoundToObject(LimonAPI *limonAPI) : TriggerInterface(limonAPI) {}


std::vector<LimonAPI::ParameterRequest> AddSoundToObject::getParameters() {
    std::vector<LimonAPI::ParameterRequest> parameters;
    LimonAPI::ParameterRequest param1;
    param1.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::FREE_TEXT;
    param1.description = "Sound to play";
    parameters.push_back(param1);

    LimonAPI::ParameterRequest param2;
    param2.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::MODEL;
    param2.description = "Model to attach";
    parameters.push_back(param2);

    return parameters;
}

bool AddSoundToObject::run(std::vector<LimonAPI::ParameterRequest> parameters) {

    std::string* sound = nullptr;

    if(parameters[3].value.stringValue[0] != '\0') {
        sound = new std::string(parameters[0].value.stringValue);
        limonAPI->attachSoundToObjectAndPlay(static_cast<uint32_t>(parameters[1].value.longValue), *sound);
        delete sound;
        return true;
    }
    return false;
}


std::vector<LimonAPI::ParameterRequest> AddSoundToObject::getResults() {
    return std::vector<LimonAPI::ParameterRequest>();
}