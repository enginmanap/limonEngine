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
#include "Camera/OrthographicCamera.h"
#include "Camera/CubeCamera.h"
#include "../Utils/HardCodedTags.h"
#include "Camera/PerspectiveCamera.h"

class Light : public GameObject, public CameraAttachment {
public:
    enum class LightTypes {
        NONE, DIRECTIONAL, POINT
    };

private:
    GraphicsInterface* graphicsWrapper;

    uint32_t objectID;
    glm::vec3 position, color;
    glm::vec3 playerPosition;
    glm::vec3 attenuation = glm::vec3(1,0.1,0.01);//const, linear, exponential
    glm::vec3 ambientColor = glm::vec3(0,0,0); //this will be added to all objects on shading phase
    OrthographicCamera* directionalCamera = nullptr;
    CubeCamera* cubeCamera = nullptr;
    LightTypes lightType;
    bool frustumChanged = true;




    void updateLightView(const PerspectiveCamera* playerCamera) {
        frustumChanged = true;
        directionalCamera->getCameraMatrix();
        directionalCamera->recalculateView(playerCamera);
        frustumChanged = true;
    }

public:
    Light(GraphicsInterface* graphicsWrapper, uint32_t objectID, LightTypes lightType, const glm::vec3 &position,
          const glm::vec3 &color) :
            graphicsWrapper(graphicsWrapper),
            objectID(objectID),
            lightType(lightType) {
        this->color.r = color.r < 1.0f ? color.r : 1.0f;
        this->color.g = color.g < 1.0f ? color.g : 1.0f;
        this->color.b = color.b < 1.0f ? color.b : 1.0f;

        if(lightType == LightTypes::DIRECTIONAL) {
            this->position = glm::normalize(position);
            directionalCamera = new OrthographicCamera(this->getName() + " camera", graphicsWrapper->getOptions(), this);
            directionalCamera->getCameraMatrix();
            directionalCamera->addRenderTag(HardCodedTags::OBJECT_MODEL_PHYSICAL);
            directionalCamera->addRenderTag(HardCodedTags::OBJECT_MODEL_STATIC);

            directionalCamera->addRenderTag(HardCodedTags::OBJECT_MODEL_BASIC);
            directionalCamera->addRenderTag(HardCodedTags::OBJECT_MODEL_ANIMATED);

            directionalCamera->addTag(HardCodedTags::CAMERA_LIGHT_DIRECTIONAL);
        } else if(lightType == LightTypes::POINT) {
            this->position = position;
            cubeCamera = new CubeCamera(this->getName() + " camera", graphicsWrapper->getOptions(), this);
            cubeCamera->getCameraMatrix();
            cubeCamera->addRenderTag(HardCodedTags::OBJECT_MODEL_PHYSICAL);
            cubeCamera->addRenderTag(HardCodedTags::OBJECT_MODEL_STATIC);

            cubeCamera->addRenderTag(HardCodedTags::OBJECT_MODEL_BASIC);
            cubeCamera->addRenderTag(HardCodedTags::OBJECT_MODEL_ANIMATED);

            cubeCamera->addTag(HardCodedTags::CAMERA_LIGHT_POINT);

        }
        //FIXME we are not rendering transparent objects when working with lights, yet.
        frustumChanged = true;
    }

    Camera* getCamera() const {
        switch (this->lightType) {
            case LightTypes::DIRECTIONAL:
                return directionalCamera;
            case LightTypes::POINT:
                return cubeCamera;
            case LightTypes::NONE:
            default:
                return nullptr;
        }
    }

    const glm::vec3 &getPosition() const {
        return position;
    }

    void setPosition(glm::vec3 position, const PerspectiveCamera* playerCamera);

    void step(long time [[gnu::unused]], PerspectiveCamera* playerCamera) {
        if(lightType == LightTypes::DIRECTIONAL) {
            updateLightView(playerCamera);
        }
    }

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

     const std::vector<glm::mat4>& getShadowMatrices() const {
        if (this->lightType == LightTypes::POINT) {
            return cubeCamera->getRenderMatrices();
        } else if(this->lightType == LightTypes::DIRECTIONAL) {
            return directionalCamera->getOrthogonalCameraMatrices();
        }
        std::cerr << "camera type is unknown, returning as directional." << std::endl;
        return directionalCamera->getOrthogonalCameraMatrices();
    }

    bool isFrustumChanged() const {
        return frustumChanged;
    }

    void setFrustumChanged(bool frustumChanged) {
        Light::frustumChanged = frustumChanged;
        //If frustum changed, we automatically set it, so this is only for clear
        if(!frustumChanged) {
            switch (this->lightType) {
                case LightTypes::NONE:
                    return;
                case LightTypes::DIRECTIONAL:
                    return directionalCamera->clearDirty();
                case LightTypes::POINT:
                    return cubeCamera->clearDirty();
            }
        }
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
                return cubeCamera->isVisible(physicalRenderable);
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
        this->setFrustumChanged(true);
    }

    float getActiveDistance() const {
        if(this->lightType == LightTypes::POINT) {
            return cubeCamera->getActiveDistance();
        }
        return 0.0f;//Only used by point lights
    }

    const glm::vec3 &getAmbientColor() const {
        return ambientColor;
    }

    void setAmbientColor(const glm::vec3 &ambientColor) {
        Light::ambientColor = ambientColor;
    }

    /**
     * we send parameters, that might not actually match what the name suggests, please make sure you are checking it
     * @param position  -> position (Directional light has direction instead of light)
     * @param center    -> player position (as directional light centers around the player)
     * @param up        -> renderPosition (Position that follows the player)
     * @param right     -> attenuation
     */
    void getCameraVariables(glm::vec3 &position, glm::vec3 &center, glm::vec3 &up[[gnu::unused]], glm::vec3 &right) override {
        position = this->position;
        center = this->playerPosition;
        right = this->attenuation;
    }

    bool isDirty() const override {
        return this->frustumChanged;
    }

    void clearDirty() override {
        this->frustumChanged = false;
    }
};


#endif //LIMONENGINE_LIGHT_H
