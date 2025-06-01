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
        //starting info
        glm::vec3 defaultStart = position - (3.0f * center) + glm::vec3(0.0f, 2.0f, 0.0f);
        //we wanna check if we can see the player
        glm::vec3 direction = position - defaultStart;
        LimonTypes::Vec4 defaultStartL = {defaultStart.x, defaultStart.y, defaultStart.z, 0.0f};
        LimonTypes::Vec4 directionL = {direction.x, direction.y, direction.z, 0.0f};
        std::vector<LimonTypes::GenericParameter> hitDetails = limonAPI->rayCastFirstHit(defaultStartL, directionL);
        //std::cout << "raycast direction: " << defaultStart.x + direction.x << " " << defaultStart.y + direction.y << " " << defaultStart.z + direction.z << std::endl;
        if (hitDetails.empty()) {
            position = defaultStart;
            //std::cout << "raycast returned empty" << std::endl;
        } else {
            if (hitDetails.size() < 3) {
                //This API returns 4 elements, if it returns less it means something is wrong
                //std::cerr << "Raycast returned less than 3 elements" << std::endl;
            } else {
                if (hitDetails[0].valueType != LimonTypes::GenericParameter::LONG) {
                    //std::cerr << "Raycast returned wrong value type" << std::endl;
                } else {
                    if (hitDetails[0].value.longValue == 1) {
                        //std::cerr << "Raycast returned player, we don't care" << std::endl;
                    } else {
                        glm::vec3 hitPosition = glm::vec3(hitDetails[1].value.vectorValue.x, hitDetails[1].value.vectorValue.y, hitDetails[1].value.vectorValue.z);
                        if (glm::distance(hitPosition, defaultStart) < glm::distance(defaultStart, position)) {
                            position = hitPosition + 0.25f * glm::normalize(direction);
                            //std::cout << "raycast returned position, reseting" << std::endl;
                        } else {
                            //std::cout << "raycast returned wrong position, ignoring"<< std::endl;
                            position = defaultStart;
                        }
                    }
                }
            }
        }

    };

};



#endif //THIRDPERSONCAMERACAMERAATTACHMENT_H
