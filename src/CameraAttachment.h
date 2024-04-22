//
// Created by engin on 15.02.2018.
//

#ifndef LIMONENGINE_CAMERAATTACHMENT_H
#define LIMONENGINE_CAMERAATTACHMENT_H


#include <glm/vec3.hpp>

class CameraAttachment {
public:
    virtual bool isDirty() const = 0;

    virtual void clearDirty() = 0;
    virtual void getCameraVariables(glm::vec3 &position, glm::vec3 &center, glm::vec3 &up, glm::vec3 &right) = 0;

    virtual ~CameraAttachment() = default;
};


#endif //LIMONENGINE_CAMERAATTACHMENT_H
