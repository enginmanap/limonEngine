//
// Created by engin on 12/02/2023.
//

#ifndef LIMONENGINE_ORTHOGRAPHICCAMERA_H
#define LIMONENGINE_ORTHOGRAPHICCAMERA_H


#include "Camera.h"
#include "PhysicalRenderable.h"
#include "CameraAttachment.h"
#include "PerspectiveCamera.h"

class OrthographicCamera : public Camera {
    CameraAttachment* cameraAttachment;
    glm::vec3 position, center, up, right;
    glm::mat4 cameraTransformMatrix;
    glm::mat4 orthogonalProjectionMatrix;
    std::vector<glm::vec4>frustumPlanes;
    std::vector<std::vector<glm::vec4>> frustumCorners;
    float backOffFactor; // how high the camera should be? we are selecting average of zfar and znear, and add that to player y
    Options *options;

    mutable bool dirty = true;


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
        this->frustumCorners.resize(4);
        LimonTypes::Vec4 lightOrthogonalValues;
        float lightOrthogonalNear, lightOrthogonalFar;
        options->getOption("lightOrthogonalProjectionValues", lightOrthogonalValues);
        options->getOption("lightOrthogonalProjectionNearPlane", lightOrthogonalNear);
        options->getOption("lightOrthogonalProjectionFarPlane", lightOrthogonalFar);
        backOffFactor = (lightOrthogonalFar - lightOrthogonalNear) / 2.0;
        orthogonalProjectionMatrix = glm::ortho(lightOrthogonalValues.x,
                                                lightOrthogonalValues.y,
                                                lightOrthogonalValues.z,
                                                lightOrthogonalValues.w,
                                                lightOrthogonalNear,
                                                lightOrthogonalFar);
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

    bool isResultVisibleOnOtherCamera(const PhysicalRenderable& renderable[[gnu::unused]], const Camera* otherCamera[[gnu::unused]]) const {
        return true;
    }

    glm::mat4 getCameraMatrix() override {
        if (cameraAttachment->isDirty()) {
            this->dirty = true;
            glm::vec3 tempCenter;
            cameraAttachment->getCameraVariables(position, tempCenter, up, right);
            cameraAttachment->clearDirty();
        }
        return cameraTransformMatrix;
    }

    glm::mat4 getProjectionMatrix() override {
        return orthogonalProjectionMatrix;
    }

    void recalculateView(const PerspectiveCamera* playerCamera) noexcept {
        const std::vector<std::vector<glm::vec4>>& playerFrustumCorners = playerCamera->getFrustumCorners();
        glm::mat4 lightView = glm::lookAt((-1.0f * position) + center,
                                          center,
                                          glm::vec3(0.0f, 1.0f, 0.0f));

        this->orthogonalProjectionMatrix = calculateOrthogonalForCascade(playerFrustumCorners[0], lightView);

        cameraTransformMatrix = orthogonalProjectionMatrix * lightView;
        calculateFrustumPlanes(lightView, orthogonalProjectionMatrix, this->frustumPlanes);
        calculateFrustumCorners(lightView,orthogonalProjectionMatrix,this->frustumCorners[0]);
        long debugDrawLines = 0;
        options->getOptionOrDefault("DebugDrawLines", debugDrawLines, 0);
        if(debugDrawLines) {
            debugDrawFrustum(frustumCorners[0]);
        }

    }

    void debugDrawFrustum(std::vector<glm::vec4> frustumCornersThis) {
        static long frameCounter = 0;
        static uint32_t drawLineBufferId = 0;
        frameCounter++;
        if(frameCounter % 300 == 0) {
            if(drawLineBufferId != 0 ) {
                options->getLogger()->clearLineBuffer(drawLineBufferId);
            }

            //std::cout << "creating lines" << std::endl;
             drawLineBufferId = options->getLogger()->drawLine(frustumCornersThis[0], frustumCornersThis[2], glm::vec3(255, 0, 0), glm::vec3(255, 0, 0), true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCornersThis[2], frustumCornersThis[4], glm::vec3(255, 0, 0), glm::vec3(255, 0, 0), true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCornersThis[4], frustumCornersThis[6], glm::vec3(255, 0, 0), glm::vec3(255, 0, 0), true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCornersThis[6], frustumCornersThis[0], glm::vec3(255, 0, 0), glm::vec3(255, 0, 0), true);

            options->getLogger()->drawLine(drawLineBufferId, frustumCornersThis[1], frustumCornersThis[3], glm::vec3(255, 0, 0), glm::vec3(255, 0, 0), true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCornersThis[3], frustumCornersThis[5], glm::vec3(255, 0, 0), glm::vec3(255, 0, 0), true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCornersThis[5], frustumCornersThis[7], glm::vec3(255, 0, 0), glm::vec3(255, 0, 0), true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCornersThis[7], frustumCornersThis[1], glm::vec3(255, 0, 0), glm::vec3(255, 0, 0), true);

            options->getLogger()->drawLine(drawLineBufferId, frustumCornersThis[1], frustumCornersThis[3], glm::vec3(255, 0, 0), glm::vec3(255, 0, 0), true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCornersThis[0], frustumCornersThis[1], glm::vec3(255, 0, 0), glm::vec3(255, 0, 0), true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCornersThis[2], frustumCornersThis[3], glm::vec3(255, 0, 0), glm::vec3(255, 0, 0), true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCornersThis[4], frustumCornersThis[5], glm::vec3(255, 0, 0), glm::vec3(255, 0, 0), true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCornersThis[6], frustumCornersThis[7], glm::vec3(255, 0, 0), glm::vec3(255, 0, 0), true);

        }
    }

    glm::mat4 calculateOrthogonalForCascade(const std::vector<glm::vec4> &playerFrustumCascadeCorners, const glm::mat4 &lightViewMatrix) {
        center = glm::vec3(0, 0, 0);
        for (const auto& corner : playerFrustumCascadeCorners)
        {
            center += glm::vec3(corner);
        }
        center /= playerFrustumCascadeCorners.size();

        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        float minZ = std::numeric_limits<float>::max();
        float maxZ = std::numeric_limits<float>::lowest();
        for (const auto& corner : playerFrustumCascadeCorners) {
            const auto trf = lightViewMatrix * corner;
            minX = std::min(minX, trf.x);
            maxX = std::max(maxX, trf.x);
            minY = std::min(minY, trf.y);
            maxY = std::max(maxY, trf.y);
            minZ = std::min(minZ, trf.z);
            maxZ = std::max(maxZ, trf.z);
        }

        constexpr float zMultiplier = 10000.0f;
        if (minZ < 0) {
            minZ *= zMultiplier;
        } else {
            minZ /= zMultiplier;
        } if (maxZ < 0) {
            maxZ /= zMultiplier;
        } else {
            maxZ *= zMultiplier;
        }

        return glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
    }

    const std::vector<std::vector<glm::vec4>>& getFrustumCorners() const override {
        return frustumCorners;
    }
};


#endif //LIMONENGINE_ORTHOGRAPHICCAMERA_H
