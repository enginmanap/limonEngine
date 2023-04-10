//
// Created by engin on 12/02/2023.
//

#ifndef LIMONENGINE_ORTHOGRAPHICCAMERA_H
#define LIMONENGINE_ORTHOGRAPHICCAMERA_H


#include "Camera.h"
#include "PhysicalRenderable.h"
#include "CameraAttachment.h"

class OrthographicCamera : public Camera {
    CameraAttachment* cameraAttachment;
    glm::vec3 position, center, up, right;
    glm::mat4 cameraTransformMatrix;
    glm::mat4 orthogonalProjectionMatrix;
    std::vector<glm::vec4>frustumPlanes;
    Options *options;

    bool dirty = true;


public:

    OrthographicCamera(const std::string& name, Options *options, CameraAttachment* cameraAttachment) :
            cameraAttachment(cameraAttachment),
            position(0,20,0),
            center(glm::vec3(0, 0, -1)),
            up(glm::vec3(0, 1, 0)),
            right(glm::vec3(-1, 0, 0)),
            options(options){
        this->name = name;
        this->frustumPlanes.resize(6);
        orthogonalProjectionMatrix = glm::ortho(options->getLightOrthogonalProjectionValues().x,
                                                      options->getLightOrthogonalProjectionValues().y,
                                                      options->getLightOrthogonalProjectionValues().z,
                                                      options->getLightOrthogonalProjectionValues().w,
                                                      options->getLightOrthogonalProjectionNearPlane(),
                                                      options->getLightOrthogonalProjectionFarPlane());
    }

    CameraTypes getType() const override {
        return CameraTypes::ORTHOGRAPHIC;
    };

    bool isDirty() const override {
        return this->dirty || cameraAttachment->isDirty();
    };

    void clearDirty() override {
        this->dirty = false;
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
    };

    glm::mat4 getCameraMatrix() override {
        if (cameraAttachment->isDirty()) {
            this->dirty = true;
            cameraAttachment->getCameraVariables(center, position, up, right);
            glm::mat4 lightView = glm::lookAt(this->position,
                                              glm::vec3(0.0f, 0.0f, 0.0f),
                                              glm::vec3(0.0f, 1.0f, 0.0f));

            cameraTransformMatrix = orthogonalProjectionMatrix * lightView;
            calculateFrustumPlanes(lightView, orthogonalProjectionMatrix, this->frustumPlanes);
            cameraAttachment->clearDirty();
        }
        return cameraTransformMatrix;
    }
};


#endif //LIMONENGINE_ORTHOGRAPHICCAMERA_H
