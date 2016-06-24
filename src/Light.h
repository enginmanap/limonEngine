//
// Created by engin on 18.06.2016.
//

#ifndef UBERGAME_LIGHT_H
#define UBERGAME_LIGHT_H


#include "glm/glm.hpp"
#include "GLSLProgram.h"

class Light {
    enum LightTypes {
        DIRECTIONAL, POINT
    };

    glm::vec3 position, color;
    LightTypes lightType;

public:
    Light(const glm::vec3 &position, const glm::vec3 &color) : position(position), color(color) { }

    bool setLightToProgram(GLSLProgram *program) {
        if (!program->setUniform("light.position", position))
            std::cerr << "light position could not be set " << std::endl;
        if (!program->setUniform("light.color", color))
            std::cerr << "light color could not be set " << std::endl;
    }
};


#endif //UBERGAME_LIGHT_H
