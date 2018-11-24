//
// Created by engin on 27.11.2017.
//

#include <iostream>
#include <glm/gtc/quaternion.hpp>
#include "ActorInterface.h"

std::map<std::string, ActorInterface*(*)(uint32_t, LimonAPI*)>* ActorInterface::typeMap;

glm::vec3 ActorInterface::getFrontVector() const {
    std::vector<LimonAPI::ParameterRequest> parameters = limonAPI->getObjectTransformation(modelID);
    glm::quat rotation(0,0,1,0);
    if(parameters.size() >= 3) {
        rotation = glm::quat(parameters[2].value.vectorValue.x,
                             parameters[2].value.vectorValue.y,
                             parameters[2].value.vectorValue.z,
                             parameters[2].value.vectorValue.w);
    } else {
        std::cerr << "ActorInterface Model transform can't be found for actor " << this->getModelID() << " and model " << modelID << std::endl;
    }

    // Extract the vector part of the quaternion
    glm::vec3 u(rotation.x, rotation.y, rotation.z);
    glm::vec3 forward(0.0f,0.0f,1.0f);
    // Extract the scalar part of the quaternion
    float s = rotation.w;

    // Do the math
    glm::vec3 vprime = 2.0f * glm::dot(u, forward) * u
                       + (s*s - glm::dot(u, u)) * forward
                       + 2.0f * s * glm::cross(u, forward);
    return vprime;
}

glm::vec3 ActorInterface::getPosition() const {
    std::vector<LimonAPI::ParameterRequest> parameters = limonAPI->getObjectTransformation(modelID);
    glm::vec3 position(0,0,0);
    if(parameters.size() >= 1) {
        position = glm::vec3(parameters[0].value.vectorValue.x,
                             parameters[0].value.vectorValue.y,
                             parameters[0].value.vectorValue.z);
    } else {
        std::cerr << "ActorInterface Model transform can't be found for actor " << this->getModelID() << " and model " << modelID << std::endl;
    }
    return position;
}

void ActorInterface::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode) const {
    tinyxml2::XMLElement *AINode = document.NewElement("Actor");
    parentNode->InsertEndChild(AINode);

    tinyxml2::XMLElement *idNode = document.NewElement("ID");
    idNode->SetText(std::to_string(this->getWorldID()).c_str());
    AINode->InsertEndChild(idNode);

    tinyxml2::XMLElement *nameNode = document.NewElement("TypeName");
    nameNode->SetText(this->getName().c_str());
    AINode->InsertEndChild(nameNode);

    tinyxml2::XMLElement *parametersNode = document.NewElement("parameters");
    std::vector<LimonAPI::ParameterRequest> parameters = this->getParameters();
    for (size_t i = 0; i < parameters.size(); ++i) {
        parameters[i].serialize(document, parametersNode, i);
    }
    AINode->InsertEndChild(parametersNode);
}

ActorInterface *
ActorInterface::deserializeActorInterface(tinyxml2::XMLElement *actorNode, LimonAPI *limonAPI) {
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
            LimonAPI::ParameterRequest request;

            if(!request.deserialize(parameterNode, index)) {
                std::cerr << "Parameter load failed for Actor, it will be using default values." << std::endl;
                parameterLoadSuccess = false;
                break;
            }
            parameters.insert(parameters.begin() + index, request);
            parameterNode = parameterNode->NextSiblingElement("Parameter");
        } // end of while (Trigger parameters)
        if(parameterLoadSuccess) {
            actor->setParameters(parameters);
        }
    }
    return actor;
}
