//
// Created by engin on 12.05.2018.
//

#ifndef LIMONENGINE_ANIMATION_H
#define LIMONENGINE_ANIMATION_H


#include <assimp/anim.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <tinyxml2.h>

class AnimationNode;

class Animation {
    friend class AnimationLoader;
public:

private:
    float ticksPerSecond;
    float duration;
    //This map keeps the animations for node(bone)
    std::unordered_map<std::string, AnimationNode*> nodes; //IF
    bool customCreation;

    /*this private constructor is meant for deserialize only*/
    Animation() = default;

public:
    Animation(aiAnimation *assimpAnimation);
    //For single Node animation creation
    Animation(std::string nodeName, AnimationNode *animationNode, int duration) : ticksPerSecond(60), duration(duration), customCreation(true) {
        this->nodes[nodeName] = animationNode;
    }

    glm::mat4 calculateTransform(const std::string& nodeName, float time, bool &isFound) const;

    float getTicksPerSecond() const {
        return ticksPerSecond;
    }

    float getDuration() const {
        return duration;
    }

    bool serializeAnimation(const std::string &path) const;
};


#endif //LIMONENGINE_ANIMATION_H
