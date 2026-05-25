//
// Created by engin on 11.09.2018.
//

#ifndef LIMONENGINE_ANIMATIONINTERFACE_H
#define LIMONENGINE_ANIMATIONINTERFACE_H


#include "../../Transformation.h"
#include <glm/glm.hpp>
#ifdef CEREAL_SUPPORT
#include <cereal/archives/binary.hpp>
#include <cereal/archives/xml.hpp>
#endif

class AnimationInterface {
public:
    virtual bool calculateTransform(const std::string& nodeName, float time, Transformation& transformation) const = 0;

    virtual float getTicksPerSecond() const = 0;

    virtual float getDuration() const = 0;

    virtual ~AnimationInterface(){}

    AnimationInterface() = default;
    AnimationInterface(const AnimationInterface&) = default;
    AnimationInterface& operator=(const AnimationInterface&) = default;
    AnimationInterface(AnimationInterface&&) noexcept = default;
    AnimationInterface& operator=(AnimationInterface&&) noexcept = default;
};


#endif //LIMONENGINE_ANIMATIONINTERFACE_H
