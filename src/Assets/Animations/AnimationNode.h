//
// Created by engin on 18.05.2018.
//

#ifndef LIMONENGINE_ANIMATIONNODE_H
#define LIMONENGINE_ANIMATIONNODE_H


#include <glm/vec3.hpp>
#include <vector>
#include <tinyxml2.h>
#include <glm/gtc/quaternion.hpp>
#include <string>

#include <cereal/types/vector.hpp>
#include "../../Utils/GLMCerealConverters.hpp"

//ATTENTION this is not a class, but a struct
struct AnimationNode {

        std::vector<glm::vec3> translates;
        std::vector<float>translateTimes;
        std::vector<glm::vec3> scales;
        std::vector<float>scaleTimes;
        std::vector<glm::quat> rotations;
        std::vector<float>rotationTimes;

        void fillNode(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *nodeElement) const;

        glm::quat getRotationQuat(const float timeInTicks) const;

        glm::vec3 getScalingVector(const float timeInTicks) const;

        glm::vec3 getPositionVector(const float timeInTicks) const;

    private:
        void fillTranslateAndTimes(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *nodeElement) const;

        void fillScaleAndTimes(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *nodeElement) const;

        void fillRotationAndTimes(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *nodeElement) const;

public:
    template<class Archive>
    void serialize( Archive & ar ) {
        ar(translates, translateTimes, scales, scaleTimes, rotations, rotationTimes);
    }

};


#endif //LIMONENGINE_ANIMATIONNODE_H
