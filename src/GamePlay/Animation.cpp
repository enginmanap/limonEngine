//
// Created by engin on 12.05.2018.
//

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "Animation.h"


glm::vec3 Animation::AnimationForNode::getPositionVector(const float timeInTicks) const {
    glm::vec3 transformVector;
    if (translates.size() == 1) {
        transformVector = translates[0];
    } else {
        unsigned int positionIndex = 0;
        for (unsigned int i = 0; i < translates.size(); i++) {
            if (timeInTicks < translateTimes[i + 1]) {
                positionIndex = i;
                break;
            }
        }

        unsigned int NextPositionIndex = (positionIndex + 1);
        assert(NextPositionIndex < translates.size());
        float DeltaTime = (float) (translateTimes[NextPositionIndex] -
                                   translateTimes[positionIndex]);
        float Factor = (timeInTicks - (float) translateTimes[positionIndex]) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        const glm::vec3 &Start = translates[positionIndex];
        const glm::vec3 &End = translates[NextPositionIndex];
        glm::vec3 Delta = End - Start;
        transformVector = Start + Factor * Delta;
    }
    return transformVector;
}

glm::vec3 Animation::AnimationForNode::getScalingVector(const float timeInTicks) const {
    glm::vec3 scalingTransformVector;
    if (scales.size() == 1) {
        scalingTransformVector = scales[0];
    } else {
        unsigned int ScalingIndex = 0;
        assert(scales.size() > 0);
        for (unsigned int i = 0; i < scales.size(); i++) {
            if (timeInTicks < scaleTimes[i + 1]) {
                ScalingIndex = i;
                break;
            }
        }

        unsigned int NextScalingIndex = (ScalingIndex + 1);
        assert(NextScalingIndex < scales.size());
        float DeltaTime = (scaleTimes[NextScalingIndex] -
                           scaleTimes[ScalingIndex]);
        float Factor = (timeInTicks - (float) scaleTimes[ScalingIndex]) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        const glm::vec3 &Start = scales[ScalingIndex];
        const glm::vec3 &End = scales[NextScalingIndex];
        glm::vec3 Delta = End - Start;
        scalingTransformVector = Start + Factor * Delta;
    }
    return scalingTransformVector;
}

glm::quat Animation::AnimationForNode::getRotationQuat(const float timeInTicks) const {
    glm::quat rotationTransformQuaternion;
    if (rotations.size() == 1) {
        rotationTransformQuaternion = rotations[0];
    } else {

        int rotationIndex = 0;

        assert(rotations.size() > 0);

        for (unsigned int i = 0; i < rotations.size(); i++) {
            if (timeInTicks < (float) rotationTimes[i + 1]) {
                rotationIndex = i;
                break;
            }
        }

        unsigned int NextRotationIndex = (rotationIndex + 1);
        assert(NextRotationIndex < rotations.size());
        float DeltaTime = (rotationTimes[NextRotationIndex] -
                           rotationTimes[rotationIndex]);
        float Factor = (timeInTicks - (float) rotationTimes[rotationIndex]) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        const glm::quat &StartRotationQ = rotations[rotationIndex];
        const glm::quat &EndRotationQ = rotations[NextRotationIndex];
        rotationTransformQuaternion = glm::normalize(glm::slerp(StartRotationQ, EndRotationQ, Factor));
    }
    return rotationTransformQuaternion;
}

