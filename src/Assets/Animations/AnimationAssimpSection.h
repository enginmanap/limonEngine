//
// Created by engin on 11.09.2018.
//

#ifndef LIMONENGINE_ANIMATIONASSIMPSECTION_H
#define LIMONENGINE_ANIMATIONASSIMPSECTION_H


#include "AnimationAssimp.h"

class AnimationAssimpSection : public AnimationInterface {
    std::shared_ptr<AnimationInterface> baseAnimation;
    float startTime = 0;
    float endTime = 0;

public:
    AnimationAssimpSection(std::shared_ptr<AnimationInterface> base, float startTime, float endTime);

    bool calculateTransform(const std::string& nodeName, float time, Transformation& transformation) const;

    float getTicksPerSecond() const {
        return baseAnimation->getTicksPerSecond();
    }

    float getDuration() const {
        return endTime - startTime;
    }
};
#endif //LIMONENGINE_ANIMATIONASSIMPSECTION_H
