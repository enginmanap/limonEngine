//
// Created by engin on 8.05.2018.
//

#include "TriggerObject.h"
#include "../../libs/ImGui/imgui.h"

GameObject::ImGuiResult TriggerObject::addImGuiEditorElements() {
    static ImGuiResult request;
    static glm::vec3 preciseTranslatePoint = this->translate;

    bool updated = false;
    bool crudeUpdated = false;

    if (ImGui::IsKeyPressed(83)) {
        request.useSnap = !request.useSnap;
    }

/*
 * at first we decide whether we are in rotation, scale or translate mode.
 */

    if (ImGui::RadioButton("Translate", request.mode == EditorModes::TRANSLATE_MODE)) {
        request.mode = EditorModes::TRANSLATE_MODE;
    }

    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", request.mode == EditorModes::ROTATE_MODE)) {
        request.mode = EditorModes::ROTATE_MODE;
    }

    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", request.mode == EditorModes::SCALE_MODE)) {
        request.mode = EditorModes::SCALE_MODE;
    }

    switch (request.mode) {
        case TRANSLATE_MODE:
            updated =
                    ImGui::DragFloat("Precise Position X", &(this->translate.x), 0.01f, preciseTranslatePoint.x - 5.0f,
                                     preciseTranslatePoint.x + 5.0f) || updated;
            updated =
                    ImGui::DragFloat("Precise Position Y", &(this->translate.y), 0.01f, preciseTranslatePoint.y - 5.0f,
                                     preciseTranslatePoint.y + 5.0f) || updated;
            updated =
                    ImGui::DragFloat("Precise Position Z", &(this->translate.z), 0.01f, preciseTranslatePoint.z - 5.0f,
                                     preciseTranslatePoint.z + 5.0f) || updated;
            ImGui::NewLine();
            crudeUpdated =
                    ImGui::SliderFloat("Crude Position X", &(this->translate.x), -100.0f, 100.0f) || crudeUpdated;
            crudeUpdated =
                    ImGui::SliderFloat("Crude Position Y", &(this->translate.y), -100.0f, 100.0f) || crudeUpdated;
            crudeUpdated =
                    ImGui::SliderFloat("Crude Position Z", &(this->translate.z), -100.0f, 100.0f) || crudeUpdated;
            if (updated || crudeUpdated) {
                this->setTranslate(translate);
            }
            if (crudeUpdated) {
                preciseTranslatePoint = this->translate;
            }
            ImGui::NewLine();
            ImGui::Checkbox("", &(request.useSnap));
            ImGui::SameLine();
            ImGui::InputFloat3("Snap", &(request.snap[0]));
            break;
        case ROTATE_MODE:
            updated = ImGui::SliderFloat("Rotate X", &(this->orientation.x), -1.0f, 1.0f) || updated;
            updated = ImGui::SliderFloat("Rotate Y", &(this->orientation.y), -1.0f, 1.0f) || updated;
            updated = ImGui::SliderFloat("Rotate Z", &(this->orientation.z), -1.0f, 1.0f) || updated;
            updated = ImGui::SliderFloat("Rotate W", &(this->orientation.w), -1.0f, 1.0f) || updated;
            if (updated || crudeUpdated) {
                this->setOrientation(orientation);
            }
            ImGui::NewLine();
            ImGui::Checkbox("", &(request.useSnap));
            ImGui::SameLine();
            ImGui::InputFloat("Angle Snap", &(request.snap[0]));
            break;
        case SCALE_MODE:
            glm::vec3 tempScale = scale;
            updated = ImGui::DragFloat("Scale X", &(tempScale.x), 0.01, 0.01f, 10.0f) || updated;
            updated = ImGui::DragFloat("Scale Y", &(tempScale.y), 0.01, 0.01f, 10.0f) || updated;
            updated = ImGui::DragFloat("Scale Z", &(tempScale.z), 0.01, 0.01f, 10.0f) || updated;
            ImGui::NewLine();
            updated = ImGui::SliderFloat("Massive Scale X", &(tempScale.x), 0.01f, 100.0f) || updated;
            updated = ImGui::SliderFloat("Massive Scale Y", &(tempScale.y), 0.01f, 100.0f) || updated;
            updated = ImGui::SliderFloat("Massive Scale Z", &(tempScale.z), 0.01f, 100.0f) || updated;
            if ((updated || crudeUpdated) && (tempScale.x != 0.0f && tempScale.y != 0.0f && tempScale.z != 0.0f)) {
//it is possible to enter any scale now. If user enters 0, don't update
                this->setScale(tempScale);
            }
            ImGui::NewLine();
            ImGui::Checkbox("", &(request.useSnap));
            ImGui::SameLine();
            ImGui::InputFloat("Scale Snap", &(request.snap[0]));
            break;
    }
    ImGui::NewLine();
    request.isGizmoRequired = true;
    return request;
}
