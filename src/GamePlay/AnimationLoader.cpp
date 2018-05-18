//
// Created by engin on 17.05.2018.
//

#include <iostream>
#include <glm/gtc/quaternion.hpp>
#include "AnimationLoader.h"

#include "AnimationNode.h"
#include "AnimationCustom.h"

AnimationCustom *AnimationLoader::loadAnimation(const std::string &fileName) {
    AnimationCustom* newAnimation = new AnimationCustom();
    if(!loadAnimationFromXML(fileName, newAnimation)) {
        std::cerr << "AnimationAssimp load failed" << std::endl;
        delete newAnimation;
        return nullptr;
    }
    return newAnimation;
}

bool AnimationLoader::loadAnimationFromXML(const std::string &fileName, AnimationCustom *loadingAnimation) {

    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError eResult = xmlDoc.LoadFile(fileName.c_str());
    if (eResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "Error loading "<< fileName << " XML: " <<  xmlDoc.ErrorName() << std::endl;
        exit(-1);
    }

    tinyxml2::XMLNode * animationNode = xmlDoc.FirstChild();
    if (animationNode == nullptr) {
        std::cerr << "AnimationAssimp xml is not a valid XML." << std::endl;
        return false;
    }

    tinyxml2::XMLElement* animationName =  animationNode->FirstChildElement("Name");
    if (animationName == nullptr) {
        std::cerr << "AnimationAssimp must have a name." << std::endl;//TODO it actually doesn't
        return false;
    }
    std::cout << "read AnimationAssimp with name " << animationName->GetText() << std::endl;

    tinyxml2::XMLElement* animationDuration =  animationNode->FirstChildElement("Duration");
    if (animationDuration == nullptr) {
        std::cerr << "AnimationAssimp must have a duration." << std::endl;
        return false;
    }
    loadingAnimation->duration = std::stof(animationDuration->GetText());

    tinyxml2::XMLElement* animationTicksPerSecond =  animationNode->FirstChildElement("TicksPerSecond");
    if (animationTicksPerSecond == nullptr) {
        std::cerr << "AnimationAssimp must have a TicksPerSecond." << std::endl;
        return false;
    }
    loadingAnimation->ticksPerSecond = std::stof(animationTicksPerSecond->GetText());

    //load objects
    tinyxml2::XMLElement* nodesNode =  animationNode->FirstChildElement("Nodes");
    animationNode->InsertEndChild(nodesNode);

    return loadNodesFromXML(nodesNode, loadingAnimation);
}

bool AnimationLoader::loadNodesFromXML(tinyxml2::XMLNode *animationNode, AnimationCustom *loadingAnimation) {
    tinyxml2::XMLElement* nodeNode =  animationNode->FirstChildElement("Node");
    if (nodeNode == nullptr) {
        std::cerr << "AnimationAssimp must have at least one animation node." << std::endl;
        return false;
    }
    AnimationNode *animationForNode = new AnimationNode();

    tinyxml2::XMLElement* nodeAttribute;
    nodeAttribute =  nodeNode->FirstChildElement("Name");
    if (nodeAttribute == nullptr) {
        std::cerr << "Object must have a source file." << std::endl;
        return false;
    }
    loadingAnimation->animationNode = animationForNode;

    readTranslateAndTimes(nodeNode, animationForNode);
    readScaleAndTimes(nodeNode, animationForNode);
    readRotationAndTimes(nodeNode, animationForNode);

    return true;
}

