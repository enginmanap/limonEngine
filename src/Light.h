//
// Created by engin on 18.06.2016.
//

#ifndef UBERGAME_LIGHT_H
#define UBERGAME_LIGHT_H


#include "glm/glm.hpp"

class Light {
    enum LightTypes {
        DIRECTIONAL, POINT
    };

    glm::vec3 position, color;
    LightTypes lightType;

};


#endif //UBERGAME_LIGHT_H
