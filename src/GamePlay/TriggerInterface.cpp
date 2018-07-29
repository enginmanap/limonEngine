//
// Created by engin on 26.05.2018.
//

#include "TriggerInterface.h"

std::map<std::string, TriggerInterface*(*)(LimonAPI*)>* TriggerInterface::typeMap;

void TriggerInterface::serializeTriggerCode(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *triggerNode,
                                            const std::string &triggerCodeNodeName,
                                            const std::vector<LimonAPI::ParameterRequest> &parameters, bool enabled) const {
    tinyxml2::XMLElement *currentElement = document.NewElement(triggerCodeNodeName.c_str());

    tinyxml2::XMLElement* codeElement = document.NewElement("Name");
    codeElement->SetText(this->getName().c_str());
    currentElement->InsertEndChild(codeElement);

    //now serialize the parameters
    codeElement = document.NewElement("parameters");
    for (size_t i = 0; i < parameters.size(); ++i) {
        parameters[i].serialize(document, codeElement, i);
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
}

TriggerInterface * TriggerInterface::deserializeTriggerCode(tinyxml2::XMLElement *triggersNode, tinyxml2::XMLElement *triggerAttribute,
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
            LimonAPI::ParameterRequest request;

            if(!request.deserialize(triggerCodeParameter, index)) {
                delete triggerCode;
                return nullptr;
            }
            parameters.insert(parameters.begin() + index, request);
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