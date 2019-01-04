//
// Created by engin on 11.09.2018.
//

#ifndef LIMONENGINE_ANIMATIONASSIMPSECTION_H
#define LIMONENGINE_ANIMATIONASSIMPSECTION_H


#include "AnimationAssimp.h"
#ifdef CEREAL_SUPPORT
#include <cereal/access.hpp>
#endif


class AnimationAssimpSection : public AnimationInterface {
    std::shared_ptr<AnimationInterface> baseAnimation;
    float startTime = 0;
    float endTime = 0;
#ifdef CEREAL_SUPPORT
    friend class cereal::access;
#endif

    AnimationAssimpSection() {}
public:
    AnimationAssimpSection(std::shared_ptr<AnimationInterface> base, float startTime, float endTime);

    bool calculateTransform(const std::string& nodeName, float time, Transformation& transformation) const;

    float getTicksPerSecond() const {
        return baseAnimation->getTicksPerSecond();
    }

    float getDuration() const {
        return endTime - startTime;
    }
#ifdef CEREAL_SUPPORT
    template<class Archive>
    void serialize( Archive & ar ) {
        ar(baseAnimation, startTime, endTime);
    }
#endif

};

#ifdef CEREAL_SUPPORT
#include <cereal/types/polymorphic.hpp>
CEREAL_REGISTER_TYPE(AnimationAssimpSection)
CEREAL_REGISTER_POLYMORPHIC_RELATION(AnimationInterface, AnimationAssimpSection)
#endif

#endif //LIMONENGINE_ANIMATIONASSIMPSECTION_H
