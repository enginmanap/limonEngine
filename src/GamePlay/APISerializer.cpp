//
// Created by engin on 17.01.2019.
//

#include <c++/8.2.1/iostream>
#include "APISerializer.h"
#include "API/TriggerInterface.h"

bool APISerializer::serializeParameterRequest(const LimonAPI::ParameterRequest &parameterRequest,
                                              tinyxml2::XMLDocument &document, tinyxml2::XMLElement *ParametersNode,
                                              uint32_t index) {

    tinyxml2::XMLElement *parameterNode= document.NewElement("Parameter");
    ParametersNode->InsertEndChild(parameterNode);

    tinyxml2::XMLElement *currentElement = document.NewElement("RequestType");
    switch (parameterRequest.requestType) {
        case LimonAPI::ParameterRequest::MODEL: {
            currentElement->SetText("Model");
        }
            break;
        case LimonAPI::ParameterRequest::ANIMATION: {
            currentElement->SetText("Animation");
        }
            break;
        case LimonAPI::ParameterRequest::SWITCH: {
            currentElement->SetText("Switch");
        }
            break;
        case LimonAPI::ParameterRequest::FREE_TEXT: {
            currentElement->SetText("FreeText");
        }
            break;
        case LimonAPI::ParameterRequest::TRIGGER: {
            currentElement->SetText("Trigger");
        }
            break;
        case LimonAPI::ParameterRequest::GUI_TEXT: {
            currentElement->SetText("GUIText");
        }
            break;
        case LimonAPI::ParameterRequest::FREE_NUMBER: {
            currentElement->SetText("FreeNumber");
        }
            break;
        case LimonAPI::ParameterRequest::COORDINATE: {
            currentElement->SetText("Coordinate");
        }
            break;
        case LimonAPI::ParameterRequest::TRANSFORM: {
            currentElement->SetText("Transform");
        }
            break;
        case LimonAPI::ParameterRequest::MULTI_SELECT: {
            currentElement->SetText("MultiSelect");
        }

    }
    parameterNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Description");
    currentElement->SetText(parameterRequest.description.c_str());
    parameterNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("valueType");
    tinyxml2::XMLElement *valueElement = document.NewElement("Value");
    switch (parameterRequest.valueType) {
        case LimonAPI::ParameterRequest::STRING: {
            currentElement->SetText("String");
            valueElement->SetText(parameterRequest.value.stringValue);
        }
            break;
        case LimonAPI::ParameterRequest::DOUBLE: {
            currentElement->SetText("Double");
            valueElement->SetText(std::to_string(parameterRequest.value.doubleValue).c_str());
        }
            break;
        case LimonAPI::ParameterRequest::LONG: {
            currentElement->SetText("Long");
            valueElement->SetText(std::to_string(parameterRequest.value.longValue).c_str());
        }
            break;
        case LimonAPI::ParameterRequest::BOOLEAN: {
            currentElement->SetText("Boolean");
            if(parameterRequest.value.boolValue) {
                valueElement->SetText("True");
            } else {
                valueElement->SetText("False");
            }
        }
            break;
        case LimonAPI::ParameterRequest::LONG_ARRAY: {
            currentElement->SetText("LongArray");
            std::string commaSeperatedArray = "";
            for (int32_t i = 0; i < parameterRequest.value.longValues[0]; ++i) {
                commaSeperatedArray = commaSeperatedArray + std::to_string(parameterRequest.value.longValues[i]);
                if(i < parameterRequest.value.longValues[0] - 1) {//if not last element
                    commaSeperatedArray = commaSeperatedArray + ",";
                }
            }
            valueElement->SetText(commaSeperatedArray.c_str());
        }
            break;
        case LimonAPI::ParameterRequest::VEC4: {
            currentElement->SetText("Vec4");
            tinyxml2::XMLElement *xELement = document.NewElement("X");
            xELement->SetText(std::to_string(parameterRequest.value.vectorValue.x).c_str());
            valueElement->InsertEndChild(xELement);
            tinyxml2::XMLElement *yELement = document.NewElement("Y");
            xELement->SetText(std::to_string(parameterRequest.value.vectorValue.y).c_str());
            valueElement->InsertEndChild(yELement);
            tinyxml2::XMLElement *zELement = document.NewElement("Z");
            xELement->SetText(std::to_string(parameterRequest.value.vectorValue.z).c_str());
            valueElement->InsertEndChild(zELement);
            tinyxml2::XMLElement *wELement = document.NewElement("W");
            xELement->SetText(std::to_string(parameterRequest.value.vectorValue.w).c_str());
            valueElement->InsertEndChild(wELement);
        }
            break;
        case LimonAPI::ParameterRequest::MAT4: {
            currentElement->SetText("Mat4");

            for (int32_t i = 0; i < 4; ++i) {
                tinyxml2::XMLElement *rowELement = document.NewElement(std::to_string(i).c_str());
                tinyxml2::XMLElement *xELement = document.NewElement("X");
                xELement->SetText(std::to_string(parameterRequest.value.matrixValue[i].x).c_str());
                rowELement->InsertEndChild(xELement);
                tinyxml2::XMLElement *yELement = document.NewElement("Y");
                xELement->SetText(std::to_string(parameterRequest.value.matrixValue[i].y).c_str());
                rowELement->InsertEndChild(yELement);
                tinyxml2::XMLElement *zELement = document.NewElement("Z");
                xELement->SetText(std::to_string(parameterRequest.value.matrixValue[i].z).c_str());
                rowELement->InsertEndChild(zELement);
                tinyxml2::XMLElement *wELement = document.NewElement("W");
                xELement->SetText(std::to_string(parameterRequest.value.matrixValue[i].w).c_str());
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
    if(parameterRequest.isSet) {
        currentElement->SetText("True");
    } else {
        currentElement->SetText("False");
    }
    parameterNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Index");
    currentElement->SetText(index);
    parameterNode->InsertEndChild(currentElement);
    return true;}

std::shared_ptr<LimonAPI::ParameterRequest>
APISerializer::deserializeParameterRequest(tinyxml2::XMLElement *parameterNode, uint32_t &index) {
    tinyxml2::XMLElement* parameterAttribute;

    parameterAttribute = parameterNode->FirstChildElement("RequestType");
    if (parameterAttribute == nullptr) {
        std::cerr << "Trigger parameter must have a Request type." << std::endl;
        return nullptr;
    }
    std::shared_ptr<LimonAPI::ParameterRequest> newParameterRequest = std::make_shared<LimonAPI::ParameterRequest>();
    if(strcmp(parameterAttribute->GetText(), "Model") == 0) {
        newParameterRequest->requestType = LimonAPI::ParameterRequest::RequestParameterTypes::MODEL;
    } else if(strcmp(parameterAttribute->GetText(), "Animation") == 0) {
        newParameterRequest->requestType = LimonAPI::ParameterRequest::RequestParameterTypes::ANIMATION;
    } else if(strcmp(parameterAttribute->GetText(), "Switch") == 0) {
        newParameterRequest->requestType = LimonAPI::ParameterRequest::RequestParameterTypes::SWITCH;
    } else if(strcmp(parameterAttribute->GetText(), "FreeText") == 0) {
        newParameterRequest->requestType = LimonAPI::ParameterRequest::RequestParameterTypes::FREE_TEXT;
    } else if(strcmp(parameterAttribute->GetText(), "Trigger") == 0) {
        newParameterRequest->requestType = LimonAPI::ParameterRequest::RequestParameterTypes::TRIGGER;
    } else if(strcmp(parameterAttribute->GetText(), "GUIText") == 0) {
        newParameterRequest->requestType = LimonAPI::ParameterRequest::RequestParameterTypes::GUI_TEXT;
    } else if(strcmp(parameterAttribute->GetText(), "FreeNumber") == 0) {
        newParameterRequest->requestType = LimonAPI::ParameterRequest::RequestParameterTypes::FREE_NUMBER;
    } else if(strcmp(parameterAttribute->GetText(), "Coordinate") == 0) {
        newParameterRequest->requestType = LimonAPI::ParameterRequest::RequestParameterTypes::COORDINATE;
    } else if(strcmp(parameterAttribute->GetText(), "Transform") == 0) {
        newParameterRequest->requestType = LimonAPI::ParameterRequest::RequestParameterTypes::TRANSFORM;
    } else if(strcmp(parameterAttribute->GetText(), "MultiSelect") == 0) {
        newParameterRequest->requestType = LimonAPI::ParameterRequest::RequestParameterTypes::MULTI_SELECT;
    } else {
        std::cerr << "Trigger parameter request type was unknown. " << parameterAttribute->GetText() << std::endl;
        return nullptr;
    }

    parameterAttribute = parameterNode->FirstChildElement("Description");
    if (parameterAttribute == nullptr) {
        std::cerr << "Trigger parameter must have a description." << std::endl;
        return nullptr;
    }
    newParameterRequest->description = parameterAttribute->GetText();

    parameterAttribute = parameterNode->FirstChildElement("IsSet");
    if (parameterAttribute == nullptr) {
        std::cerr << "Trigger parameter Didn't have isSet set, defaulting to False." << std::endl;
        newParameterRequest->isSet = false;
    } else {
        if(strcmp(parameterAttribute->GetText(), "True") == 0) {
            newParameterRequest->isSet = true;
        } else if(strcmp(parameterAttribute->GetText(), "False") == 0) {
            newParameterRequest->isSet = false;
        } else {
            std::cerr << "Trigger parameter isSet setting is unknown value ["<< parameterAttribute->GetText()  <<"], can't be loaded " << std::endl;
            return nullptr;
        }
    }

    parameterAttribute = parameterNode->FirstChildElement("valueType");
    if (parameterAttribute == nullptr) {
        std::cerr << "Trigger parameter must have a Value type." << std::endl;
        return nullptr;
    }
    if(strcmp(parameterAttribute->GetText(),"String") == 0)  {
        newParameterRequest->valueType = LimonAPI::ParameterRequest::ValueTypes::STRING;
        if(newParameterRequest->isSet) {
            parameterAttribute = parameterNode->FirstChildElement("Value");
            if(parameterAttribute != nullptr && parameterAttribute->GetText() != nullptr) {
                std::string temp = parameterAttribute->GetText();
                snprintf(newParameterRequest->value.stringValue, 63, "%s", temp.c_str());
            }
        }
    } else if(strcmp(parameterAttribute->GetText(),"Double") == 0) {
        newParameterRequest->valueType = LimonAPI::ParameterRequest::ValueTypes::DOUBLE;
        if(newParameterRequest->isSet) {
            parameterAttribute = parameterNode->FirstChildElement("Value");
            newParameterRequest->value.doubleValue = std::stod(parameterAttribute->GetText());
        }
    } else if(strcmp(parameterAttribute->GetText(),"Long")== 0) {
        newParameterRequest->valueType = LimonAPI::ParameterRequest::ValueTypes::LONG;
        if(newParameterRequest->isSet) {
            parameterAttribute = parameterNode->FirstChildElement("Value");
            newParameterRequest->value.longValue = std::stol(parameterAttribute->GetText());
        }
    } else if(strcmp(parameterAttribute->GetText(), "Boolean")== 0) {
        newParameterRequest->valueType = LimonAPI::ParameterRequest::ValueTypes::BOOLEAN;
        if(newParameterRequest->isSet) {
            parameterAttribute = parameterNode->FirstChildElement("Value");
            if(strcmp(parameterAttribute->GetText(), "True") == 0) {
                newParameterRequest->value.boolValue = true;
            } else if(strcmp(parameterAttribute->GetText(), "False")== 0) {
                newParameterRequest->value.boolValue = false;
            } else {
                std::cerr << "Trigger parameter boolean value setting is unknown value ["<< parameterAttribute->GetText()  <<"], can't be loaded " << std::endl;
                return nullptr;
            }
        }
    } else if(strcmp(parameterAttribute->GetText(),"LongArray")== 0) {
        newParameterRequest->valueType = LimonAPI::ParameterRequest::ValueTypes::LONG_ARRAY;
        if(newParameterRequest->isSet) {
            parameterAttribute = parameterNode->FirstChildElement("Value");
            std::string commaSeperatedParameterString = parameterAttribute->GetText();
            //the parameters are comma separated, separate
            std::size_t commaPosition = commaSeperatedParameterString.find(",");
            newParameterRequest->value.longValues[0] = std::stol(commaSeperatedParameterString.substr(0, commaPosition));
            commaSeperatedParameterString = commaSeperatedParameterString.substr(commaPosition + 1);
            for(long i = 1; i < newParameterRequest->value.longValues[0]; i++) {
                std::size_t commaPosition = commaSeperatedParameterString.find(",");
                newParameterRequest->value.longValues[i] = std::stol(commaSeperatedParameterString.substr(0, commaPosition));
                commaSeperatedParameterString = commaSeperatedParameterString.substr(commaPosition + 1);
            }
        }
    } else if(strcmp(parameterAttribute->GetText(),"Vec4")== 0) {
        newParameterRequest->valueType = LimonAPI::ParameterRequest::ValueTypes::VEC4;
        if(newParameterRequest->isSet) {
            parameterAttribute = parameterNode->FirstChildElement("Value");
            loadVec4(parameterAttribute, newParameterRequest->value.vectorValue);
        }
    } else if(strcmp(parameterAttribute->GetText(),"Mat4")== 0) {
        newParameterRequest->valueType = LimonAPI::ParameterRequest::ValueTypes::MAT4;
        if(newParameterRequest->isSet) {
            parameterAttribute = parameterNode->FirstChildElement("Value");
            for(long i = 1; i < newParameterRequest->value.longValues[0]; i++) {
                tinyxml2::XMLElement *rowNode = parameterAttribute->FirstChildElement(std::to_string(i).c_str());;
                loadVec4(rowNode, newParameterRequest->value.matrixValue[i]);
            }
        }
    } else {
        std::cerr << "Trigger parameter value type was unknown." << std::endl;
        return nullptr;
    }

    parameterAttribute = parameterNode->FirstChildElement("Index");
    if (parameterAttribute == nullptr) {
        std::cerr << "Trigger parameter must have an index." << std::endl;
        return nullptr;
    }
    index = std::stol(parameterAttribute->GetText());
    return newParameterRequest;
}

void APISerializer::loadVec4(tinyxml2::XMLNode *vectorNode, LimonAPI::Vec4 &vector) {
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

bool APISerializer::serializeTriggerCode(const TriggerInterface &trigger, tinyxml2::XMLDocument &document,
                                         tinyxml2::XMLElement *triggerNode, const std::string &triggerCodeNodeName,
                                         const std::vector<LimonAPI::ParameterRequest> &parameters,
                                         bool enabled) {
    tinyxml2::XMLElement *currentElement = document.NewElement(triggerCodeNodeName.c_str());

    tinyxml2::XMLElement* codeElement = document.NewElement("Name");
    codeElement->SetText(trigger.getName().c_str());
    currentElement->InsertEndChild(codeElement);

    //now serialize the parameters
    codeElement = document.NewElement("parameters");
    for (size_t i = 0; i < parameters.size(); ++i) {
        APISerializer::serializeParameterRequest(parameters[i], document, codeElement, i);
    }
    currentElement->InsertEndChild(codeElement);

    codeElement = document.NewElement("Enabled");
    if(enabled) {
        codeElement->SetText("True");
    } else {
        codeElement->SetText("False");
    }
    currentElement->InsertEndChild(codeElement);

    triggerNode->InsertEndChild(currentElement);
    return true;
}

TriggerInterface*
APISerializer::deserializeTriggerCode(tinyxml2::XMLElement *triggersNode, tinyxml2::XMLElement *triggerAttribute,
                                      const std::string &nodeName, LimonAPI *limonAPI,
                                      std::vector<LimonAPI::ParameterRequest> &parameters, bool &enabled) {
    TriggerInterface* triggerCode = nullptr;
    enabled= false;
    triggerAttribute = triggersNode->FirstChildElement(nodeName.c_str());
    if (triggerAttribute != nullptr) {
        tinyxml2::XMLElement* triggerCodeAttribute = triggerAttribute->FirstChildElement("Name");
        triggerCode = TriggerInterface::createTrigger(triggerCodeAttribute->GetText(), limonAPI);

        triggerCodeAttribute = triggerAttribute->FirstChildElement("parameters");

        tinyxml2::XMLElement* triggerCodeParameter = triggerCodeAttribute->FirstChildElement("Parameter");

        uint32_t index;
        while(triggerCodeParameter != nullptr) {
            std::shared_ptr<LimonAPI::ParameterRequest> request = APISerializer::deserializeParameterRequest(triggerCodeParameter, index);

            if(request == nullptr) {
                delete triggerCode;
                return nullptr;
            }
            parameters.insert(parameters.begin() + index, *request);
            triggerCodeParameter = triggerCodeParameter->NextSiblingElement("Parameter");
        } // end of while (Trigger parameters)

        triggerCodeAttribute = triggerAttribute->FirstChildElement("Enabled");
        if (triggerCodeAttribute == nullptr) {
            std::cerr << "Trigger Didn't have enabled set, defaulting to False." << std::endl;
            enabled = false;
        } else {
            if(strcmp(triggerCodeAttribute->GetText(), "True") == 0) {
                enabled = true;
            } else if(strcmp(triggerCodeAttribute->GetText(), "False") == 0) {
                enabled = false;
            } else {
                std::cerr << "Trigger enabled setting is unknown value [" << triggerCodeAttribute->GetText() << "], can't be loaded " << std::endl;
                delete triggerCode;
                return nullptr;
            }
        }
    }
    return triggerCode;

}

void APISerializer::serializeActorInterface(const ActorInterface& actor, tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode) {
    tinyxml2::XMLElement *AINode = document.NewElement("Actor");
    parentNode->InsertEndChild(AINode);

    tinyxml2::XMLElement *idNode = document.NewElement("ID");
    idNode->SetText(std::to_string(actor.getWorldID()).c_str());
    AINode->InsertEndChild(idNode);

    tinyxml2::XMLElement *nameNode = document.NewElement("TypeName");
    nameNode->SetText(actor.getName().c_str());
    AINode->InsertEndChild(nameNode);

    tinyxml2::XMLElement *parametersNode = document.NewElement("parameters");
    std::vector<LimonAPI::ParameterRequest> parameters = actor.getParameters();
    for (size_t i = 0; i < parameters.size(); ++i) {
        serializeParameterRequest(parameters[i], document, parametersNode, i);
    }
    AINode->InsertEndChild(parametersNode);
}

ActorInterface *APISerializer::deserializeActorInterface(tinyxml2::XMLElement *actorNode, LimonAPI *limonAPI) {
    ActorInterface* actor = nullptr;
    if (actorNode != nullptr) {
        std::string typeName;
        tinyxml2::XMLElement* nameNode = actorNode->FirstChildElement("TypeName");
        if(nameNode != nullptr && nameNode->GetText() != nullptr){
            typeName = nameNode->GetText();
        } else {
            std::cerr << "Name can't be found for Actor load, failed." << std::endl;
            return nullptr;
        }
        uint32_t id;
        tinyxml2::XMLElement* idNode = actorNode->FirstChildElement("ID");
        if(idNode != nullptr && idNode->GetText() != nullptr){
            id = std::atoi(idNode->GetText());
        } else {
            std::cerr << "ID can't be found for Actor load, failed." << std::endl;
            return nullptr;
        }

        actor = ActorInterface::createActor(typeName, id, limonAPI);
        if(actor == nullptr) {
            std::cerr << "Actor with given name " << typeName << " can't be created. Please check if extensions loaded successfully." << std::endl;
            return nullptr;
        }
        tinyxml2::XMLElement* allParametersNode = actorNode->FirstChildElement("parameters");

        tinyxml2::XMLElement* parameterNode = allParametersNode->FirstChildElement("Parameter");
        uint32_t index;
        std::vector<LimonAPI::ParameterRequest> parameters;
        bool parameterLoadSuccess = true;
        while(parameterNode != nullptr) {
            std::shared_ptr<LimonAPI::ParameterRequest> request = APISerializer::deserializeParameterRequest(parameterNode, index);
            if(request == nullptr) {
                std::cerr << "Parameter load failed for Actor, it will be using default values." << std::endl;
                parameterLoadSuccess = false;
                break;
            }
            parameters.insert(parameters.begin() + index, *request);
            parameterNode = parameterNode->NextSiblingElement("Parameter");
        } // end of while (Trigger parameters)
        if(parameterLoadSuccess) {
            actor->setParameters(parameters);
        }
    }
    return actor;

}