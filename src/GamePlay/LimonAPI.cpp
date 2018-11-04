//
// Created by engin on 13.05.2018.
//

#include "LimonAPI.h"
#include <tinyxml2.h>
#include <iostream>

uint32_t LimonAPI::animateModel(uint32_t modelID, uint32_t animationID, bool looped, const std::string *soundPath) {
    return worldAddAnimationToObject(modelID, animationID, looped, soundPath);
}

bool LimonAPI::generateEditorElementsForParameters(std::vector<ParameterRequest> &runParameters, uint32_t index) {
    return worldGenerateEditorElementsForParameters(runParameters, index);
}

uint32_t LimonAPI::addGuiText(const std::string &fontFilePath, uint32_t fontSize, const std::string &name, const std::string &text,
                              const glm::vec3 &color, const glm::vec2 &position, float rotation) {
    return worldAddGuiText(fontFilePath, fontSize, name, text, color, position,rotation);
}

uint32_t LimonAPI::addObject(const std::string &modelFilePath, float modelWeight, bool physical,
                             const glm::vec3 &position,
                             const glm::vec3 &scale, const glm::quat &orientation) {
    return worldAddModel(modelFilePath, modelWeight, physical, position, scale, orientation);
}


bool LimonAPI::updateGuiText(uint32_t guiTextID, const std::string &newText) {
    return worldUpdateGuiText(guiTextID, newText);
}

uint32_t LimonAPI::removeGuiElement(uint32_t guiElementID) {
    return worldRemoveGuiText(guiElementID);

}

std::vector<LimonAPI::ParameterRequest> LimonAPI::getResultOfTrigger(uint32_t TriggerObjectID, uint32_t TriggerCodeID) {
    std::vector<LimonAPI::ParameterRequest> results = worldGetResultOfTrigger(TriggerObjectID, TriggerCodeID);
    return results;
}

bool LimonAPI::removeObject(uint32_t objectID) {
    return worldRemoveObject(objectID);
}

bool LimonAPI::removeTriggerObject(uint32_t TriggerObjectID) {
    return worldRemoveTriggerObject(TriggerObjectID);
}

bool LimonAPI::disconnectObjectFromPhysics(uint32_t modelID) {
    return worldDisconnectObjectFromPhysics(modelID);
}

bool LimonAPI::reconnectObjectToPhysics(uint32_t modelID) {
    return worldReconnectObjectToPhysics(modelID);
}

bool LimonAPI::attachSoundToObjectAndPlay(uint32_t objectWorldID, const std::string &soundPath) {
    return worldAttachSoundToObjectAndPlay(objectWorldID, soundPath);
}
bool LimonAPI::detachSoundFromObject(uint32_t objectWorldID){
    return worldDetachSoundFromObject(objectWorldID);
}
uint32_t LimonAPI::playSound(const std::string &soundPath, const glm::vec3 &position, bool looped){
    return worldPlaySound(soundPath, position, looped);
}

bool LimonAPI::loadAndSwitchWorld(const std::string& worldFileName) {
    return limonLoadWorld(worldFileName);
}

bool LimonAPI::returnToWorld(const std::string &worldFileName) {
    return this->limonReturnOrLoadWorld(worldFileName);
}

bool LimonAPI::LoadAndRemove(const std::string &worldFileName) {
    return this->limonLoadNewAndRemoveCurrentWorld(worldFileName);
}

void LimonAPI::returnPreviousWorld() {
    this->limonReturnPrevious();
}

void LimonAPI::quitGame() {
    limonExitGame();
}

