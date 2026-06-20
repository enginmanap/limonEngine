//
// Created by engin on 2026.
//

#ifndef LIMONENGINE_ORTHOGRAPHICCAMERARIG_H
#define LIMONENGINE_ORTHOGRAPHICCAMERARIG_H

#include "limonAPI/CameraExtensionInterface.h"
#include "limonAPI/LimonAPI.h"
#include <glm/glm.hpp>

/**
 * A registered orthographic camera rig: an isometric / top-down view that follows the player.
 *
 * This is the new-style CameraExtensionInterface (a CameraRig behaviour), so it is added like any other
 * camera rig: in the editor via "Add Camera Rig" -> select it under "Cameras" -> Activate; or in the world
 * XML as a <CameraRig><Type>OrthographicCameraRig</Type>...</CameraRig>.
 *
 * It is UNATTACHED (it has no CameraRig parent); it produces its own pose by querying the player position
 * through the API every frame. The engine selects an orthographic player camera purely from
 * getProjection().type, so the only thing that makes this camera orthographic is returning ORTHOGRAPHIC.
 *
 * Parameters (all editable in the editor as drag fields, and round-tripped in the world XML):
 *   params[0..2] : camera offset X/Y/Z relative to the player, world space. The default places the camera
 *                  above and behind the player (isometric). For a straight top-down view use a mostly
 *                  vertical offset (e.g. 0, 30, 0.01) but keep a tiny horizontal component so the look-at
 *                  is never exactly parallel to world-up.
 *   params[3]    : orthographicHalfHeight (THE ZOOM) — half the vertical span of the view volume in world
 *                  units; the horizontal span follows the screen aspect. Smaller = more zoomed in.
 *   params[4..5] : near / far planes. nearPlane MUST be <= -farPlane/49 so that the depth range
 *                  [0, 0.02) is reserved for GUI elements and scene geometry starts above that.
 *                  Values that violate this are clamped on load. Orthographic has uniform depth
 *                  precision, so the ~2% reservation costs negligible precision.
 */
class OrthographicCameraRig : public CameraExtensionInterface {
    bool dirty = true;
public:
    glm::vec3 offset                 = glm::vec3(0.0f, 20.0f, 20.0f);
    float     orthographicHalfHeight = 15.0f;
    float     nearPlane              = -20.0f;
    float     farPlane               = 1000.0f;

    explicit OrthographicCameraRig(LimonAPI* limonAPI) : CameraExtensionInterface(limonAPI) {}

    std::string getName() const override { return "OrthographicCameraRig"; }

    std::vector<LimonTypes::GenericParameter> getParameters() const override {
        std::vector<LimonTypes::GenericParameter> descriptors;

        const char* descriptions[6] = {"Offset X", "Offset Y", "Offset Z", "Zoom (half height)", "Near plane", "Far plane"};
        const float values[6] = {offset.x, offset.y, offset.z, orthographicHalfHeight, nearPlane, farPlane};
        for (int parameterIndex = 0; parameterIndex < 6; ++parameterIndex) {
            LimonTypes::GenericParameter parameter{};
            parameter.requestType = LimonTypes::GenericParameter::RequestParameterTypes::FREE_NUMBER;
            parameter.valueType   = LimonTypes::GenericParameter::ValueTypes::DOUBLE;
            parameter.description  = descriptions[parameterIndex];
            parameter.value.doubleValue = values[parameterIndex];
            parameter.isSet = true;
            descriptors.push_back(parameter);
        }
        return descriptors;
    }

    void setParameters(std::vector<LimonTypes::GenericParameter> parameters) override {
        CameraExtensionInterface::setParameters(parameters);
        if (this->parameters.size() > 5) {
            bool allDoubles = true;
            for (int parameterIndex = 0; parameterIndex < 6; ++parameterIndex) {
                if (this->parameters[parameterIndex].valueType != LimonTypes::GenericParameter::ValueTypes::DOUBLE) {
                    allDoubles = false;
                    break;
                }
            }
            if (allDoubles) {
                offset = glm::vec3(this->parameters[0].value.doubleValue,
                                   this->parameters[1].value.doubleValue,
                                   this->parameters[2].value.doubleValue);
                orthographicHalfHeight = static_cast<float>(this->parameters[3].value.doubleValue);
                nearPlane              = static_cast<float>(this->parameters[4].value.doubleValue);
                farPlane               = static_cast<float>(this->parameters[5].value.doubleValue);
                float nearPlaneLimit = -farPlane / 49.0f;
                if (nearPlane > nearPlaneLimit) {
                    nearPlane = nearPlaneLimit;
                }
            }
        }
    }

    // Returning true every frame keeps the camera following the player as it moves.
    bool isDirty() const override { return dirty; }
    void clearDirty() override {}

    void getCameraVariables(glm::vec3& position, glm::vec3& center,
                            glm::vec3& up, glm::vec3& right) override {
        glm::vec3 playerPosition, playerCenter, playerUp, playerRight;
        limonAPI->getPlayerPosition(playerPosition, playerCenter, playerUp, playerRight);

        position = playerPosition + offset;
        center   = glm::normalize(playerPosition - position); // look toward the player
        up       = glm::vec3(0.0f, 1.0f, 0.0f);
        right    = glm::normalize(glm::cross(center, up));
    }

    CameraAttachment::ProjectionParameters getProjection() const override {
        CameraAttachment::ProjectionParameters projection;
        projection.type                   = CameraAttachment::ProjectionType::ORTHOGRAPHIC;
        projection.orthographicHalfHeight = orthographicHalfHeight;
        projection.nearPlane              = nearPlane;
        projection.farPlane               = farPlane;
        return projection;
    }
};

#endif //LIMONENGINE_ORTHOGRAPHICCAMERARIG_H
