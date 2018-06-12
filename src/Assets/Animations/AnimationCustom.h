//
// Created by engin on 18.05.2018.
//

#ifndef LIMONENGINE_ANIMATIONCUSTOM_H
#define LIMONENGINE_ANIMATIONCUSTOM_H


#include "AnimationNode.h"
#include "../../Transformation.h"

class AnimationCustom {
    friend class AnimationLoader;
    friend struct AnimationSequenceInterface;

    float ticksPerSecond;
    float duration;

    AnimationNode* animationNode;
    std::string name;

    /*this private constructor is meant for deserialize only*/
    AnimationCustom() = default;

public:
    AnimationCustom(const std::string &animationName, AnimationNode *animationNode, int duration)
            : ticksPerSecond(60), duration(duration), name(animationName) {
            this->animationNode = animationNode;
    }

    ~AnimationCustom() {
        delete animationNode;
    }

    AnimationCustom(const AnimationCustom &otherAnimation) {
        this->ticksPerSecond = otherAnimation.ticksPerSecond;
        this->duration = otherAnimation.duration;
        this->name = otherAnimation.name;
        this->animationNode = new AnimationNode(*(otherAnimation.animationNode));//default copy constructor used
    }

    Transformation calculateTransform(float time) const;

    float getTicksPerSecond() const {
        return ticksPerSecond;
    }

    float getDuration() const {
        return duration;
    }

    std::string getName() const {
        return name;
    }

    bool serializeAnimation(const std::string &path) const;
};


#endif //LIMONENGINE_ANIMATIONCUSTOM_H
