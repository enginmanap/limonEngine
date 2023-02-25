//
// Created by engin on 18.06.2016.
//

#ifndef LIMONENGINE_LIGHT_H
#define LIMONENGINE_LIGHT_H


#include <glm/gtx/norm.hpp>
#include "glm/glm.hpp"
#include "GameObject.h"
#include "CameraAttachment.h"
#include "API/Graphics/GraphicsInterface.h"
#include "../../libs/ImGui/imgui.h"
#include "../../libs/ImGuizmo/ImGuizmo.h"
#include "../Camera/OrthographicCamera.h"

class Light : public GameObject, public CameraAttachment {
public:
    enum class LightTypes {
        NONE, DIRECTIONAL, POINT
    };
    const glm::mat4 getLightSpaceMatrix() const;

private:
    GraphicsInterface* graphicsWrapper;
    glm::mat4 shadowMatrices[6];//these are used only for point lights for now

    uint32_t objectID;
    glm::vec3 position, color;
    const glm::vec3 CENTER =  glm::vec3(0.0f, 0.0f, 0.0f);
    const glm::vec3 UP = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 renderPosition; //for directional lights, moves with player.
    glm::vec3 attenuation = glm::vec3(1,0.1,0.01);//const, linear, exponential
    glm::vec3 ambientColor = glm::vec3(0,0,0); //this will be added to all objects on shading phase
    float activeDistance = 10;//will auto recalculate on constructor
    OrthographicCamera* directionalCamera;
    LightTypes lightType;
    bool frustumChanged = true;

    void setShadowMatricesForPosition(){
        shadowMatrices[0] =graphicsWrapper->getLightProjectionMatrixPoint() *
                           glm::lookAt(position, position + glm::vec3( 1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0));
        shadowMatrices[1] =graphicsWrapper->getLightProjectionMatrixPoint() *
                           glm::lookAt(position, position + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0));
        shadowMatrices[2] =graphicsWrapper->getLightProjectionMatrixPoint() *
                           glm::lookAt(position, position + glm::vec3( 0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
        shadowMatrices[3] =graphicsWrapper->getLightProjectionMatrixPoint() *
                           glm::lookAt(position, position + glm::vec3( 0.0,-1.0, 0.0), glm::vec3(0.0, 0.0,-1.0));
        shadowMatrices[4] =graphicsWrapper->getLightProjectionMatrixPoint() *
                           glm::lookAt(position, position + glm::vec3( 0.0, 0.0, 1.0), glm::vec3(0.0,-1.0, 0.0));
        shadowMatrices[5] =graphicsWrapper->getLightProjectionMatrixPoint() *
                           glm::lookAt(position, position + glm::vec3( 0.0, 0.0,-1.0), glm::vec3(0.0,-1.0, 0.0));
    }

    void calculateActiveDistance();

    void updateLightView();

public:
    Light(GraphicsInterface* graphicsWrapper, uint32_t objectID, LightTypes lightType, const glm::vec3 &position,
          const glm::vec3 &color) :
            graphicsWrapper(graphicsWrapper),
            objectID(objectID),
            position(position),
            lightType(lightType) {
        this->color.r = color.r < 1.0f ? color.r : 1.0f;
        this->color.g = color.g < 1.0f ? color.g : 1.0f;
        this->color.b = color.b < 1.0f ? color.b : 1.0f;

        setShadowMatricesForPosition();

        glm::mat4 lightView = glm::lookAt(this->position,
                                          CENTER,
                                          UP);
        if(lightType == LightTypes::DIRECTIONAL) {
            directionalCamera = new OrthographicCamera(graphicsWrapper->getOptions(), this);
            directionalCamera->getCameraMatrix();
        }
        if(lightType == LightTypes::POINT) {
            calculateActiveDistance();
        }
        frustumChanged = true;
    }

    const glm::vec3 &getPosition() const {
        return position;
    }

    void setPosition(glm::vec3 position);

    void step(long time);

    const glm::vec3 &getColor() const {
        return color;
    }

    void setColor(glm::vec3 color) {
        this->color = color;
        this->setFrustumChanged(true);//the change is not frustum, but at this point this flag is a general dirty flag
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


    bool isShadowCaster(const PhysicalRenderable& physicalRenderable) const {
        //there are 2 possibilities.
        // 1) if directional light -> check if in frustum
        // 2) point light -> check if within range

        switch (this->lightType) {
            case LightTypes::NONE:
                return false;
            case LightTypes::DIRECTIONAL:
                return directionalCamera->isVisible(physicalRenderable);
            case LightTypes::POINT:
            return (glm::distance2(physicalRenderable.getTransformation()->getTranslate(), this->position) < activeDistance * activeDistance);
        }
        return true;//for safety only
    }

    /************Game Object methods **************/

    uint32_t getWorldObjectID() const override {
        return objectID;
    };

    ObjectTypes getTypeID() const override {
        return GameObject::LIGHT;
    };

    std::string getName() const override {
        std::string goName;
        switch (this->lightType) {
            case LightTypes::DIRECTIONAL:
                goName = "DIRECTIONAL_" + std::to_string(objectID);
                break;
            case LightTypes::POINT:
                goName = "POINT_" + std::to_string(objectID);
                break;
            default:
                goName = "Light with unknown type, please fix.";
        }
        return goName;
    };

    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request) override;
    /************Game Object methods **************/

    glm::vec3 getAttenuation() const {
        return attenuation;
    }

    void setAttenuation(const glm::vec3& attenuation) {
        this->attenuation = attenuation;
        calculateActiveDistance();
        this->setFrustumChanged(true);
    }

    float getActiveDistance() const {
        return activeDistance;
    }

    const glm::vec3 &getAmbientColor() const {
        return ambientColor;
    }

    void setAmbientColor(const glm::vec3 &ambientColor) {
        Light::ambientColor = ambientColor;
    }

    void getCameraVariables(glm::vec3 &position, glm::vec3 &center, glm::vec3 &up, glm::vec3 &right) override {
        position = this->renderPosition;
        center = this->renderPosition - this->position;
        up = this->UP;
    }

    bool isDirty() const override {
        return this->frustumChanged;
    }

    void clearDirty() override {
        this->frustumChanged = false;
    }
};


#endif //LIMONENGINE_LIGHT_H
