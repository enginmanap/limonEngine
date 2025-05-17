//
// Created by engin on 06/04/2025.
//

#ifndef THIRDPERSONCAMERACAMERAATTACHMENT_H
#define THIRDPERSONCAMERACAMERAATTACHMENT_H

#include "limonAPI/CameraAttachment.h"
#include "limonAPI/LimonAPI.h"

class ThirdPersonCameraAttachment : public CameraAttachment {
  LimonAPI *limonAPI;
public:

    ThirdPersonCameraAttachment(LimonAPI *limonAPI) : limonAPI(limonAPI) {}

    bool isDirty() const override {
        return true;
    }

    void clearDirty() override {
        //do nothing
    }
    void getCameraVariables(glm::vec3 &position, glm::vec3 &center, glm::vec3 &up, glm::vec3 &right) override {
        limonAPI->getPlayerPosition(position, center, up, right);
        //now move the camera a bit back
        position -= 3.0f * center;
        position.y += 2.0f;//for putting the camera up portion of capsule

    };

};



#endif //THIRDPERSONCAMERACAMERAATTACHMENT_H
