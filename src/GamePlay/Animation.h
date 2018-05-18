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


class Animation {
    friend class AnimationLoader;
public:
    struct AnimationForNode {
        std::vector<glm::vec3> translates;
        std::vector<float>translateTimes;
        std::vector<glm::vec3> scales;
        std::vector<float>scaleTimes;
        std::vector<glm::quat> rotations;
        std::vector<float>rotationTimes;

        void fillNode(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *nodeElement,
                              const std::string &nodeName) const;

        glm::quat getRotationQuat(const float timeInTicks) const;

        glm::vec3 getScalingVector(const float timeInTicks) const;

        glm::vec3 getPositionVector(const float timeInTicks) const;

    private:
        void fillTranslateAndTimes(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *nodeElement) const;

        void fillScaleAndTimes(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *nodeElement) const;

        void fillRotationAndTimes(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *nodeElement) const;


    };
private:
    float ticksPerSecond;
    float duration;
    //This map keeps the animations for node(bone)
    std::unordered_map<std::string, AnimationForNode*> nodes; //IF
    bool customCreation;

    /*this private constructor is meant for deserialize only*/
    Animation() = default;

public:
    Animation(aiAnimation *assimpAnimation);
    //For single Node animation creation
    Animation(std::string nodeName, AnimationForNode *animationNode, int duration) : ticksPerSecond(60), duration(duration), customCreation(true) {
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
