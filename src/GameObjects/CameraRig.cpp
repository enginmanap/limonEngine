//
// Created by engin on 2026.
//

#include "CameraRig.h"

#include <glm/gtx/matrix_decompose.hpp>

void CameraRig::feedHeldAttachmentTransform() {
    if (heldAttachment == nullptr) {
        return;
    }
    Attachable* parent = getParentObject();
    if (parent == nullptr) {
        return; // unattached rig: the held behaviour produces its own pose
    }
    const glm::mat4 parentWorldTransform =
        parent->getAttachmentTransformFor(getParentBoneID())->getWorldTransform();
    // Decompose once here (C++) and feed components, so the held behaviour — including any Python rig —
    // never has to decompose a matrix every frame.
    glm::vec3 position, scale, skew;
    glm::quat orientation;
    glm::vec4 perspective;
    glm::decompose(parentWorldTransform, scale, orientation, position, skew, perspective);
    heldAttachment->setAttachmentTransform(position, orientation, scale);
}
