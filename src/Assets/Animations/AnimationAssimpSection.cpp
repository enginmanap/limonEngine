//
// Created by engin on 11.09.2018.
//

#include <memory>
#include "AnimationAssimpSection.h"

AnimationAssimpSection::AnimationAssimpSection(std::shared_ptr<AnimationInterface> base, float startTime, float endTime) {
    this->baseAnimation = base;
    this->startTime = startTime;
    this->endTime = endTime;
}

bool AnimationAssimpSection::calculateTransform(const std::string& nodeName, float time, Transformation& transformation) const {
    time = startTime + time;
    return baseAnimation->calculateTransform(nodeName, time, transformation);
}
