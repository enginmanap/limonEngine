//
// Created by engin on 18.06.2016.
//

#ifndef LIMONENGINE_LIGHT_H
#define LIMONENGINE_LIGHT_H


#include "glm/glm.hpp"
#include "GameObject.h"
#include "../GLHelper.h"
#include "../../libs/ImGui/imgui.h"
#include "../../libs/ImGuizmo/ImGuizmo.h"

class Light : public GameObject {
public:
    enum LightTypes {
        DIRECTIONAL, POINT
    };
private:
    GLHelper* glHelper;
    glm::mat4 shadowMatrices[6];//these are used only for point lights for now
    std::vector<glm::vec4> frustumPlanes;

    uint32_t objectID;
    glm::vec3 position, color;
    LightTypes lightType;
    bool frustumChanged;
    void setShadowMatricesForPosition(){
        shadowMatrices[0] =glHelper->getLightProjectionMatrixPoint() *
                           glm::lookAt(position, position + glm::vec3( 1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0));
        shadowMatrices[1] =glHelper->getLightProjectionMatrixPoint() *
                           glm::lookAt(position, position + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0));
        shadowMatrices[2] =glHelper->getLightProjectionMatrixPoint() *
                           glm::lookAt(position, position + glm::vec3( 0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
        shadowMatrices[3] =glHelper->getLightProjectionMatrixPoint() *
                           glm::lookAt(position, position + glm::vec3( 0.0,-1.0, 0.0), glm::vec3(0.0, 0.0,-1.0));
        shadowMatrices[4] =glHelper->getLightProjectionMatrixPoint() *
                           glm::lookAt(position, position + glm::vec3( 0.0, 0.0, 1.0), glm::vec3(0.0,-1.0, 0.0));
        shadowMatrices[5] =glHelper->getLightProjectionMatrixPoint() *
                           glm::lookAt(position, position + glm::vec3( 0.0, 0.0,-1.0), glm::vec3(0.0,-1.0, 0.0));
    }
public:
    Light(GLHelper *glHelper, uint32_t objectID, LightTypes lightType, const glm::vec3 &position,
              const glm::vec3 &color) :
            glHelper(glHelper),
            objectID(objectID),
            position(position),
            lightType(lightType) {
        this->color.r = color.r < 1.0f ? color.r : 1.0f;
        this->color.g = color.g < 1.0f ? color.g : 1.0f;
        this->color.b = color.b < 1.0f ? color.b : 1.0f;

        setShadowMatricesForPosition();

        glm::mat4 lightView = glm::lookAt(this->position,
                                          glm::vec3(0.0f, 0.0f, 0.0f),
                                          glm::vec3(0.0f, 1.0f, 0.0f));

        this->frustumPlanes.resize(6);
        glHelper->calculateFrustumPlanes(lightView, glHelper->getLightProjectionMatrixDirectional(), this->frustumPlanes);
        frustumChanged = true;
    }

    const glm::vec3 &getPosition() const {
        return position;
    }

    void setPosition(glm::vec3 position) {
        this->position = position;
        setShadowMatricesForPosition();

        glm::mat4 lightView = glm::lookAt(this->position,
                                          glm::vec3(0.0f, 0.0f, 0.0f),
                                          glm::vec3(0.0f, 1.0f, 0.0f));

        glHelper->calculateFrustumPlanes(lightView, glHelper->getLightProjectionMatrixDirectional(), this->frustumPlanes);
        frustumChanged = true;
        glHelper->setLight(*this, objectID);
    }

    const glm::vec3 &getColor() const {
        return color;
    }

    LightTypes getLightType() const {
        return lightType;
    }

     const glm::mat4 * getShadowMatrices() const {
        return shadowMatrices;
    }

    bool isFrustumChanged() const {
        return frustumChanged;
    }

    void setFrustumChanged(bool frustumChanged) {
        Light::frustumChanged = frustumChanged;
    }

    const std::vector<glm::vec4>& getFrustumPlanes() const {
        return frustumPlanes;
    }

    bool isShadowCaster(const glm::vec3& aabbMin, const glm::vec3& aabbMax, const glm::vec3& position) const {
        //there are 2 possibilities.
        // 1) if directional light -> check if in frustum
        // 2) point light -> check if within range

        switch (this->lightType) {
            case DIRECTIONAL:
                return glHelper->isInFrustum(aabbMin, aabbMax, this->frustumPlanes);
            case POINT:
                return true; //TODO not implemented yet
        }

    }

    /************Game Object methods **************/

    uint32_t getWorldObjectID() {
        return objectID;
    };

    ObjectTypes getTypeID() const {
        return GameObject::LIGHT;
    };

    std::string getName() const {
        std::string goName;
        switch (this->lightType) {
            case Light::DIRECTIONAL:
                goName = "DIRECTIONAL_" + std::to_string(objectID);
                break;
            case Light::POINT:
                goName = "POINT_" + std::to_string(objectID);
                break;
            default:
                goName = "Light with unknown type, please fix.";
        }
        return goName;
    };

    ImGuiResult addImGuiEditorElements(const glm::mat4 &cameraMatrix, const glm::mat4 &perspectiveMatrix) {
        static ImGuiResult request;

        bool updated = false;
        bool crudeUpdated = false;
        static glm::vec3 preciseTranslatePoint = this->position;
        updated = ImGui::SliderFloat("Precise Position X", &(this->position.x), preciseTranslatePoint.x - 5.0f, preciseTranslatePoint.x + 5.0f)   || updated;
        updated = ImGui::SliderFloat("Precise Position Y", &(this->position.y), preciseTranslatePoint.y - 5.0f, preciseTranslatePoint.y + 5.0f)   || updated;
        updated = ImGui::SliderFloat("Precise Position Z", &(this->position.z), preciseTranslatePoint.z - 5.0f, preciseTranslatePoint.z + 5.0f)   || updated;
        ImGui::NewLine();
        crudeUpdated = ImGui::SliderFloat("Crude Position X", &(this->position.x), -100.0f, 100.0f)   || crudeUpdated;
        crudeUpdated = ImGui::SliderFloat("Crude Position Y", &(this->position.y), -100.0f, 100.0f)   || crudeUpdated;
        crudeUpdated = ImGui::SliderFloat("Crude Position Z", &(this->position.z), -100.0f, 100.0f)   || crudeUpdated;
        ImGui::NewLine();
        updated = ImGui::SliderFloat("Color R", &(this->color.r), 0.0f, 1.0f)   || updated;
        updated = ImGui::SliderFloat("Color G", &(this->color.g), 0.0f, 1.0f)   || updated;
        updated = ImGui::SliderFloat("Color B", &(this->color.b), 0.0f, 1.0f)   || updated;
        ImGui::NewLine();

        if(updated || crudeUpdated) {
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
        ImGuizmo::Manipulate(glm::value_ptr(cameraMatrix), glm::value_ptr(perspectiveMatrix), ImGuizmo::TRANSLATE, mCurrentGizmoMode, glm::value_ptr(objectMatrix), NULL, useSnap ? &(snap[0]) : NULL);

        //now we should have object matrix updated, update the object
        this->setPosition(glm::vec3(objectMatrix[3][0], objectMatrix[3][1], objectMatrix[3][2]));

        return request;
    }
    /************Game Object methods **************/

};


#endif //LIMONENGINE_LIGHT_H
