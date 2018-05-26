//
// Created by engin on 8.05.2018.
//

#include "TriggerObject.h"
#include "../Assets/Animations/AnimationCustom.h"
#include "../../libs/ImGui/imgui.h"
#include "../GameObjects/Model.h"

GameObject::ImGuiResult TriggerObject::addImGuiEditorElements(const glm::mat4 &cameraMatrix, const glm::mat4 &perspectiveMatrix) {
    static ImGuiResult request;

    transformation.addImGuiEditorElements(cameraMatrix, perspectiveMatrix);


    bool isSet = LimonAPI::generateEditorElementsForParameters(runParameters);
    if(this->enabled) {
        if(ImGui::Button("Disable Trigger")) {
            this->enabled = false;
        }
    } else {
        if(isSet) {
            if (ImGui::Button("Enable Trigger")) {
                this->enabled = true;
            }
        } else {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Button("Enable Trigger");
            ImGui::PopStyleVar();
        }
    }

    return request;
}



