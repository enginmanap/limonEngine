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
#include "AnimationInterface.h"
#include <memory>
#include "AnimationNode.h"

class AnimationAssimp : public AnimationInterface {
    float ticksPerSecond;
    float duration;
    //This map keeps the animations for node(bone)
    std::unordered_map<std::string, std::shared_ptr<AnimationNode>> nodes;
public:
    AnimationAssimp(aiAnimation *assimpAnimation);

    bool calculateTransform(const std::string& nodeName, float time, Transformation& transformation) const;

    float getTicksPerSecond() const {
        return ticksPerSecond;
    }

    float getDuration() const {
        return duration;
    }
};


#endif //LIMONENGINE_ANIMATIONASSIMP_H
