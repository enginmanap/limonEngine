//
// Created by engin on 2026.
//

#ifndef LIMONENGINE_OBJECTATTACHEDCAMERARIG_H
#define LIMONENGINE_OBJECTATTACHEDCAMERARIG_H

#include "limonAPI/CameraExtensionInterface.h"
#include <glm/glm.hpp>

/**
 * A registered camera rig that follows the object its CameraRig is attached to.
 *
 * The target is the CameraRig's attachment parent (set through the engine's standard attachment system);
 * the engine resolves that parent's world transform and hands it to this rig through setAttachmentTransform()
 * each frame. The rig only composes its local offset and look-at on top of it.
 *
 * Configuration (GenericParameters):
 *   params[0..2] DOUBLE : local offset (x,y,z) of the camera in the target's local frame.
 */
class ObjectAttachedCameraRig : public CameraExtensionInterface {
    glm::vec3 targetPosition    = glm::vec3(0.0f);
    glm::quat targetOrientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    bool dirty = true;
public:
    glm::vec3 localOffset     = glm::vec3(0.0f, 40.0f, 8.0f);

    explicit ObjectAttachedCameraRig(LimonAPI* limonAPI) : CameraExtensionInterface(limonAPI) {}

    std::string getName() const override { return "ObjectAttachedCameraRig"; }

    // Advertise editable descriptors so the editor renders offset drags (and so the saved XML round-trips).
    // params[0..2] = local offset X/Y/Z. The target object is the CameraRig's attachment parent, not a param.
    std::vector<LimonTypes::GenericParameter> getParameters() const override {
        std::vector<LimonTypes::GenericParameter> descriptors;

        const char* offsetDescriptions[3] = {"Offset X", "Offset Y", "Offset Z"};
        const float offsetValues[3] = {localOffset.x, localOffset.y, localOffset.z};
        for (int axis = 0; axis < 3; ++axis) {
            LimonTypes::GenericParameter offsetParameter{};
            offsetParameter.requestType = LimonTypes::GenericParameter::RequestParameterTypes::FREE_NUMBER;
            offsetParameter.valueType   = LimonTypes::GenericParameter::ValueTypes::DOUBLE;
            offsetParameter.description  = offsetDescriptions[axis];
            offsetParameter.value.doubleValue = offsetValues[axis];
            offsetParameter.isSet = true;
            descriptors.push_back(offsetParameter);
        }
        return descriptors;
    }

    void setParameters(std::vector<LimonTypes::GenericParameter> parameters) override {
        CameraExtensionInterface::setParameters(parameters);
        if (this->parameters.size() > 2
            && this->parameters[0].valueType == LimonTypes::GenericParameter::ValueTypes::DOUBLE
            && this->parameters[1].valueType == LimonTypes::GenericParameter::ValueTypes::DOUBLE
            && this->parameters[2].valueType == LimonTypes::GenericParameter::ValueTypes::DOUBLE) {
            this->localOffset = glm::vec3(this->parameters[0].value.doubleValue,
                                          this->parameters[1].value.doubleValue,
                                          this->parameters[2].value.doubleValue);
        }
    }

    // Engine feeds the target object's world transform each frame (pre-decomposed) before getCameraVariables().
    void setAttachmentTransform(const glm::vec3& position, const glm::quat& orientation,
                                const glm::vec3& /*scale*/) override {
        this->targetPosition    = position;
        this->targetOrientation = orientation;
        this->dirty = true;
    }

    bool isDirty() const override { return dirty; }
    void clearDirty() override { dirty = false; }

    void getCameraVariables(glm::vec3& position, glm::vec3& center,
                            glm::vec3& up, glm::vec3& right) override {
        // Offset is rotated by the object's orientation (scale ignored, so the camera distance is stable).
        position = targetPosition + targetOrientation * localOffset;
        center   = glm::normalize(targetPosition - position); // look at the object
        up       = glm::vec3(0.0f, 1.0f, 0.0f);
        right    = glm::normalize(glm::cross(center, up));
    }

    CameraAttachment::ProjectionParameters getProjection() const override {
        return CameraAttachment::ProjectionParameters{}; // perspective
    }
};

#endif //LIMONENGINE_OBJECTATTACHEDCAMERARIG_H
