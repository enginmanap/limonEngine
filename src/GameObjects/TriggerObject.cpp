//
// Created by engin on 8.05.2018.
//

#include "TriggerObject.h"
#include "../GamePlay/LimonAPI.h"
#include "../GamePlay/AnimationCustom.h"
#include "../../libs/ImGui/imgui.h"
#include "../GameObjects/Model.h"

GameObject::ImGuiResult TriggerObject::addImGuiEditorElements(const glm::mat4 &cameraMatrix, const glm::mat4 &perspectiveMatrix) {
    static ImGuiResult request;

    transformation.addImGuiEditorElements(cameraMatrix, perspectiveMatrix);

    //let user select model and animation.

    const std::map<uint32_t, PhysicalRenderable *> &worldObject = LimonAPI::getObjects();
    std::string currentObject;
    if(this->model) {
        currentObject = this->model->getName();
    } else {
        currentObject = "Not selected";
    }
    if (ImGui::BeginCombo("Available objects", currentObject.c_str())) {
        for (auto it = worldObject.begin();
             it != worldObject.end(); it++) {
            if (ImGui::Selectable(dynamic_cast<Model*>((it->second))->getName().c_str())) {
                this->model = dynamic_cast<Model*>((it->second));
            }
        }
        ImGui::EndCombo();
    }

    std::string currentAnimation;
    if(this->animation) {
        currentAnimation = this->animation->getName();
    } else {
        currentAnimation = "Not selected";
    }
    const std::vector<AnimationCustom>& worldAnimations = LimonAPI::getAnimations();
    if (ImGui::BeginCombo("Available Animations", currentAnimation.c_str())) {
        for (auto it = worldAnimations.begin();
             it != worldAnimations.end(); it++) {
            if (ImGui::Selectable(it->getName().c_str())) {
                this->animation = &(*it);//FIXME whats this?
            }
        }
        ImGui::EndCombo();
    }

    if(this->enabled) {
        if(ImGui::Button("Disable Trigger")) {
            this->enabled = false;
        }
    } else {
        if(this->model != nullptr && this->animation != nullptr) {
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



