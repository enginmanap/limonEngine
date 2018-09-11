//
// Created by engin on 11.09.2018.
//

#ifndef LIMONENGINE_ANIMATIONINTERFACE_H
#define LIMONENGINE_ANIMATIONINTERFACE_H


#include "../../Transformation.h"
#include <glm/glm.hpp>

class AnimationInterface {
public:
    virtual glm::mat4 calculateTransform(const std::string& nodeName __attribute((unused)), float time __attribute((unused)), bool &isFound) const {
        isFound = false;
        return glm::mat4(1.0f);
    };

    virtual Transformation calculateTransform(float time __attribute((unused))) const {
        return Transformation();
    }

    virtual float getTicksPerSecond() const = 0;

    virtual float getDuration() const = 0;

    virtual ~AnimationInterface(){}
};


#endif //LIMONENGINE_ANIMATIONINTERFACE_H
