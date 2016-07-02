//
// Created by engin on 18.06.2016.
//

#ifndef UBERGAME_LIGHT_H
#define UBERGAME_LIGHT_H


#include "glm/glm.hpp"

class Light {
public:
    enum LightTypes {
        DIRECTIONAL, POINT
    };
private:

    glm::vec3 position, color;
    LightTypes lightType;

public:
    Light(LightTypes lightType, const glm::vec3 &position, const glm::vec3 &color) : position(position), color(color),
                                                                                     lightType(lightType) { }


    const glm::vec3 &getPosition() const {
        return position;
    }

    const glm::vec3 &getColor() const {
        return color;
    }
};


#endif //UBERGAME_LIGHT_H
