//
// Created by engin on 18.05.2018.
//

#include "AnimationNode.h"

glm::vec3 AnimationNode::getPositionVector(const float timeInTicks) const {
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

glm::vec3 AnimationNode::getScalingVector(const float timeInTicks) const {
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

glm::quat AnimationNode::getRotationQuat(const float timeInTicks) const {
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


void AnimationNode::fillNode(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *nodesNode,
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

void AnimationNode::fillTranslateAndTimes(tinyxml2::XMLDocument &document,
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

void AnimationNode::fillScaleAndTimes(tinyxml2::XMLDocument &document,
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

void AnimationNode::fillRotationAndTimes(tinyxml2::XMLDocument &document,
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
