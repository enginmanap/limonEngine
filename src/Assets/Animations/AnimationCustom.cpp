//
// Created by engin on 18.05.2018.
//

#include "AnimationCustom.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "AnimationNode.h"

bool AnimationCustom::calculateTransform(const std::string& nodeName __attribute((unused)), float time, Transformation& transformation) const {
    transformation.setScale(animationNode->getScalingVector(time));
    transformation.setOrientation(animationNode->getRotationQuat(time));
    transformation.setTranslate(animationNode->getPositionVector(time));
    return true;
}

/**
 * Saves the animation to a xml file with the name of first node of animation.
 * @param path must end with "/"
 * @return true if saved successfully, or not needs saving. False if fails to save
 */
bool AnimationCustom::serializeAnimation(const std::string &path) const {
    tinyxml2::XMLDocument animationDocument;
    tinyxml2::XMLNode *rootNode = animationDocument.NewElement("Animation");
    animationDocument.InsertFirstChild(rootNode);
    tinyxml2::XMLElement *currentElement = animationDocument.NewElement("Name");
    currentElement->SetText(this->name.c_str());
    rootNode->InsertEndChild(currentElement);
    //after current element is inserted, we can reuse
    currentElement = animationDocument.NewElement("Nodes");
    //save node
    animationNode->fillNode(animationDocument, currentElement);

    rootNode->InsertEndChild(currentElement);//add nodes

    currentElement = animationDocument.NewElement("Duration");
    currentElement->SetText(std::to_string(this->duration).c_str());
    rootNode->InsertEndChild(currentElement);//add duration

    currentElement = animationDocument.NewElement("TicksPerSecond");
    currentElement->SetText(std::to_string(this->ticksPerSecond).c_str());
    rootNode->InsertEndChild(currentElement);//add ticks per second

    tinyxml2::XMLError eResult = animationDocument.SaveFile((path + this->name + ".xml").c_str());
    if (eResult != tinyxml2::XML_SUCCESS) {
        std::cout << "ERROR " << eResult << std::endl;
        return false;
    }

    return true;

}
