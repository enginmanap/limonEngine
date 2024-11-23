//
// Created by engin on 12/02/2023.
//

#ifndef LIMONENGINE_CUBECAMERA_H
#define LIMONENGINE_CUBECAMERA_H


#include "Camera.h"
#include "PhysicalRenderable.h"
#include "CameraAttachment.h"

class CubeCamera : public Camera {
    std::vector<glm::mat4> renderMatrices;//these are used only for point lights for now
    CameraAttachment* cameraAttachment;
    glm::vec3 position, center, up, right;
    OptionsUtil::Options *options;
    OptionsUtil::Options::Option<long> shadowMapWidthOption;
    OptionsUtil::Options::Option<long> shadowMapHeightOption;
    bool dirty = true;
    float activeDistance;
    std::vector<std::vector<glm::vec4>> frustumCorners;


public:

    CubeCamera(const std::string& name, OptionsUtil::Options *options, CameraAttachment* cameraAttachment) :
            cameraAttachment(cameraAttachment),
            position(0,20,0),
            center(glm::vec3(0, 0, -1)),
            up(glm::vec3(0, 1, 0)),
            right(glm::vec3(-1, 0, 0)),
            options(options){
        renderMatrices.resize(6);
        this->name = name;
        shadowMapWidthOption = options->getOption<long>(HASH("shadowMapPointWidth"));
        shadowMapHeightOption = options->getOption<long>(HASH("shadowMapPointHeight"));
    }

    CameraTypes getType() const override {
        return CameraTypes::CUBE;
    };

    bool isDirty() const override {
        return this->dirty || cameraAttachment->isDirty();
    };

    void clearDirty() override {
        this->dirty = false;
    }

    bool isVisible(const PhysicalRenderable& renderable) const override {
        return glm::distance2(renderable.getTransformation()->getTranslate(), this->position) < activeDistance * activeDistance;
    }

    bool isResultVisibleOnOtherCamera(const PhysicalRenderable& renderable[[gnu::unused]], const Camera* otherCamera[[gnu::unused]]) const {
        std::cerr << "Multiple camera culling for cube camera is not implemented!" << std::endl;
        return true;
    }

    const glm::mat4& getCameraMatrix() override {
        if(isDirty()) {
            cameraAttachment->getCameraVariables(position,center,up,right);
            calculateActiveDistance(right);
            setShadowMatricesForPosition();
        }
        return renderMatrices[0]; // don't use
    }

    const glm::mat4& getCameraMatrixConst() const override {
        std::cerr << "Cube Camera can't provide single camera matrix" << std::endl;
        return renderMatrices[0]; // don't use
    }

    const glm::mat4& getProjectionMatrix() const override {
        std::cerr << "Cube Camera can't provide single projection matrix" << std::endl;
        exit(-1);
    }

    const std::vector<glm::mat4>& getRenderMatrices() const {
        return renderMatrices;
    }

    float getActiveDistance() {
        return activeDistance;
    }

private:
    void calculateActiveDistance(const glm::vec3& attenuation) {
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
                activeDistance = ((float)sqrt(1000) - attenuation.x) / attenuation.y;
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

    void setShadowMatricesForPosition(){
        long sWidth, sHeight;
        sWidth = shadowMapWidthOption.getOrDefault(512);
        sHeight = shadowMapHeightOption.getOrDefault(512);

        OptionsUtil::Options::Option<double> near = options->getOption<double>(HASH("lightPerspectiveProjectionNearPlane"));
        OptionsUtil::Options::Option<double> far = options->getOption<double>(HASH("lightPerspectiveProjectionFarPlane"));

        glm::mat4 lightProjectionMatrixPoint = glm::perspective(glm::radians(90.0f),
                                                      (float)sWidth/(float)sHeight,
                                                                (float)near.getOrDefault(0.1),
                                                                (float)far.getOrDefault(0.1));
        renderMatrices[0] = lightProjectionMatrixPoint *
                            glm::lookAt(position, position + glm::vec3( 1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0));
        renderMatrices[1] = lightProjectionMatrixPoint *
                            glm::lookAt(position, position + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0));
        renderMatrices[2] = lightProjectionMatrixPoint *
                            glm::lookAt(position, position + glm::vec3( 0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
        renderMatrices[3] = lightProjectionMatrixPoint *
                            glm::lookAt(position, position + glm::vec3( 0.0,-1.0, 0.0), glm::vec3(0.0, 0.0,-1.0));
        renderMatrices[4] = lightProjectionMatrixPoint *
                            glm::lookAt(position, position + glm::vec3( 0.0, 0.0, 1.0), glm::vec3(0.0,-1.0, 0.0));
        renderMatrices[5] = lightProjectionMatrixPoint *
                            glm::lookAt(position, position + glm::vec3( 0.0, 0.0,-1.0), glm::vec3(0.0,-1.0, 0.0));
    }

};

#endif //LIMONENGINE_CUBECAMERA_H
