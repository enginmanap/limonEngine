//
// Created by engin on 12.05.2018.
//

#ifndef LIMONENGINE_ANIMATION_H
#define LIMONENGINE_ANIMATION_H


class Animation {
    struct AnimationForNode {
        std::vector<glm::vec3> translates;
        std::vector<float>translateTimes;
        std::vector<glm::vec3> scales;
        std::vector<float>scaleTimes;
        std::vector<glm::quat> rotations;
        std::vector<float>rotationTimes;
    };

    float ticksPerSecond;
    float duration;
    //This map keeps the animations for node(bone)
    std::unordered_map<std::string, AnimationForNode*> nodes;//FIXME these should be removed

    glm::quat getRotationQuat(const float timeInTicks, const AnimationForNode *nodeAnimation) const;

    glm::vec3 getScalingVector(const float timeInTicks, const AnimationForNode *nodeAnimation) const;

    glm::vec3 getPositionVector(const float timeInTicks, const AnimationForNode *nodeAnimation) const;

public:
    Animation(aiAnimation *assimpAnimation);

    glm::mat4 calculateTransform(const std::string& nodeName, float time, bool &isFound) const;

    float getTicksPerSecond() const {
        return ticksPerSecond;
    }

    float getDuration() const {
        return duration;
    }

};


#endif //LIMONENGINE_ANIMATION_H