bool LimonAPI::ParameterRequest::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *ParametersNode,
                                           uint32_t index) const {

    tinyxml2::XMLElement *parameterNode= document.NewElement("Parameter");
    ParametersNode->InsertEndChild(parameterNode);

    tinyxml2::XMLElement *currentElement = document.NewElement("RequestType");
    switch (requestType) {
        case MODEL: {
            currentElement->SetText("Model");
        }
        break;
        case ANIMATION: {
            currentElement->SetText("Animation");
        }
            break;
        case SWITCH: {
            currentElement->SetText("Switch");
        }
            break;
        case FREE_TEXT: {
            currentElement->SetText("FreeText");
        }
            break;
        case TRIGGER: {
            currentElement->SetText("Trigger");
        }
            break;
        case GUI_TEXT: {
            currentElement->SetText("GUIText");
        }
            break;
        case FREE_NUMBER: {
            currentElement->SetText("FreeNumber");
        }
            break;
    }
    parameterNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Description");
    currentElement->SetText(description.c_str());
    parameterNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("valueType");
    tinyxml2::XMLElement *valueElement = document.NewElement("Value");
    switch (valueType) {
        case STRING: {
            currentElement->SetText("String");
            valueElement->SetText(value.stringValue);
        }
            break;
        case DOUBLE: {
            currentElement->SetText("Double");
            valueElement->SetText(std::to_string(value.doubleValue).c_str());
        }
            break;
        case LONG: {
            currentElement->SetText("Long");
            valueElement->SetText(std::to_string(value.longValue).c_str());
        }
            break;
        case BOOLEAN: {
            currentElement->SetText("Boolean");
            if(value.boolValue) {
                valueElement->SetText("True");
            } else {
                valueElement->SetText("False");
            }
        }
            break;
        case LONG_ARRAY: {
            currentElement->SetText("LongArray");
            std::string commaSeperatedArray = "";
            for (int32_t i = 0; i < value.longValues[0]; ++i) {
                commaSeperatedArray = commaSeperatedArray + std::to_string(value.longValues[i]);
                if(i < value.longValues[0] - 1) {//if not last element
                    commaSeperatedArray = commaSeperatedArray + ",";
                }
            }
            valueElement->SetText(commaSeperatedArray.c_str());
        }
        break;
        default:
            currentElement->SetText("UNKNOWN");
    }
    parameterNode->InsertEndChild(valueElement);
    parameterNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("IsSet");
    if(isSet) {
        currentElement->SetText("True");
    } else {
        currentElement->SetText("False");
    }
    parameterNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Index");
    currentElement->SetText(index);
    parameterNode->InsertEndChild(currentElement);
    return true;
}