bool AnimationLoader::readTranslateAndTimes(tinyxml2::XMLElement *nodeNode,
                                            AnimationNode *animationForNode) {
    tinyxml2::XMLElement *nodeAttribute;
    tinyxml2::XMLElement* nodeAttributeAttribute;
    tinyxml2::XMLElement* nodeAttributeAttributeValue;
    nodeAttribute =  nodeNode->FirstChildElement("Translates");
    if (nodeAttribute == nullptr) {
        std::cout << "AnimationNode does not have Translates. This is an error" << std::endl;
        return false;
    }
    //at this point we have translates at node attribute
    glm::vec3 element;
    nodeAttributeAttribute =  nodeAttribute->FirstChildElement("Translate");
    while(nodeAttributeAttribute != nullptr) {
        nodeAttributeAttributeValue = nodeAttributeAttribute->FirstChildElement("X");
        if(nodeAttributeAttributeValue != nullptr) {
            element.x = std::stof(nodeAttributeAttributeValue->GetText());
        } else {
            element.x = 0.0f;
        }
        nodeAttributeAttributeValue = nodeAttributeAttribute->FirstChildElement("Y");
        if(nodeAttributeAttributeValue != nullptr) {
            element.y = std::stof(nodeAttributeAttributeValue->GetText());
        } else {
            element.y = 0.0f;
        }
        nodeAttributeAttributeValue = nodeAttributeAttribute->FirstChildElement("Z");
        if(nodeAttributeAttributeValue != nullptr) {
            element.z = std::stof(nodeAttributeAttributeValue->GetText());
        } else {
            element.z = 0.0f;
        }

        nodeAttributeAttributeValue = nodeAttributeAttribute->FirstChildElement("Index");
        animationForNode->translates.insert(animationForNode->translates.begin()+
                                            std::stoi(nodeAttributeAttributeValue->GetText()), element);
        nodeAttributeAttribute =  nodeAttributeAttribute->NextSiblingElement("Translate");
    }
    /***** Translate times ******************************/
    nodeAttribute =  nodeNode->FirstChildElement("TranslateTimes");
    if (nodeAttribute == nullptr) {
            std::cout << "AnimationNode does not have TranslateTimes. This is an error" << std::endl;
            return false;
        }
    //at this point we have translates at node attribute
    nodeAttributeAttribute =  nodeAttribute->FirstChildElement("Time");
    float time;
    while(nodeAttributeAttribute != nullptr) {
        nodeAttributeAttributeValue = nodeAttributeAttribute->FirstChildElement("Value");
        if(nodeAttributeAttributeValue != nullptr) {
            time = std::stof(nodeAttributeAttributeValue->GetText());
        } else {
            std::cout << "AnimationNode translateTimes has Time but its value is not set. This is an error" << std::endl;
            std::exit(-1);
        }
        nodeAttributeAttributeValue = nodeAttributeAttribute->FirstChildElement("Index");
        animationForNode->translateTimes.insert(animationForNode->translateTimes.begin()+
                                            std::stoi(nodeAttributeAttributeValue->GetText()), time);
        nodeAttributeAttribute =  nodeAttributeAttribute->NextSiblingElement("Time");
    }
    /***** Translate times ******************************/
    return true;
}

bool AnimationLoader::readScaleAndTimes(tinyxml2::XMLElement *nodeNode, AnimationNode *animationForNode) {
    tinyxml2::XMLElement *nodeAttribute;
    tinyxml2::XMLElement* nodeAttributeAttribute;
    tinyxml2::XMLElement* nodeAttributeAttributeValue;
    nodeAttribute =  nodeNode->FirstChildElement("Scales");
    if (nodeAttribute == nullptr) {
        std::cout << "AnimationNode does not have Scales. This is an error" << std::endl;
        return false;
    }
    //at this point we have scales at node attribute
    glm::vec3 element;
    nodeAttributeAttribute =  nodeAttribute->FirstChildElement("Scale");
    while(nodeAttributeAttribute != nullptr) {
        nodeAttributeAttributeValue = nodeAttributeAttribute->FirstChildElement("X");
        if(nodeAttributeAttributeValue != nullptr) {
            element.x = std::stof(nodeAttributeAttributeValue->GetText());
        } else {
            element.x = 1.0f;
        }
        nodeAttributeAttributeValue = nodeAttributeAttribute->FirstChildElement("Y");
        if(nodeAttributeAttributeValue != nullptr) {
            element.y = std::stof(nodeAttributeAttributeValue->GetText());
        } else {
            element.y = 1.0f;
        }
        nodeAttributeAttributeValue = nodeAttributeAttribute->FirstChildElement("Z");
        if(nodeAttributeAttributeValue != nullptr) {
            element.z = std::stof(nodeAttributeAttributeValue->GetText());
        } else {
            element.z = 1.0f;
        }

        nodeAttributeAttributeValue = nodeAttributeAttribute->FirstChildElement("Index");
        animationForNode->scales.insert(animationForNode->scales.begin()+
                                            std::stoi(nodeAttributeAttributeValue->GetText()), element);
        nodeAttributeAttribute =  nodeAttributeAttribute->NextSiblingElement("Scale");
    }
    /***** scale times ******************************/
    nodeAttribute =  nodeNode->FirstChildElement("ScaleTimes");
    if (nodeAttribute == nullptr) {
        std::cout << "AnimationNode does not have ScaleTimes. This is an error" << std::endl;
        return false;
    }
    //at this point we have scales at node attribute
    nodeAttributeAttribute =  nodeAttribute->FirstChildElement("Time");
    float time;
    while(nodeAttributeAttribute != nullptr) {
        nodeAttributeAttributeValue = nodeAttributeAttribute->FirstChildElement("Value");
        if(nodeAttributeAttributeValue != nullptr) {
            time = std::stof(nodeAttributeAttributeValue->GetText());
        } else {
            std::cout << "AnimationNode scaleTimes has Time but its value is not set. This is an error" << std::endl;
            std::exit(-1);
        }
        nodeAttributeAttributeValue = nodeAttributeAttribute->FirstChildElement("Index");
        animationForNode->scaleTimes.insert(animationForNode->scaleTimes.begin()+
                                                std::stoi(nodeAttributeAttributeValue->GetText()), time);
        nodeAttributeAttribute =  nodeAttributeAttribute->NextSiblingElement("Time");
    }
    /***** scale times ******************************/
    return true;
}

