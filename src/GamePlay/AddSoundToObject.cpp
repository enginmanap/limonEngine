//
// Created by engin on 21.07.2018.
//

#include "AddSoundToObject.h"

TriggerRegister<AddSoundToObject> AddSoundToObject::reg("AddSoundToObject");


AddSoundToObject::AddSoundToObject(LimonAPI *limonAPI) : TriggerInterface(limonAPI) {
    LimonTypes::GenericParameter param1;
    param1.requestType = LimonTypes::GenericParameter::RequestParameterTypes::FREE_TEXT;
    param1.description = "Sound to play";
    this->parameters.push_back(param1);

    LimonTypes::GenericParameter param2;
    param2.requestType = LimonTypes::GenericParameter::RequestParameterTypes::MODEL;
    param2.description = "Model to attach";
    this->parameters.push_back(param2);
}

bool AddSoundToObject::run(std::vector<LimonTypes::GenericParameter> parameters) {
    if(parameters[3].value.stringValue[0] != '\0') {
        std::string soundPath(parameters[0].value.stringValue);
        uint32_t objectID = static_cast<uint32_t>(parameters[1].value.longValue);
        uint32_t soundID = limonAPI->playSound(soundPath, glm::vec3(0.0f, 0.0f, 0.0f), false, true);
        if(soundID == 0) {
            return false;
        }
        limonAPI->attachObjectToObject(soundID, objectID);
        return true;
    }
    return false;
}


std::vector<LimonTypes::GenericParameter> AddSoundToObject::getResults() {
    return std::vector<LimonTypes::GenericParameter>();
}