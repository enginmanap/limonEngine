//
// Created by engin on 11.09.2018.
//

#ifndef LIMONENGINE_ANIMATIONASSIMPSECTION_H
#define LIMONENGINE_ANIMATIONASSIMPSECTION_H


#include "AnimationAssimp.h"

class AnimationAssimpSection : public AnimationInterface {
    AnimationInterface *baseAnimation;
    float startTime = 0;
    float endTime = 0;

public:
    AnimationAssimpSection(AnimationInterface *base, float startTime, float endTime);

    glm::mat4 calculateTransform(const std::string &nodeName, float time, bool &isFound) const;

    float getTicksPerSecond() const {
        return baseAnimation->getTicksPerSecond();
    }

    float getDuration() const {
        return endTime - startTime;
    }
};
#endif //LIMONENGINE_ANIMATIONASSIMPSECTION_H
