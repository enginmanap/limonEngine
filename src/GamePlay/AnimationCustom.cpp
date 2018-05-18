//
// Created by engin on 18.05.2018.
//

#include "AnimationCustom.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "AnimationNode.h"



//FIXME there must be a better way then is found.
glm::mat4 AnimationCustom::calculateTransform(float time, bool &isFound) const {

    isFound = true;
    //this method can benefit from move and also reusing the intermediate matrices
    glm::vec3 scalingTransformVector, transformVector;
    glm::quat rotationTransformQuaternion;

    scalingTransformVector = animationNode->getScalingVector(time);
    rotationTransformQuaternion = animationNode->getRotationQuat(time);
    transformVector = animationNode->getPositionVector(time);

    glm::mat4 rotationMatrix = glm::mat4_cast(rotationTransformQuaternion);
    glm::mat4 translateMatrix = glm::translate(glm::mat4(1.0f), transformVector);
    glm::mat4 scaleTransform = glm::scale(glm::mat4(1.0f), scalingTransformVector);
    return translateMatrix * rotationMatrix * scaleTransform;
}

/**
 * Saves the animation to a xml file with the name of first node of animation.
 * @param path must end with "/"
 * @return true if saved successfully, or not needs saving. False if fails to save
 */
bool AnimationCustom::serializeAnimation(const std::string &path) const {
    tinyxml2::XMLDocument animationDocument;
    tinyxml2::XMLNode *rootNode = animationDocument.NewElement("AnimationAssimp");
    animationDocument.InsertFirstChild(rootNode);
    tinyxml2::XMLElement *currentElement = animationDocument.NewElement("Name");
    currentElement->SetText("root");
    rootNode->InsertEndChild(currentElement);
    //after current element is inserted, we can reuse
    currentElement = animationDocument.NewElement("Nodes");
    //save node
    animationNode->fillNode(animationDocument, currentElement, "root");

    rootNode->InsertEndChild(currentElement);//add nodes

    currentElement = animationDocument.NewElement("Duration");
    currentElement->SetText(std::to_string(this->duration).c_str());
    rootNode->InsertEndChild(currentElement);//add duration

    currentElement = animationDocument.NewElement("TicksPerSecond");
    currentElement->SetText(std::to_string(this->ticksPerSecond).c_str());
    rootNode->InsertEndChild(currentElement);//add ticks per second

    tinyxml2::XMLError eResult = animationDocument.SaveFile((path + "root" + ".xml").c_str());
    if (eResult != tinyxml2::XML_SUCCESS) {
        std::cout << "ERROR " << eResult << std::endl;
        return false;
    }

    return true;

}