bool LimonAPI::ParameterRequest::
deserialize(tinyxml2::XMLElement *parameterNode, uint32_t &index) {
    tinyxml2::XMLElement* parameterAttribute;

    parameterAttribute = parameterNode->FirstChildElement("RequestType");
    if (parameterAttribute == nullptr) {
        std::cerr << "Trigger parameter must have a Request type." << std::endl;
        return false;
    }
    if(strcmp(parameterAttribute->GetText(), "Model") == 0) {
        this->requestType = RequestParameterTypes::MODEL;
    } else if(strcmp(parameterAttribute->GetText(), "Animation") == 0) {
        this->requestType = RequestParameterTypes::ANIMATION;
    } else if(strcmp(parameterAttribute->GetText(), "Switch") == 0) {
        this->requestType = RequestParameterTypes::SWITCH;
    } else if(strcmp(parameterAttribute->GetText(), "FreeText") == 0) {
        this->requestType = RequestParameterTypes::FREE_TEXT;
    } else if(strcmp(parameterAttribute->GetText(), "Trigger") == 0) {
        this->requestType = RequestParameterTypes::TRIGGER;
    } else if(strcmp(parameterAttribute->GetText(), "GUIText") == 0) {
        this->requestType = RequestParameterTypes::GUI_TEXT;
    } else if(strcmp(parameterAttribute->GetText(), "FreeNumber") == 0) {
        this->requestType = RequestParameterTypes::FREE_NUMBER;
    } else {
        std::cerr << "Trigger parameter request type was unknown. " << parameterAttribute->GetText() << std::endl;
        return false;
    }

    parameterAttribute = parameterNode->FirstChildElement("Description");
    if (parameterAttribute == nullptr) {
        std::cerr << "Trigger parameter must have a description." << std::endl;
        return false;
    }
    this->description = parameterAttribute->GetText();

    parameterAttribute = parameterNode->FirstChildElement("IsSet");
    if (parameterAttribute == nullptr) {
        std::cerr << "Trigger parameter Didn't have isSet set, defaulting to False." << std::endl;
        this->isSet = false;
    } else {
        if(strcmp(parameterAttribute->GetText(), "True") == 0) {
            this->isSet = true;
        } else if(strcmp(parameterAttribute->GetText(), "False") == 0) {
            this->isSet = false;
        } else {
            std::cerr << "Trigger parameter isSet setting is unknown value ["<< parameterAttribute->GetText()  <<"], can't be loaded " << std::endl;
            return false;
        }
    }

    parameterAttribute = parameterNode->FirstChildElement("valueType");
    if (parameterAttribute == nullptr) {
        std::cerr << "Trigger parameter must have a Value type." << std::endl;
        return false;
    }
    if(strcmp(parameterAttribute->GetText(),"String") == 0)  {
        this->valueType = ValueTypes::STRING;
        if(this->isSet) {
            parameterAttribute = parameterNode->FirstChildElement("Value");
            if(parameterAttribute != nullptr && parameterAttribute->GetText() != nullptr) {
                std::string temp = parameterAttribute->GetText();
                snprintf(this->value.stringValue, 63, "%s", temp.c_str());
            }
        }
    } else if(strcmp(parameterAttribute->GetText(),"Double") == 0) {
        this->valueType = ValueTypes::DOUBLE;
        if(this->isSet) {
            parameterAttribute = parameterNode->FirstChildElement("Value");
            this->value.doubleValue = std::stod(parameterAttribute->GetText());
        }
    } else if(strcmp(parameterAttribute->GetText(),"Long")== 0) {
        this->valueType = ValueTypes::LONG;
        if(this->isSet) {
            parameterAttribute = parameterNode->FirstChildElement("Value");
            this->value.longValue = std::stol(parameterAttribute->GetText());
        }
    } else if(strcmp(parameterAttribute->GetText(), "Boolean")== 0) {
        this->valueType = ValueTypes::BOOLEAN;
        if(this->isSet) {
            parameterAttribute = parameterNode->FirstChildElement("Value");
            if(strcmp(parameterAttribute->GetText(), "True") == 0) {
                value.boolValue = true;
            } else if(strcmp(parameterAttribute->GetText(), "False")== 0) {
                value.boolValue = false;
            } else {
                std::cerr << "Trigger parameter boolean value setting is unknown value ["<< parameterAttribute->GetText()  <<"], can't be loaded " << std::endl;
                return false;
            }
        }
    } else if(strcmp(parameterAttribute->GetText(),"LongArray")== 0) {
        this->valueType = ValueTypes::LONG_ARRAY;
        if(this->isSet) {
            parameterAttribute = parameterNode->FirstChildElement("Value");
            std::string commaSeperatedParameterString = parameterAttribute->GetText();
            //the parameters are comma seperated, seperate
            std::size_t commaPosition = commaSeperatedParameterString.find(",");
            value.longValues[0] = std::stol(commaSeperatedParameterString.substr(0, commaPosition));
            commaSeperatedParameterString = commaSeperatedParameterString.substr(commaPosition + 1);
            for(long i = 1; i < value.longValues[0]; i++) {
                std::size_t commaPosition = commaSeperatedParameterString.find(",");
                value.longValues[i] = std::stol(commaSeperatedParameterString.substr(0, commaPosition));
                commaSeperatedParameterString = commaSeperatedParameterString.substr(commaPosition + 1);
            }
        }
    } else {
        std::cerr << "Trigger parameter value type was unknown." << std::endl;
        return false;
    }

    parameterAttribute = parameterNode->FirstChildElement("Index");
    if (parameterAttribute == nullptr) {
        std::cerr << "Trigger parameter must have an index." << std::endl;
        return false;
    }
    index = std::stol(parameterAttribute->GetText());
    return true;
}