//FIXME there must be a better way then is found.
//FIXME requiring node name should not be a thing
glm::mat4 Animation::calculateTransform(const std::string &nodeName, float time, bool &isFound) const {
    if (nodes.find(nodeName) == nodes.end()) {//if the bone has no animation, it can happen
        isFound = false;
        return glm::mat4(1.0f);
    }
    isFound = true;
    AnimationForNode *nodeAnimation = nodes.at(nodeName);
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
        AnimationForNode *node = new AnimationForNode();
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

void Animation::AnimationForNode::fillNode(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *nodesNode,
                                           const std::string &nodeName) const {
    tinyxml2::XMLElement *nodeElement = document.NewElement("Node");
    nodesNode->InsertEndChild(nodeElement);

    tinyxml2::XMLElement *currentElement = document.NewElement("Name");
    currentElement->SetText(nodeName.c_str());
    nodeElement->InsertEndChild(currentElement);//add name

    fillTranslateAndTimes(document, nodeElement);
    fillScaleAndTimes(document, nodeElement);
    fillRotationAndTimes(document, nodeElement);
}

void Animation::AnimationForNode::fillTranslateAndTimes(tinyxml2::XMLDocument &document,
                                                        tinyxml2::XMLElement *nodeElement) const {
    tinyxml2::XMLElement *currentElement;//used for multiple elements

    tinyxml2::XMLElement *translatesNode = document.NewElement("Translates");
    for (size_t i = 0; i < translates.size(); i++) {
        tinyxml2::XMLElement *translateNode = document.NewElement("Translate");
        currentElement = document.NewElement("Index");
        currentElement->SetText(std::to_string(i).c_str());
        translateNode->InsertEndChild(currentElement);

        currentElement = document.NewElement("X");
        currentElement->SetText(translates[i].x);
        translateNode->InsertEndChild(currentElement);

        currentElement = document.NewElement("Y");
        currentElement->SetText(translates[i].y);
        translateNode->InsertEndChild(currentElement);

        currentElement = document.NewElement("Z");
        currentElement->SetText(translates[i].z);
        translateNode->InsertEndChild(currentElement);
        translatesNode->InsertEndChild(translateNode);
    }
    nodeElement->InsertEndChild(translatesNode);

    tinyxml2::XMLElement *translateTimesNode = document.NewElement("TranslateTimes");
    for (size_t i = 0; i < translateTimes.size(); i++) {
        tinyxml2::XMLElement *translateTimeNode = document.NewElement("Time");
        currentElement = document.NewElement("Index");
        currentElement->SetText(std::to_string(i).c_str());
        translateTimeNode->InsertEndChild(currentElement);

        currentElement = document.NewElement("Value");
        currentElement->SetText(std::to_string(translateTimes[i]).c_str());
        translateTimeNode->InsertEndChild(currentElement);

        translateTimesNode->InsertEndChild(translateTimeNode);
    }
    nodeElement->InsertEndChild(translateTimesNode);
}

void Animation::AnimationForNode::fillScaleAndTimes(tinyxml2::XMLDocument &document,
                                                    tinyxml2::XMLElement *nodeElement) const {
    tinyxml2::XMLElement *currentElement;//used for multiple elements

    tinyxml2::XMLElement *scalesNode = document.NewElement("Scales");
    for (size_t i = 0; i < scales.size(); i++) {
        tinyxml2::XMLElement *scaleNode = document.NewElement("Scale");
        currentElement = document.NewElement("Index");
        currentElement->SetText(std::to_string(i).c_str());
        scaleNode->InsertEndChild(currentElement);

        currentElement = document.NewElement("X");
        currentElement->SetText(scales[i].x);
        scaleNode->InsertEndChild(currentElement);

        currentElement = document.NewElement("Y");
        currentElement->SetText(scales[i].y);
        scaleNode->InsertEndChild(currentElement);

        currentElement = document.NewElement("Z");
        currentElement->SetText(scales[i].z);
        scaleNode->InsertEndChild(currentElement);
        scalesNode->InsertEndChild(scaleNode);
    }
    nodeElement->InsertEndChild(scalesNode);

    tinyxml2::XMLElement *scaleTimesNode = document.NewElement("ScaleTimes");
    for (size_t i = 0; i < scaleTimes.size(); i++) {
        tinyxml2::XMLElement *scaleTimeNode = document.NewElement("Time");
        currentElement = document.NewElement("Index");
        currentElement->SetText(std::to_string(i).c_str());
        scaleTimeNode->InsertEndChild(currentElement);

        currentElement = document.NewElement("Value");
        currentElement->SetText(std::to_string(scaleTimes[i]).c_str());
        scaleTimeNode->InsertEndChild(currentElement);

        scaleTimesNode->InsertEndChild(scaleTimeNode);
    }
    nodeElement->InsertEndChild(scaleTimesNode);
}

void Animation::AnimationForNode::fillRotationAndTimes(tinyxml2::XMLDocument &document,
                                                       tinyxml2::XMLElement *nodeElement) const {
    tinyxml2::XMLElement *currentElement;//used for multiple elements

    tinyxml2::XMLElement *rotationsNode = document.NewElement("Rotations");
    for (size_t i = 0; i < rotations.size(); i++) {
        tinyxml2::XMLElement *rotationNode = document.NewElement("Rotation");
        currentElement = document.NewElement("Index");
        currentElement->SetText(std::to_string(i).c_str());
        rotationNode->InsertEndChild(currentElement);

        currentElement = document.NewElement("X");
        currentElement->SetText(rotations[i].x);
        rotationNode->InsertEndChild(currentElement);

        currentElement = document.NewElement("Y");
        currentElement->SetText(rotations[i].y);
        rotationNode->InsertEndChild(currentElement);

        currentElement = document.NewElement("Z");
        currentElement->SetText(rotations[i].z);
        rotationNode->InsertEndChild(currentElement);

        currentElement = document.NewElement("W");
        currentElement->SetText(rotations[i].w);
        rotationNode->InsertEndChild(currentElement);

        rotationsNode->InsertEndChild(rotationNode);
    }
    nodeElement->InsertEndChild(rotationsNode);

    tinyxml2::XMLElement *rotationTimesNode = document.NewElement("RotationTimes");
    for (size_t i = 0; i < rotationTimes.size(); i++) {
        tinyxml2::XMLElement *rotationTimeNode = document.NewElement("Time");
        currentElement = document.NewElement("Index");
        currentElement->SetText(std::to_string(i).c_str());
        rotationTimeNode->InsertEndChild(currentElement);

        currentElement = document.NewElement("Value");
        currentElement->SetText(std::to_string(rotationTimes[i]).c_str());
        rotationTimeNode->InsertEndChild(currentElement);

        rotationTimesNode->InsertEndChild(rotationTimeNode);
    }
    nodeElement->InsertEndChild(rotationTimesNode);
}
