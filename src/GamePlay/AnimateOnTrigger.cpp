//
// Created by engin on 20.05.2018.
//

#include "AnimateOnTrigger.h"
#include "LimonAPI.h"

/*
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
*/
std::vector<LimonAPI::ParameterRequest> AnimateOnTrigger::getParameters() {
    std::vector<LimonAPI::ParameterRequest> parameters;
    LimonAPI::ParameterRequest param1;
    param1.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::MODEL;
    param1.description = "Model to animate";
    parameters.push_back(param1);

    LimonAPI::ParameterRequest param2;
    param2.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::ANIMATION;
    param2.description = "Animation to apply";
    parameters.push_back(param2);

    LimonAPI::ParameterRequest param3;
    param3.requestType = LimonAPI::ParameterRequest::RequestParameterTypes::BOOLEAN;
    param3.description = "Is animation looped";
    parameters.push_back(param3);

    return parameters;
}

bool AnimateOnTrigger::run(std::vector<LimonAPI::ParameterRequest> parameters) {
    LimonAPI::animateModel(static_cast<uint32_t>(parameters[0].value.longValue),
                           static_cast<uint32_t>(parameters[1].value.longValue),
                           parameters[2].value.boolValue);
    return true;
}