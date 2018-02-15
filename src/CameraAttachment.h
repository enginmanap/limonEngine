//
// Created by engin on 15.02.2018.
//

#ifndef LIMONENGINE_CAMERAATTACHEMENT_H
#define LIMONENGINE_CAMERAATTACHEMENT_H


#include <glm/vec3.hpp>

class CameraAttachment {
public:
    virtual bool isDirty() = 0;
    virtual void getCameraVariables(glm::vec3 &position, glm::vec3 &center, glm::vec3& up, glm::vec3 right) = 0;

    virtual ~CameraAttachment() {};
};


#endif //LIMONENGINE_CAMERAATTACHEMENT_H
