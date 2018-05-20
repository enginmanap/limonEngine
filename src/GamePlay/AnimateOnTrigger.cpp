//
// Created by engin on 20.05.2018.
//

#include "AnimateOnTrigger.h"
#include "../PhysicalRenderable.h"
#include "../GameObjects/Model.h"
#include "../Assets/Animations/AnimationCustom.h"
#include "LimonAPI.h"
#include "../../libs/ImGui/imgui.h"

bool AnimateOnTrigger::addEditorElements() {

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
    ImGui::Checkbox("Looped animation", &(this->loop));

    //if both model and animation is set, means can be enabled;
    return (this->model && this->animation);
}

bool AnimateOnTrigger::run() {
    LimonAPI::animateModel(model, animation, loop);
    return true;
}
