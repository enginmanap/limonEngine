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
    std::vector<glm::mat4> orthogonalProjectionMatrices;
    std::vector<std::vector<glm::vec4>>frustumPlanes;
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

        long cascadeCount;
        options->getOptionOrDefault("CascadeCount", cascadeCount, 4L);
        this->frustumPlanes.resize(cascadeCount);
        for (int i = 0; i < cascadeCount; ++i) {
            frustumPlanes[i].resize(6);
        }
        this->frustumCorners.resize(cascadeCount);
        this->orthogonalProjectionMatrices.resize(cascadeCount);
        float lightOrthogonalNear, lightOrthogonalFar;
        options->getOption("lightOrthogonalProjectionNearPlane", lightOrthogonalNear);
        options->getOption("lightOrthogonalProjectionFarPlane", lightOrthogonalFar);
        backOffFactor = (lightOrthogonalFar - lightOrthogonalNear) / 2.0;
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
        //test all 6 frustum planes
        for (size_t i = 0; i < frustumPlanes.size(); ++i) {
            for (int j = 0; j < 6; j++) {
                //pick closest point to plane and check if it behind the plane
                //if yes - object outside frustum
                float d =   std::fmax(aabbMin.x * frustumPlanes[i][j].x, aabbMax.x * frustumPlanes[i][j].x)
                            + std::fmax(aabbMin.y * frustumPlanes[i][j].y, aabbMax.y * frustumPlanes[i][j].y)
                            + std::fmax(aabbMin.z * frustumPlanes[i][j].z, aabbMax.z * frustumPlanes[i][j].z)
                            + frustumPlanes[i][j].w;
                if( d > 0) {
                    return true;
                }
            }
        }
        return false;
    };

    bool isResultVisibleOnOtherCamera(const PhysicalRenderable& renderable[[gnu::unused]], const Camera* otherCamera[[gnu::unused]]) const {
        return true;
    }

    const glm::mat4 &getCameraMatrix() override {
        if (cameraAttachment->isDirty()) {
            this->dirty = true;
            glm::vec3 tempCenter;
            cameraAttachment->getCameraVariables(position, tempCenter, up, right);
            cameraAttachment->clearDirty();
        }
        return cameraTransformMatrix;
    }

    const glm::mat4& getProjectionMatrix() override {
        return orthogonalProjectionMatrices[0];
    }

    void recalculateView(const PerspectiveCamera* playerCamera) noexcept {
        const std::vector<std::vector<glm::vec4>>& playerFrustumCorners = playerCamera->getFrustumCorners();
        glm::mat4 lightView = glm::lookAt((-1.0f * position) + center,
                                          center,
                                          glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec3 tempCenter;
        for (size_t i = 0; i < playerFrustumCorners.size(); ++i) {
            this->orthogonalProjectionMatrices[i] = calculateOrthogonalForCascade(playerFrustumCorners[i], lightView, i==0?center:tempCenter);
            calculateFrustumPlanes(lightView, this->orthogonalProjectionMatrices[i], this->frustumPlanes[i]);
            calculateFrustumCorners(lightView,this->orthogonalProjectionMatrices[i],this->frustumCorners[i]);
        }

        cameraTransformMatrix = orthogonalProjectionMatrices[0] * lightView;
        long debugDrawLines = 0;
        options->getOptionOrDefault("DebugDrawLines", debugDrawLines, 0);
        if(debugDrawLines) {
            debugDrawFrustum(frustumCorners[0], glm::vec3(255,255,0));
        }

    }

    void debugDrawFrustum(const std::vector<glm::vec4>& frustumCorners, const glm::vec3& color) {
        static long frameCounter = 0;
        static uint32_t drawLineBufferId = 0;
        frameCounter++;
        if(frameCounter % 300 == 0) {
            if(drawLineBufferId != 0 ) {
                options->getLogger()->clearLineBuffer(drawLineBufferId);
            }

            //std::cout << "creating lines" << std::endl;
             drawLineBufferId = options->getLogger()->drawLine(frustumCorners[0], frustumCorners[2], color, color, true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[2], frustumCorners[4], color, color, true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[4], frustumCorners[6], color, color, true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[6], frustumCorners[0], color, color, true);

            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[1], frustumCorners[3], color, color, true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[3], frustumCorners[5], color, color, true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[5], frustumCorners[7], color, color, true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[7], frustumCorners[1], color, color, true);

            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[1], frustumCorners[3], color, color, true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[0], frustumCorners[1], color, color, true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[2], frustumCorners[3], color, color, true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[4], frustumCorners[5], color, color, true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[6], frustumCorners[7], color, color, true);

        }
    }

    glm::mat4 calculateOrthogonalForCascade(const std::vector<glm::vec4> &playerFrustumCascadeCorners, const glm::mat4 &lightViewMatrix, glm::vec3& centerToUse) {
        centerToUse = glm::vec3(0, 0, 0);
        for (const auto& corner : playerFrustumCascadeCorners)
        {
            centerToUse += glm::vec3(corner);
        }
        centerToUse /= playerFrustumCascadeCorners.size();

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
