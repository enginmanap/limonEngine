//
// Created by engin on 2026.
//

#ifndef LIMONENGINE_THIRDPERSONCAMERARIG_H
#define LIMONENGINE_THIRDPERSONCAMERARIG_H

#include "limonAPI/CameraExtensionInterface.h"
#include "limonAPI/LimonAPI.h"
#include <glm/glm.hpp>

/**
 * A registered third-person camera rig: follows the player from behind/above with collision-aware
 * pullback (it raycasts toward the desired camera position and pulls in if a wall is in the way).
 * Perspective projection.
 *
 * This is the new-style CameraExtensionInterface (a camera rig), the replacement for the old
 * ThirdPersonCameraAttachment. It is UNATTACHED - it produces its own pose by querying the player position
 * through the API each frame. Activate it from gameplay with
 * LimonAPI::createCameraRig("ThirdPersonCameraRig") followed by activateCameraRig(id), or add/activate it in
 * the editor's Cameras tree.
 */
class ThirdPersonCameraRig : public CameraExtensionInterface {
    bool dirty = true;
public:
    explicit ThirdPersonCameraRig(LimonAPI* limonAPI) : CameraExtensionInterface(limonAPI) {}

    std::string getName() const override { return "ThirdPersonCameraRig"; }

    // Follows the player every frame, so it is always dirty.
    bool isDirty() const override { return dirty; }
    void clearDirty() override { /* intentionally stays dirty */ }

    CameraAttachment::ProjectionParameters getProjection() const override {
        return CameraAttachment::ProjectionParameters{}; // perspective
    }

    void getCameraVariables(glm::vec3 &position, glm::vec3 &center, glm::vec3 &up, glm::vec3 &right) override {
        limonAPI->getPlayerPosition(position, center, up, right);
        //starting info: behind and slightly above the player, along the view direction
        glm::vec3 defaultStart = position - (3.0f * center) + glm::vec3(0.0f, 2.0f, 0.0f);
        //we wanna check if we can see the player
        glm::vec3 direction = defaultStart - position;
        LimonTypes::Vec4 positionL = {position.x, position.y, position.z, 0.0f};
        LimonTypes::Vec4 directionL = {direction.x, direction.y, direction.z, 0.0f};
        std::vector<LimonTypes::GenericParameter> hitDetails = limonAPI->rayCastFirstHit(positionL, directionL);
        if (hitDetails.empty()) {
            position = defaultStart;
        } else {
            if (hitDetails.size() < 3) {
                //This API returns 4 elements, if it returns less it means something is wrong
            } else {
                if (hitDetails[0].valueType != LimonTypes::GenericParameter::LONG) {
                    //wrong value type
                } else {
                    if (hitDetails[0].value.longValue == 1) {
                        //raycast returned the player, we don't care
                    } else {
                        glm::vec3 hitPosition = glm::vec3(hitDetails[1].value.vectorValue.x, hitDetails[1].value.vectorValue.y, hitDetails[1].value.vectorValue.z);
                        if (glm::distance(hitPosition, defaultStart) < glm::distance(defaultStart, position)) {
                            position = hitPosition - 0.25f * glm::normalize(direction);
                        } else {
                            position = defaultStart;
                        }
                    }
                }
            }
        }
    }
};

#endif //LIMONENGINE_THIRDPERSONCAMERARIG_H
