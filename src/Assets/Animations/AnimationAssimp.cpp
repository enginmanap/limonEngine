//
// Created by engin on 12.05.2018.
//

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "AnimationAssimp.h"
#include "AnimationNode.h"



//FIXME there must be a better way then is found.
//FIXME requiring node name should not be a thing
bool AnimationAssimp::calculateTransform(const std::string& nodeName, float time, Transformation& transformation) const {
    bool status = false;
    if (nodes.find(nodeName) == nodes.end()) { //if the bone has no animation, it can happen
        return status;
    }
    status = true;
    std::shared_ptr<AnimationNode> nodeAnimation = nodes.at(nodeName);

    transformation.setTransformations(nodeAnimation->getPositionVector(time),
    nodeAnimation->getScalingVector(time),
    nodeAnimation->getRotationQuat(time));
    return status;
}

AnimationAssimp::AnimationAssimp(aiAnimation *assimpAnimation) {
    duration = assimpAnimation->mDuration;
    ticksPerSecond = assimpAnimation->mTicksPerSecond;
    //create and attach AnimationNodes
    for (unsigned int j = 0; j < assimpAnimation->mNumChannels; ++j) {
        std::shared_ptr<AnimationNode> node = std::make_shared<AnimationNode>();
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
