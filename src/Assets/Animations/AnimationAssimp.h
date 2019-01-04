//
// Created by engin on 12.05.2018.
//

#ifndef LIMONENGINE_ANIMATIONASSIMP_H
#define LIMONENGINE_ANIMATIONASSIMP_H


#include <assimp/anim.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <tinyxml2.h>
#include <memory>
#ifdef CEREAL_SUPPORT
#include <cereal/types/unordered_map.hpp>
#include <cereal/access.hpp>
#endif

#include "AnimationInterface.h"
#include "AnimationNode.h"

class AnimationAssimp : public AnimationInterface {
    float ticksPerSecond;
    float duration;
    //This map keeps the animations for node(bone)
    std::unordered_map<std::string, std::shared_ptr<AnimationNode>> nodes;
#ifdef CEREAL_SUPPORT
    friend class cereal::access;
#endif

    AnimationAssimp(){}
public:
    AnimationAssimp(aiAnimation *assimpAnimation);

    bool calculateTransform(const std::string& nodeName, float time, Transformation& transformation) const;

    float getTicksPerSecond() const {
        return ticksPerSecond;
    }

    float getDuration() const {
        return duration;
    }
#ifdef CEREAL_SUPPORT
    template<class Archive>
    void serialize( Archive & ar ) {
        ar(ticksPerSecond, duration, nodes);
    }
#endif
};

#ifdef CEREAL_SUPPORT
#include <cereal/types/polymorphic.hpp>
CEREAL_REGISTER_TYPE(AnimationAssimp)
CEREAL_REGISTER_POLYMORPHIC_RELATION(AnimationInterface, AnimationAssimp)
#endif

#endif //LIMONENGINE_ANIMATIONASSIMP_H
