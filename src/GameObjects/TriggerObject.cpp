//
// Created by engin on 8.05.2018.
//

#include "TriggerObject.h"

GameObject::ImGuiResult TriggerObject::addImGuiEditorElements(const glm::mat4 &cameraMatrix, const glm::mat4 &perspectiveMatrix) {
    static ImGuiResult request;

    transformation.addImGuiEditorElements(cameraMatrix, perspectiveMatrix);
    return request;
}
