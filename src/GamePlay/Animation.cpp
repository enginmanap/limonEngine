//
// Created by engin on 12.05.2018.
//

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "Animation.h"
#include "AnimationNode.h"



//FIXME there must be a better way then is found.
//FIXME requiring node name should not be a thing
glm::mat4 Animation::calculateTransform(const std::string &nodeName, float time, bool &isFound) const {
    if (nodes.find(nodeName) == nodes.end()) {//if the bone has no animation, it can happen
        isFound = false;
        return glm::mat4(1.0f);
    }
    isFound = true;
    AnimationNode *nodeAnimation = nodes.at(nodeName);
    //this method can benefit from move and also reusing the intermediate matrices
    glm::vec3 scalingTransformVector, transformVector;
    glm::quat rotationTransformQuaternion;

    scalingTransformVector = nodeAnimation->getScalingVector(time);
    rotationTransformQuaternion = nodeAnimation->getRotationQuat(time);
    transformVector = nodeAnimation->getPositionVector(time);

    glm::mat4 rotationMatrix = glm::mat4_cast(rotationTransformQuaternion);
    glm::mat4 translateMatrix = glm::translate(glm::mat4(1.0f), transformVector);
    glm::mat4 scaleTransform = glm::scale(glm::mat4(1.0f), scalingTransformVector);
    return translateMatrix * rotationMatrix * scaleTransform;
}

Animation::Animation(aiAnimation *assimpAnimation) : customCreation(false) {
    duration = assimpAnimation->mDuration;
    ticksPerSecond = assimpAnimation->mTicksPerSecond;
    //create and attach AnimationNodes
    for (unsigned int j = 0; j < assimpAnimation->mNumChannels; ++j) {
        AnimationNode *node = new AnimationNode();
        for (unsigned int k = 0; k < assimpAnimation->mChannels[j]->mNumPositionKeys; ++k) {
            node->translates.push_back(glm::vec3(
                    assimpAnimation->mChannels[j]->mPositionKeys[k].mValue.x,
                    assimpAnimation->mChannels[j]->mPositionKeys[k].mValue.y,
                    assimpAnimation->mChannels[j]->mPositionKeys[k].mValue.z));
            node->translateTimes.push_back(assimpAnimation->mChannels[j]->mPositionKeys[k].mTime);
        }

        for (unsigned int k = 0; k < assimpAnimation->mChannels[j]->mNumScalingKeys; ++k) {
            node->scales.push_back(glm::vec3(
                    assimpAnimation->mChannels[j]->mScalingKeys[k].mValue.x,
                    assimpAnimation->mChannels[j]->mScalingKeys[k].mValue.y,
                    assimpAnimation->mChannels[j]->mScalingKeys[k].mValue.z));
            node->scaleTimes.push_back(assimpAnimation->mChannels[j]->mScalingKeys[k].mTime);
        }

        for (unsigned int k = 0; k < assimpAnimation->mChannels[j]->mNumRotationKeys; ++k) {
            node->rotations.push_back(glm::quat(
                    assimpAnimation->mChannels[j]->mRotationKeys[k].mValue.w,
                    assimpAnimation->mChannels[j]->mRotationKeys[k].mValue.x,
                    assimpAnimation->mChannels[j]->mRotationKeys[k].mValue.y,
                    assimpAnimation->mChannels[j]->mRotationKeys[k].mValue.z));
            node->rotationTimes.push_back(assimpAnimation->mChannels[j]->mRotationKeys[k].mTime);
        }
        nodes[assimpAnimation->mChannels[j]->mNodeName.C_Str()] = node;
    }

    //validate
}

/**
 * Saves the animation to a xml file with the name of first node of animation.
 * @param path must end with "/"
 * @return true if saved successfully, or not needs saving. False if fails to save
 */
bool Animation::serializeAnimation(const std::string &path) const {
    if (!this->customCreation) {
        return true; //don't try to serialize assimp animations, only custom ones
    }
    tinyxml2::XMLDocument animationDocument;
    tinyxml2::XMLNode *rootNode = animationDocument.NewElement("Animation");
    animationDocument.InsertFirstChild(rootNode);
    tinyxml2::XMLElement *currentElement = animationDocument.NewElement("Name");
    currentElement->SetText(this->nodes.begin()->first.c_str());
    rootNode->InsertEndChild(currentElement);
    //after current element is inserted, we can reuse
    currentElement = animationDocument.NewElement("Nodes");
    for (auto nodeIt = nodes.begin(); nodeIt != nodes.end(); nodeIt++) {
        //save node
        nodeIt->second->fillNode(animationDocument, currentElement, nodeIt->first);
    }
    rootNode->InsertEndChild(currentElement);//add nodes

    currentElement = animationDocument.NewElement("Duration");
    currentElement->SetText(std::to_string(this->duration).c_str());
    rootNode->InsertEndChild(currentElement);//add duration

    currentElement = animationDocument.NewElement("TicksPerSecond");
    currentElement->SetText(std::to_string(this->ticksPerSecond).c_str());
    rootNode->InsertEndChild(currentElement);//add ticks per second

    tinyxml2::XMLError eResult = animationDocument.SaveFile((path + this->nodes.begin()->first + ".xml").c_str());
    if (eResult != tinyxml2::XML_SUCCESS) {
        std::cout << "ERROR " << eResult << std::endl;
        return false;
    }

    return true;

}
