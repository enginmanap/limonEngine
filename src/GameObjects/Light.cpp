//
// Created by engin on 18.06.2016.
//

#include <glm/ext.hpp>
#include "Light.h"


void Light::setPosition(glm::vec3 position, const PerspectiveCamera* playerCamera) {
    this->position = position;
    switch (lightType) {
        case LightTypes::NONE:
            return;
        case LightTypes::POINT: this->frustumChanged = true; cubeCamera->getCameraMatrix(); this->frustumChanged = true;
            break;
        case LightTypes::DIRECTIONAL:
            this->position = glm::normalize(position);
            updateLightView(playerCamera);
        break;
    }
}

ImGuiResult Light::addImGuiEditorElements(const ImGuiRequest &request) {
    ImGuiResult result;

    ImGui::Text("Please note, Directional lights position setting is relative to player.");
    static glm::vec3 preciseTranslatePoint = this->position;
    bool crudeUpdated = false;
    result.updated = ImGui::DragFloat("Color R", &(this->color.r), 0.0f, 1.0f)   || result.updated;
    result.updated = ImGui::DragFloat("Color G", &(this->color.g), 0.0f, 1.0f)   || result.updated;
    result.updated = ImGui::DragFloat("Color B", &(this->color.b), 0.0f, 1.0f)   || result.updated;
    ImGui::NewLine();
    bool attenuationUpdate = false;
    switch (lightType) {
        case LightTypes::NONE:
            break;
        case LightTypes::POINT: {
            result.updated = ImGui::DragFloat("Precise Position X", &(this->position.x), 0.01, preciseTranslatePoint.x - 5.0f, preciseTranslatePoint.x + 5.0f)   || result.updated;
            result.updated = ImGui::DragFloat("Precise Position Y", &(this->position.y), 0.01, preciseTranslatePoint.y - 5.0f, preciseTranslatePoint.y + 5.0f)   || result.updated;
            result.updated = ImGui::DragFloat("Precise Position Z", &(this->position.z), 0.01, preciseTranslatePoint.z - 5.0f, preciseTranslatePoint.z + 5.0f)   || result.updated;
            ImGui::NewLine();
            crudeUpdated = ImGui::SliderFloat("Crude Position X", &(this->position.x), -100.0f, 100.0f)   || crudeUpdated;
            crudeUpdated = ImGui::SliderFloat("Crude Position Y", &(this->position.y), -100.0f, 100.0f)   || crudeUpdated;
            crudeUpdated = ImGui::SliderFloat("Crude Position Z", &(this->position.z), -100.0f, 100.0f)   || crudeUpdated;
            ImGui::NewLine();
            attenuationUpdate = ImGui::DragFloat("Constant", &(this->attenuation.x), 0.01f, -10.0f, 1.0f) || attenuationUpdate;
            attenuationUpdate = ImGui::DragFloat("Linear", &(this->attenuation.y), 0.01f, 0.0f, 1.0f) || attenuationUpdate;
            attenuationUpdate = ImGui::DragFloat("Exponential", &(this->attenuation.z), 0.01f, 0.0f, 1.0f) || attenuationUpdate;
            if (attenuationUpdate) {
                this->setFrustumChanged(true);
                result.updated = true;
            }
            ImGui::NewLine();
        }
        break;
        case LightTypes::DIRECTIONAL: {
            result.updated = ImGui::DragFloat3("Ambient", glm::value_ptr(ambientColor), 0.01f, 0.0f, 1.0f) || result.updated;
            ImGui::NewLine();
            result.updated = ImGui::DragFloat("Precise Position X", &(this->position.x), 0.001, -1.0f, 1.0f)   || result.updated;
            result.updated = ImGui::DragFloat("Precise Position Y", &(this->position.y), 0.001, -1.0f, 0.0f)   || result.updated;
            result.updated = ImGui::DragFloat("Precise Position Z", &(this->position.z), 0.001, -1.0f, 1.0f)   || result.updated;
            this->position = glm::normalize(this->position);
        }
        break;
    }

    if(result.updated || crudeUpdated) {
        this->setPosition(position, request.playerCamera);
    }
    if(crudeUpdated) {
        preciseTranslatePoint = this->position;
    }

    /* IMGUIZMO PART */

    static bool useSnap; //these are static because we want to keep the values
    static float snap[3] = {1.0f, 1.0f, 1.0f};
    ImGui::NewLine();
    ImGui::Checkbox("", &(useSnap));
    ImGui::SameLine();
    ImGui::InputFloat3("Snap", &(snap[0]));

    glm::mat4 objectMatrix = glm::translate(glm::mat4(1.0f), position);
    ImGuizmo::BeginFrame();
    static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    ImGuizmo::Manipulate(glm::value_ptr(request.perspectiveCameraMatrix), glm::value_ptr(request.perspectiveMatrix), ImGuizmo::TRANSLATE, mCurrentGizmoMode, glm::value_ptr(objectMatrix), NULL, useSnap ? &(snap[0]) : NULL);

    //now we should have object matrix updated, update the object
    this->setPosition(glm::vec3(objectMatrix[3][0], objectMatrix[3][1], objectMatrix[3][2]), request.playerCamera);

    if(ImGui::Button("Remove light")) {
        result.remove = true;
        std::cout << "remove button press" << std::endl;
    }

    return result;
}
