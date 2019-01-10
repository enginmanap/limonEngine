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

uint32_t LimonAPI::addGuiImage(const std::string &imageFilePath, const std::string &name, const Vec2 &position,
                               const Vec2 &scale, float rotation) {
    return worldAddGuiImage(imageFilePath, name, position, scale, rotation);
}

uint32_t LimonAPI::addObject(const std::string &modelFilePath, float modelWeight, bool physical,
                             const glm::vec3 &position,
                             const glm::vec3 &scale, const glm::quat &orientation) {
    return worldAddModel(modelFilePath, modelWeight, physical, position, scale, orientation);
}

bool LimonAPI::attachObjectToObject(uint32_t objectID, uint32_t objectToAttachToID) {
    return worldAttachObjectToObject(objectID, objectToAttachToID);
}

bool LimonAPI::updateGuiText(uint32_t guiTextID, const std::string &newText) {
    return worldUpdateGuiText(guiTextID, newText);
}

uint32_t LimonAPI::removeGuiElement(uint32_t guiElementID) {
    return worldRemoveGuiElement(guiElementID);

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
        case COORDINATE: {
            currentElement->SetText("Coordinate");
        }
            break;
        case TRANSFORM: {
            currentElement->SetText("Transform");
        }
            break;
        case MULTI_SELECT: {
            currentElement->SetText("MultiSelect");
        }

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
        case VEC4: {
            currentElement->SetText("Vec4");
            tinyxml2::XMLElement *xELement = document.NewElement("X");
            xELement->SetText(std::to_string(value.vectorValue.x).c_str());
            valueElement->InsertEndChild(xELement);
            tinyxml2::XMLElement *yELement = document.NewElement("Y");
            xELement->SetText(std::to_string(value.vectorValue.y).c_str());
            valueElement->InsertEndChild(yELement);
            tinyxml2::XMLElement *zELement = document.NewElement("Z");
            xELement->SetText(std::to_string(value.vectorValue.z).c_str());
            valueElement->InsertEndChild(zELement);
            tinyxml2::XMLElement *wELement = document.NewElement("W");
            xELement->SetText(std::to_string(value.vectorValue.w).c_str());
            valueElement->InsertEndChild(wELement);
        }
            break;
        case MAT4: {
            currentElement->SetText("Mat4");

            for (int32_t i = 0; i < 4; ++i) {
                tinyxml2::XMLElement *rowELement = document.NewElement(std::to_string(i).c_str());
                tinyxml2::XMLElement *xELement = document.NewElement("X");
                xELement->SetText(std::to_string(value.matrixValue[i].x).c_str());
                rowELement->InsertEndChild(xELement);
                tinyxml2::XMLElement *yELement = document.NewElement("Y");
                xELement->SetText(std::to_string(value.matrixValue[i].y).c_str());
                rowELement->InsertEndChild(yELement);
                tinyxml2::XMLElement *zELement = document.NewElement("Z");
                xELement->SetText(std::to_string(value.matrixValue[i].z).c_str());
                rowELement->InsertEndChild(zELement);
                tinyxml2::XMLElement *wELement = document.NewElement("W");
                xELement->SetText(std::to_string(value.matrixValue[i].w).c_str());
                rowELement->InsertEndChild(wELement);
                valueElement->InsertEndChild(rowELement);
            }
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
    } else if(strcmp(parameterAttribute->GetText(), "Coordinate") == 0) {
        this->requestType = RequestParameterTypes::COORDINATE;
    } else if(strcmp(parameterAttribute->GetText(), "Transform") == 0) {
        this->requestType = RequestParameterTypes::TRANSFORM;
    } else if(strcmp(parameterAttribute->GetText(), "MultiSelect") == 0) {
        this->requestType = RequestParameterTypes::MULTI_SELECT;
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
    } else if(strcmp(parameterAttribute->GetText(),"Vec4")== 0) {
        this->valueType = ValueTypes::VEC4;
        if(this->isSet) {
            parameterAttribute = parameterNode->FirstChildElement("Value");
            loadVec4(parameterAttribute, this->value.vectorValue);
        }
    } else if(strcmp(parameterAttribute->GetText(),"Mat4")== 0) {
        this->valueType = ValueTypes::MAT4;
        if(this->isSet) {
            parameterAttribute = parameterNode->FirstChildElement("Value");
            for(long i = 1; i < value.longValues[0]; i++) {
                tinyxml2::XMLElement *rowNode = parameterAttribute->FirstChildElement(std::to_string(i).c_str());;
                loadVec4(rowNode, this->value.matrixValue[i]);
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


std::vector<LimonAPI::ParameterRequest> LimonAPI::rayCastToCursor() {
    return worldRayCastToCursor();
}

std::vector<LimonAPI::ParameterRequest> LimonAPI::getObjectTransformation(uint32_t objectID) {
    return worldGetObjectTransformation(objectID);
}

std::vector<LimonAPI::ParameterRequest> LimonAPI::getObjectTransformationMatrix(uint32_t objectID) {
    return worldGetObjectTransformationMatrix(objectID);
}

bool LimonAPI::interactWithAI(uint32_t AIID, std::vector<LimonAPI::ParameterRequest> &interactionInformation) {
    return worldInteractWithAI(AIID, interactionInformation);
}

void LimonAPI::interactWithPlayer(std::vector<LimonAPI::ParameterRequest> &input) {
    return this->worldInteractWithPlayer(input);
}

bool LimonAPI::addLightTranslate(uint32_t lightID, const LimonAPI::Vec4 &position) {
    return worldAddLightTranslate(lightID, position);
}

bool LimonAPI::setLightColor(uint32_t lightID, const LimonAPI::Vec4 &color){
    return worldSetLightColor(lightID, color);
}

void LimonAPI::addTimedEvent(long waitTime,
                             std::function<void(const std::vector<LimonAPI::ParameterRequest> &)> methodToCall,
                             std::vector<LimonAPI::ParameterRequest> parameters) {
    worldAddTimedEvent(waitTime, methodToCall, parameters);
}



LimonAPI::Vec4 LimonAPI::getPlayerAttachedModelOffset() {
    return worldGetPlayerAttachmentOffset();
}

bool LimonAPI::setPlayerAttachedModelOffset(LimonAPI::Vec4 newOffset) {
    return worldSetPlayerAttachmentOffset(newOffset);
}

uint32_t LimonAPI::getPlayerAttachedModel() {
    return worldGetPlayerAttachedModel();
}

std::vector<uint32_t> LimonAPI::getModelChildren(uint32_t modelID) {
    return worldGetModelChildren(modelID);
}

std::string LimonAPI::getModelAnimationName(uint32_t modelID) {
    return worldGetModelAnimationName(modelID);
}

bool LimonAPI::getModelAnimationFinished(uint32_t modelID) {
    return worldGetModelAnimationFinished(modelID);
}

bool LimonAPI::setModelAnimation(uint32_t modelID, std::string animationName, bool isLooped) {
    return worldSetAnimationOfModel(modelID, animationName, isLooped);
}

bool LimonAPI::setModelAnimationWithBlend(uint32_t modelID, std::string animationName, bool isLooped, long blendTime) {
    return worldSetAnimationOfModelWithBlend(modelID, animationName, isLooped, blendTime);
}

void LimonAPI::killPlayer() {
    worldKillPlayer();
}

bool LimonAPI::setObjectTranslate(uint32_t objectID, const LimonAPI::Vec4 &position) {
    return worldSetObjectTranslate(objectID, position);
}

bool LimonAPI::setObjectScale(uint32_t objectID, const LimonAPI::Vec4 &scale) {
    return worldSetObjectScale(objectID, scale);
}

bool LimonAPI::setObjectOrientation(uint32_t objectID, const LimonAPI::Vec4 &orientation) {
    return worldSetObjectOrientation(objectID, orientation);
}

bool LimonAPI::addObjectTranslate(uint32_t objectID, const LimonAPI::Vec4 &position) {
    return worldAddObjectTranslate(objectID, position);
}

bool LimonAPI::addObjectScale(uint32_t objectID, const LimonAPI::Vec4 &scale) {
    return worldAddObjectScale(objectID, scale);
}

bool LimonAPI::addObjectOrientation(uint32_t objectID, const LimonAPI::Vec4 &orientation) {
    return worldAddObjectOrientation(objectID, orientation);
}

void LimonAPI::loadVec4(tinyxml2::XMLNode *vectorNode, LimonAPI::Vec4 &vector) {
    tinyxml2::XMLElement *vectorElementNode = vectorNode->FirstChildElement("X");
    if(vectorElementNode != nullptr) {
        vector.x = std::stof(vectorElementNode->GetText());
    } else {
        vector.x = 0;
    }
    vectorElementNode = vectorNode->FirstChildElement("Y");
    if(vectorElementNode != nullptr) {
        vector.y = std::stof(vectorElementNode->GetText());
    } else {
        vector.y = 0;
    }
    vectorElementNode = vectorNode->FirstChildElement("Z");
    if(vectorElementNode != nullptr) {
        vector.z = std::stof(vectorElementNode->GetText());
    } else {
        vector.z = 0;
    }
    vectorElementNode = vectorNode->FirstChildElement("W");
    if(vectorElementNode != nullptr) {
        vector.w = std::stof(vectorElementNode->GetText());
    } else {
        vector.w = 0;
    }
}

bool LimonAPI::setObjectTemporary(uint32_t modelID, bool temporary) {
    return worldSetModelTemporary(modelID, temporary);
}
