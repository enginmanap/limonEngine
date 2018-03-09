//
// Created by engin on 18.06.2016.
//

#ifndef LIMONENGINE_LIGHT_H
#define LIMONENGINE_LIGHT_H


#include "glm/glm.hpp"
#include "GameObject.h"

class Light : public GameObject {
public:
    enum LightTypes {
        DIRECTIONAL, POINT
    };
private:

    glm::vec3 position, color;
    LightTypes lightType;

public:
    Light(LightTypes lightType, const glm::vec3 &position, const glm::vec3 &color) : position(position),
                                                                                     lightType(lightType) {
        this->color.r = color.r < 1.0f ? color.r : 1.0f;
        this->color.g = color.g < 1.0f ? color.g : 1.0f;
        this->color.b = color.b < 1.0f ? color.b : 1.0f;
    }

    const glm::vec3 &getPosition() const {
        return position;
    }

    const glm::vec3 &getColor() const {
        return color;
    }

    LightTypes getLightType() const {
        return lightType;
    }

    /************Game Object methods **************/
    ObjectTypes getTypeID() const {
        return GameObject::LIGHT;
    };

    std::string getName() const {
        return "";//Lights doesn't have names
    };
    /************Game Object methods **************/
};


#endif //LIMONENGINE_LIGHT_H
