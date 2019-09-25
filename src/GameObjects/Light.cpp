//
// Created by engin on 18.06.2016.
//

#include <glm/ext.hpp>
#include "Light.h"

void Light::calculateActiveDistance() {
    /*
     * to cut off at 0.1,
     * for a = const, b = linear, c = exp attenuation
     * c*d^2 + b*d + a = 1000;
     *
     * since we want 10, we should calculate for (a - 1000)
     */

    //calculate discriminant
    //b^2 - 4*a*c

    if(attenuation.z == 0) {
        if(attenuation.y == 0) {
            activeDistance = 500;//max
        } else {
            //z = 0 means this is not a second degree equation. handle it
            // mx + n = y
            // when y < sqrt(1000) is active limit
            activeDistance = (sqrt(1000) - attenuation.x) / attenuation.y;
        }
    } else {
        float discriminant = attenuation.y * attenuation.y - (4 * (attenuation.x - 1000) * attenuation.z);
        if (discriminant < 0) {
            activeDistance = 0;
        } else {
            activeDistance = (-1 * attenuation.y);
            if (activeDistance > discriminant) {
                activeDistance = activeDistance - std::sqrt(discriminant);
            } else {
                activeDistance = activeDistance + std::sqrt(discriminant);
            }

            activeDistance = activeDistance / (2 * attenuation.z);
        }
    }
}

void Light::step(long time __attribute__((unused))) {
    if(lightType == DIRECTIONAL) {
        updateLightView();

    }
}

void Light::updateLightView() {
    glm::vec3 playerPos = graphicsWrapper->getCameraPosition();
    renderPosition = position + playerPos;

    glm::mat4 lightView = lookAt(renderPosition,
                                 playerPos,
                                 glm::vec3(0.0f, 1.0f, 0.0f));

    lightSpaceMatrix = graphicsWrapper->getLightProjectionMatrixDirectional() * lightView;

    graphicsWrapper->calculateFrustumPlanes(lightView, graphicsWrapper->getLightProjectionMatrixDirectional(), frustumPlanes);
    frustumChanged = true;
}

const glm::mat4 &Light::getLightSpaceMatrix() const {
    return lightSpaceMatrix;
}

void Light::setPosition(glm::vec3 position) {
    this->position = position;
    switch (lightType) {
        case POINT: setShadowMatricesForPosition();
            break;
        case DIRECTIONAL: updateLightView();
        break;
    }
}

GameObject::ImGuiResult Light::addImGuiEditorElements(const GameObject::ImGuiRequest &request) {
    ImGuiResult result;

    ImGui::Text("Please note, Directional lights position setting is relative to player.");

    bool crudeUpdated = false;
    static glm::vec3 preciseTranslatePoint = this->position;
    result.updated = ImGui::DragFloat("Precise Position X", &(this->position.x), 0.01, preciseTranslatePoint.x - 5.0f, preciseTranslatePoint.x + 5.0f)   || result.updated;
    result.updated = ImGui::DragFloat("Precise Position Y", &(this->position.y), 0.01, preciseTranslatePoint.y - 5.0f, preciseTranslatePoint.y + 5.0f)   || result.updated;
    result.updated = ImGui::DragFloat("Precise Position Z", &(this->position.z), 0.01, preciseTranslatePoint.z - 5.0f, preciseTranslatePoint.z + 5.0f)   || result.updated;
    ImGui::NewLine();
    crudeUpdated = ImGui::SliderFloat("Crude Position X", &(this->position.x), -100.0f, 100.0f)   || crudeUpdated;
    crudeUpdated = ImGui::SliderFloat("Crude Position Y", &(this->position.y), -100.0f, 100.0f)   || crudeUpdated;
    crudeUpdated = ImGui::SliderFloat("Crude Position Z", &(this->position.z), -100.0f, 100.0f)   || crudeUpdated;
    ImGui::NewLine();
    result.updated = ImGui::DragFloat("Color R", &(this->color.r), 0.0f, 1.0f)   || result.updated;
    result.updated = ImGui::DragFloat("Color G", &(this->color.g), 0.0f, 1.0f)   || result.updated;
    result.updated = ImGui::DragFloat("Color B", &(this->color.b), 0.0f, 1.0f)   || result.updated;
    ImGui::NewLine();
    bool attenuationUpdate = false;
    switch (lightType) {
        case LightTypes::POINT: {
            attenuationUpdate = ImGui::DragFloat("Constant", &(this->attenuation.x), 0.01f, 0.0f, 1.0f) || attenuationUpdate;
            attenuationUpdate = ImGui::DragFloat("Linear", &(this->attenuation.y), 0.01f, 0.0f, 1.0f) || attenuationUpdate;
            attenuationUpdate = ImGui::DragFloat("Exponential", &(this->attenuation.z), 0.01f, 0.0f, 1.0f) || attenuationUpdate;
            if (attenuationUpdate) {
                calculateActiveDistance();
                result.updated = true;
            }
            ImGui::NewLine();
        }
        break;
        case LightTypes::DIRECTIONAL: {
            result.updated = ImGui::DragFloat3("Ambient", glm::value_ptr(ambientColor), 0.01f, 0.0f, 1.0f) || result.updated;
            ImGui::NewLine();
        }
        break;
    }

    if(result.updated || crudeUpdated) {
        this->setPosition(position);
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
    this->setPosition(glm::vec3(objectMatrix[3][0], objectMatrix[3][1], objectMatrix[3][2]));

    if(ImGui::Button("Remove light")) {
        result.remove = true;
        std::cout << "remove button press" << std::endl;
    }

    return result;
}
