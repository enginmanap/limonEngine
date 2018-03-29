//
// Created by engin on 18.06.2016.
//

#ifndef LIMONENGINE_LIGHT_H
#define LIMONENGINE_LIGHT_H


#include "glm/glm.hpp"
#include "GameObject.h"
#include "../GLHelper.h"
#include "../../libs/ImGui/imgui.h"

class Light : public GameObject {
public:
    enum LightTypes {
        DIRECTIONAL, POINT
    };
private:
    GLHelper* glHelper;
    glm::mat4 shadowMatrices[6];//these are used only for point lights for now
    uint32_t objectID;
    glm::vec3 position, color;
    LightTypes lightType;

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
    }



    const glm::vec3 &getPosition() const {
        return position;
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

    void addImGuiEditorElements() {
        bool updated = false;
        bool crudeUpdated = false;
        static glm::vec3 preciseTranslatePoint = this->position;
        ImGui::Text("%s",getName().c_str());                           // Some text (you can use a format string too)
        ImGui::Text("%s",getName().c_str());                           // Some text (you can use a format string too)
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
            this->position = position;
            setShadowMatricesForPosition();
            glHelper->setLight(*this, objectID);
        }
        if(crudeUpdated) {
            preciseTranslatePoint = this->position;
        }

    }
    /************Game Object methods **************/
};


#endif //LIMONENGINE_LIGHT_H
