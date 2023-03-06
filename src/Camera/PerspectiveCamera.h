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

#include "Options.h"
#include "Utils/GLMConverter.h"
#include "Utils/GLMUtils.h"
#include "CameraAttachment.h"
#include "Camera.h"


class PerspectiveCamera : public Camera {
    glm::mat4 cameraTransformMatrix;
    glm::mat4 perspectiveProjectionMatrix;
    std::vector<glm::vec4>frustumPlanes;
    glm::quat view;
    const glm::vec3 startPosition = glm::vec3(0, 10, 15);
    glm::vec3 position, center, up, right;
    CameraAttachment* cameraAttachment;
    float aspect;

    Options *options;

public:

    PerspectiveCamera(const std::string& name, Options *options, CameraAttachment* cameraAttachment) :
            view(glm::quat(0, 0, 0, -1)),
            position(startPosition),
            center(glm::vec3(0, 0, -1)),
            up(glm::vec3(0, 1, 0)),
            right(glm::vec3(-1, 0, 0)),
            cameraAttachment(cameraAttachment),
            options(options){
        this->name = name;
        cameraTransformMatrix = glm::lookAt(position, position + center, up);
        aspect = float(options->getScreenHeight()) / float(options->getScreenWidth());
        this->frustumPlanes.resize(6);
        perspectiveProjectionMatrix = glm::perspective(options->PI/3.0f, 1.0f / aspect, 0.01f, 10000.0f);

    }

    void setCameraAttachment(CameraAttachment *cameraAttachment) {
        PerspectiveCamera::cameraAttachment = cameraAttachment;
    }

    glm::mat4 getCameraMatrix() override {
        if (cameraAttachment->isDirty()) {
            cameraAttachment->getCameraVariables(position, center, up, right);
            this->cameraTransformMatrix = glm::lookAt(position, position + center, up);
            calculateFrustumPlanes(cameraTransformMatrix, perspectiveProjectionMatrix, frustumPlanes);
            cameraAttachment->clearDirty();
        }
        return cameraTransformMatrix;
    }

    bool isDirty() const override {
        return cameraAttachment->isDirty();
    }

    glm::vec3 const& getPosition() const {
        return position;
    }

    glm::vec3 const& getCenter() const {
        return center;
    }

    const glm::vec3 &getUp() const {
        return up;
    }

    CameraTypes getType() const override {
        return CameraTypes::PERSPECTIVE;
    }

    bool isVisible(const PhysicalRenderable& renderable) const override {
        glm::vec3 aabbMin = renderable.getAabbMin();
        glm::vec3 aabbMax = renderable.getAabbMax();
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
};


#endif //LIMONENGINE_PERSPECTIVECAMERA_H
