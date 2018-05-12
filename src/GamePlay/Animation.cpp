//
// Created by engin on 12.05.2018.
//

#include "Animation.h"


glm::vec3 Animation::getPositionVector(const float timeInTicks, const AnimationForNode *nodeAnimation) const {
    glm::vec3 transformVector;
    if (nodeAnimation->translates.size() == 1) {
        transformVector = nodeAnimation->translates[0];
    } else {
        unsigned int positionIndex = 0;
        for (unsigned int i = 0; i < nodeAnimation->translates.size(); i++) {
            if (timeInTicks < nodeAnimation->translateTimes[i + 1]) {
                positionIndex = i;
                break;
            }
        }

        unsigned int NextPositionIndex = (positionIndex + 1);
        assert(NextPositionIndex < nodeAnimation->translates.size());
        float DeltaTime = (float) (nodeAnimation->translateTimes[NextPositionIndex] -
                                   nodeAnimation->translateTimes[positionIndex]);
        float Factor = (timeInTicks - (float) nodeAnimation->translateTimes[positionIndex]) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        const glm::vec3 &Start = nodeAnimation->translates[positionIndex];
        const glm::vec3 &End = nodeAnimation->translates[NextPositionIndex];
        glm::vec3 Delta = End - Start;
        transformVector = Start + Factor * Delta;
    }
    return transformVector;
}

glm::vec3 Animation::getScalingVector(const float timeInTicks, const AnimationForNode *nodeAnimation) const {
    glm::vec3 scalingTransformVector;
    if (nodeAnimation->scales.size() == 1) {
        scalingTransformVector = nodeAnimation->scales[0];
    } else {
        unsigned int ScalingIndex = 0;
        assert(nodeAnimation->scales.size() > 0);
        for (unsigned int i = 0; i < nodeAnimation->scales.size(); i++) {
            if (timeInTicks < nodeAnimation->scaleTimes[i + 1]) {
                ScalingIndex = i;
                break;
            }
        }


        unsigned int NextScalingIndex = (ScalingIndex + 1);
        assert(NextScalingIndex < nodeAnimation->scales.size());
        float DeltaTime = (nodeAnimation->scaleTimes[NextScalingIndex] -
                           nodeAnimation->scaleTimes[ScalingIndex]);
        float Factor = (timeInTicks - (float) nodeAnimation->scaleTimes[ScalingIndex]) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        const glm::vec3 &Start = nodeAnimation->scales[ScalingIndex];
        const glm::vec3 &End = nodeAnimation->scales[NextScalingIndex];
        glm::vec3 Delta = End - Start;
        scalingTransformVector = Start + Factor * Delta;
    }
    return scalingTransformVector;
}

glm::quat Animation::getRotationQuat(const float timeInTicks, const AnimationForNode *nodeAnimation) const {
    glm::quat rotationTransformQuaternion;
    if (nodeAnimation->rotations.size() == 1) {
        rotationTransformQuaternion = nodeAnimation->rotations[0];
    } else {

        int rotationIndex = 0;

        assert(nodeAnimation->rotations.size() > 0);

        for (unsigned int i = 0; i < nodeAnimation->rotations.size(); i++) {
            if (timeInTicks < (float) nodeAnimation->rotationTimes[i + 1]) {
                rotationIndex = i;
                break;
            }
        }

        unsigned int NextRotationIndex = (rotationIndex + 1);
        assert(NextRotationIndex < nodeAnimation->rotations.size());
        float DeltaTime = (nodeAnimation->rotationTimes[NextRotationIndex] -
                           nodeAnimation->rotationTimes[rotationIndex]);
        float Factor = (timeInTicks - (float) nodeAnimation->rotationTimes[rotationIndex]) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        const glm::quat &StartRotationQ = nodeAnimation->rotations[rotationIndex];
        const glm::quat &EndRotationQ = nodeAnimation->rotations[NextRotationIndex];
        rotationTransformQuaternion = glm::normalize(glm::slerp(StartRotationQ, EndRotationQ, Factor));
    }
    return rotationTransformQuaternion;
}

//FIXME there must be a better way then is found
glm::mat4 Animation::calculateTransform(const std::string& nodeName, float time, bool &isFound) const {
    if (nodes.find(nodeName) == nodes.end()) {//if the bone has no animation, it can happen
        isFound = false;
        return glm::mat4(1.0f);
    }
    isFound = true;
    AnimationForNode *nodeAnimation = nodes.at(nodeName);
    //this method can benefit from move and also reusing the intermediate matrices
    glm::vec3 scalingTransformVector, transformVector;
    glm::quat rotationTransformQuaternion;

    scalingTransformVector = getScalingVector(time, nodeAnimation);
    rotationTransformQuaternion = getRotationQuat(time, nodeAnimation);
    transformVector = getPositionVector(time, nodeAnimation);

    glm::mat4 rotationMatrix = glm::mat4_cast(rotationTransformQuaternion);
    glm::mat4 translateMatrix = glm::translate(glm::mat4(1.0f), transformVector);
    glm::mat4 scaleTransform = glm::scale(glm::mat4(1.0f), scalingTransformVector);
    return translateMatrix * rotationMatrix * scaleTransform;
}

Animation::Animation(aiAnimation *assimpAnimation) {
    duration = assimpAnimation->mDuration;
    ticksPerSecond = assimpAnimation->mTicksPerSecond;
    //create and attach AnimationNodes
    for (unsigned int j = 0; j < assimpAnimation->mNumChannels; ++j) {
        AnimationForNode* node = new AnimationForNode();
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