bool
AnimationLoader::readRotationAndTimes(tinyxml2::XMLElement *nodeNode, AnimationNode *animationForNode) {
    tinyxml2::XMLElement *nodeAttribute;
    tinyxml2::XMLElement* nodeAttributeAttribute;
    tinyxml2::XMLElement* nodeAttributeAttributeValue;
    nodeAttribute =  nodeNode->FirstChildElement("Rotations");
    if (nodeAttribute == nullptr) {
        std::cout << "AnimationNode does not have Rotations. This is an error" << std::endl;
        return false;
    }
    //at this point we have rotations at node attribute
    glm::quat element;
    nodeAttributeAttribute =  nodeAttribute->FirstChildElement("Rotation");
    while(nodeAttributeAttribute != nullptr) {
        nodeAttributeAttributeValue = nodeAttributeAttribute->FirstChildElement("X");
        if(nodeAttributeAttributeValue != nullptr) {
            element.x = std::stof(nodeAttributeAttributeValue->GetText());
        } else {
            element.x = 0.0;
        }
        nodeAttributeAttributeValue = nodeAttributeAttribute->FirstChildElement("Y");
        if(nodeAttributeAttributeValue != nullptr) {
            element.y = std::stof(nodeAttributeAttributeValue->GetText());
        } else {
            element.y = 0.0;
        }
        nodeAttributeAttributeValue = nodeAttributeAttribute->FirstChildElement("Z");
        if(nodeAttributeAttributeValue != nullptr) {
            element.z = std::stof(nodeAttributeAttributeValue->GetText());
        } else {
            element.z = 0.0;
        }
        nodeAttributeAttributeValue = nodeAttributeAttribute->FirstChildElement("W");
        if(nodeAttributeAttributeValue != nullptr) {
            element.w = std::stof(nodeAttributeAttributeValue->GetText());
        } else {
            element.w = 1.0;
        }

        nodeAttributeAttributeValue = nodeAttributeAttribute->FirstChildElement("Index");
        animationForNode->rotations.insert(animationForNode->rotations.begin()+
                                            std::stoi(nodeAttributeAttributeValue->GetText()), element);
        nodeAttributeAttribute =  nodeAttributeAttribute->NextSiblingElement("Rotation");
    }
    /***** Rotation times ******************************/
    nodeAttribute =  nodeNode->FirstChildElement("RotationTimes");
    if (nodeAttribute == nullptr) {
        std::cout << "AnimationNode does not have RotationTimes. This is an error" << std::endl;
        return false;
    }
    //at this point we have Rotations at node attribute
    nodeAttributeAttribute =  nodeAttribute->FirstChildElement("Time");
    float time;
    while(nodeAttributeAttribute != nullptr) {
        nodeAttributeAttributeValue = nodeAttributeAttribute->FirstChildElement("Value");
        if(nodeAttributeAttributeValue != nullptr) {
            time = std::stof(nodeAttributeAttributeValue->GetText());
        } else {
            std::cout << "AnimationNode RotationTimes has Time but its value is not set. This is an error" << std::endl;
            std::exit(-1);
        }
        nodeAttributeAttributeValue = nodeAttributeAttribute->FirstChildElement("Index");
        animationForNode->rotationTimes.insert(animationForNode->rotationTimes.begin()+
                                                std::stoi(nodeAttributeAttributeValue->GetText()), time);
        nodeAttributeAttribute =  nodeAttributeAttribute->NextSiblingElement("Time");
    }
    /***** Translate times ******************************/
    return true;
}
