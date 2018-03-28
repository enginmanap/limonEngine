//
// Created by engin on 18.06.2016.
//

#ifndef LIMONENGINE_LIGHT_H
#define LIMONENGINE_LIGHT_H


#include "glm/glm.hpp"
#include "GameObject.h"
#include "../GLHelper.h"

class Light : public GameObject {
public:
    enum LightTypes {
        DIRECTIONAL, POINT
    };
private:
    GLHelper* glHelper;
    glm::mat4 shadowMatrices[6];//these are used only for point lights for now
    uint32_t objectID;
    glm::vec3 position, color;
    LightTypes lightType;

    void setShadowMatricesForPosition(){
        shadowMatrices[0] =glHelper->getLightProjectionMatrixPoint() *
                           glm::lookAt(position, position + glm::vec3( 1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0));
        shadowMatrices[1] =glHelper->getLightProjectionMatrixPoint() *
                           glm::lookAt(position, position + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0));
        shadowMatrices[2] =glHelper->getLightProjectionMatrixPoint() *
                           glm::lookAt(position, position + glm::vec3( 0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
        shadowMatrices[3] =glHelper->getLightProjectionMatrixPoint() *
                           glm::lookAt(position, position + glm::vec3( 0.0,-1.0, 0.0), glm::vec3(0.0, 0.0,-1.0));
        shadowMatrices[4] =glHelper->getLightProjectionMatrixPoint() *
                           glm::lookAt(position, position + glm::vec3( 0.0, 0.0, 1.0), glm::vec3(0.0,-1.0, 0.0));
        shadowMatrices[5] =glHelper->getLightProjectionMatrixPoint() *
                           glm::lookAt(position, position + glm::vec3( 0.0, 0.0,-1.0), glm::vec3(0.0,-1.0, 0.0));
    }
public:
    Light(GLHelper *glHelper, uint32_t objectID, LightTypes lightType, const glm::vec3 &position,
              const glm::vec3 &color) :
            glHelper(glHelper),
            objectID(objectID),
            position(position),
            lightType(lightType) {
        this->color.r = color.r < 1.0f ? color.r : 1.0f;
        this->color.g = color.g < 1.0f ? color.g : 1.0f;
        this->color.b = color.b < 1.0f ? color.b : 1.0f;

        setShadowMatricesForPosition();
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

     const glm::mat4 * getShadowMatrices() const {
        return shadowMatrices;
    }

    /************Game Object methods **************/

    uint32_t getWorldObjectID() {
        return objectID;
    };

    ObjectTypes getTypeID() const {
        return GameObject::LIGHT;
    };

    std::string getName() const {
        return "";//Lights doesn't have names
    };
    /************Game Object methods **************/
};


#endif //LIMONENGINE_LIGHT_H
