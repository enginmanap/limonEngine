//
// Created by Engin Manap on 17.02.2016.
//

#ifndef LIMONENGINE_PERSPECTIVECAMERA_H
#define LIMONENGINE_PERSPECTIVECAMERA_H


#include <vector>
#include <algorithm>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "limonAPI/Options.h"
#include "Utils/GLMConverter.h"
#include "Utils/GLMUtils.h"
#include "limonAPI/CameraAttachment.h"
#include "Camera.h"
#include "PhysicalRenderable.h"


class PerspectiveCamera : public Camera {
    glm::mat4 cameraTransformMatrix;
    glm::mat4 perspectiveProjectionMatrix;
    std::vector<glm::mat4>cascadePerspectiveProjectionMatrices;
    std::vector<glm::vec4>frustumPlanes;
    std::vector<std::vector<glm::vec4>>frustumCorners;
    glm::quat view;
    const glm::vec3 startPosition = glm::vec3(0, 10, 15);
    glm::vec3 position, center, up, right;
    CameraAttachment* cameraAttachment;
    float aspect;
    CameraAttachment::ProjectionParameters projectionParameters;
    std::vector<float> cascadeLimits;

    OptionsUtil::Options *options;
    bool dirty = true;

    // Builds the main + per-cascade projection matrices from the current projectionParameters.
    void rebuildProjection() {
        const float invAspect = 1.0f / aspect; // width/height
        perspectiveProjectionMatrix = glm::perspective(projectionParameters.verticalFieldOfView, invAspect,
                                                       projectionParameters.nearPlane, projectionParameters.farPlane);
        cascadePerspectiveProjectionMatrices.clear();
        for (size_t i = 1; i < cascadeLimits.size(); ++i) {
            cascadePerspectiveProjectionMatrices.emplace_back(glm::perspective(projectionParameters.verticalFieldOfView, invAspect,
                                                                              cascadeLimits[i - 1], cascadeLimits[i]));
        }
        frustumCorners.resize(cascadePerspectiveProjectionMatrices.size());
    }

public:

    PerspectiveCamera(const std::string& name, OptionsUtil::Options *options, CameraAttachment* cameraAttachment) :
            view(glm::quat(0, 0, 0, -1)),
            position(startPosition),
            center(glm::vec3(0, 0, -1)),
            up(glm::vec3(0, 1, 0)),
            right(glm::vec3(-1, 0, 0)),
            cameraAttachment(cameraAttachment),
            options(options){
        this->name = name;
        cameraTransformMatrix = glm::lookAt(position, position + center, up);//view matrix
        aspect = float(options->getScreenHeight()) / float(options->getScreenWidth());
        this->frustumPlanes.resize(6);

        cascadeLimits = Camera::readCascadeLimits(options);
        projectionParameters = cameraAttachment->getProjection();
        rebuildProjection();
    }

    void setCameraAttachment(CameraAttachment *cameraAttachment) override {
        PerspectiveCamera::cameraAttachment = cameraAttachment;
        // A new attachment may declare different projection parameters (fov/near/far).
        projectionParameters = cameraAttachment->getProjection();
        rebuildProjection();
    }

    const glm::mat4& getCameraMatrix() override {
        if (cameraAttachment->isDirty()) {
            this->dirty = true;
            cameraAttachment->getCameraVariables(position, center, up, right);
            this->cameraTransformMatrix = glm::lookAt(position, position + center, up);
            calculateFrustumPlanes(cameraTransformMatrix, perspectiveProjectionMatrix, frustumPlanes);
            for (size_t i = 0; i < cascadePerspectiveProjectionMatrices.size(); ++i) {
                calculateFrustumCorners(cameraTransformMatrix, cascadePerspectiveProjectionMatrices[i], frustumCorners[i]);
            }
            cameraAttachment->clearDirty();
        }
        return cameraTransformMatrix;
    }

    const glm::mat4& getCameraMatrixConst() const override {
        return cameraTransformMatrix;
    }

    const glm::mat4& getProjectionMatrix() const override {
        return perspectiveProjectionMatrix;
    }

    bool isDirty() const override {
        return this->dirty || cameraAttachment->isDirty();
    }

    void clearDirty() override {
        this->dirty = false;
    }

    const glm::vec3& getPosition() const override {
        return position;
    }

    const glm::vec3& getCenter() const override {
        return center;
    }

    const glm::vec3 &getUp() const override {
        return up;
    }

    CameraTypes getType() const override {
        return CameraTypes::PERSPECTIVE;
    }

    bool isVisible(const PhysicalRenderable& renderable) const override {
        glm::vec3 aabbMin = renderable.getAabbMin();
        glm::vec3 aabbMax = renderable.getAabbMax();
        return this->isVisible(aabbMin, aabbMax);
    }

    bool isVisible(const glm::vec3& aabbMin, const glm::vec3& aabbMax) const override {
        bool inside = true;
        //test all 6 frustum planes
        for (int i = 0; i<6; i++) {
            //pick closest point to plane and check if it behind the plane
            //if yes - object outside frustum
            float d =   std::fmax(aabbMin.x * frustumPlanes[i].x, aabbMax.x * frustumPlanes[i].x)
                        + std::fmax(aabbMin.y * frustumPlanes[i].y, aabbMax.y * frustumPlanes[i].y)
                        + std::fmax(aabbMin.z * frustumPlanes[i].z, aabbMax.z * frustumPlanes[i].z)
                        + frustumPlanes[i].w;
            inside &= d > 0;
            //return false; //with flag works faster
        }
        return inside;
    }

    bool isVisible(const glm::vec3 &position, float radius) const override {
        {
            // Check if the radius of the sphere is inside the view frustum.
            for(int i=0; i<6; i++) {
                if(((frustumPlanes[i][0] * position.x) + (frustumPlanes[i][1] * position.y) + (frustumPlanes[i][2] * position.z) + frustumPlanes[i][3]) < -radius){
                    return false;
                }
            }
            return true;
        }
    }

    const std::vector<std::vector<glm::vec4>>& getFrustumCorners() const override {
        return frustumCorners;
    }

};


#endif //LIMONENGINE_PERSPECTIVECAMERA_H
