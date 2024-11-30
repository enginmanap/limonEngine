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
    std::vector<glm::vec4> frustumPlanes;
    std::vector<glm::vec4>  frustumCorners;
    uint32_t cascadeIndex;
    OptionsUtil::Options *options;
    OptionsUtil::Options::Option<double> lightOrthogonalProjectionZBottom;
    uint32_t drawLineBufferId = 0;
    uint32_t playerDrawLineBufferId = 0;
    long frameCounter = 0;
    mutable bool dirty = true;
    OptionsUtil::Options::Option<bool> debugDrawLinesOption = options->getOption<bool>(HASH("DebugDrawLines"));

public:

    OrthographicCamera(const std::string &name, OptionsUtil::Options *options, uint32_t cascadeIndex, CameraAttachment *cameraAttachment) :
            cameraAttachment(cameraAttachment),
            position(0,20,0),
            center(glm::vec3(0, 0, -1)),
            up(glm::vec3(0, 1, 0)),
            right(glm::vec3(-1, 0, 0)),
            cascadeIndex(cascadeIndex),
            options(options),
            lightOrthogonalProjectionZBottom(options->getOption<double>(HASH("lightOrthogonalProjectionBackOff"))){
        this->name = name;

        frustumPlanes.resize(6);
        this->frustumCorners.resize(8);
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

    bool isVisible(const glm::vec3 &position, float radius) const override {
        // Check if the radius of the sphere is inside the view frustum.
        for(int i=0; i<6; i++) {
            if(((frustumPlanes[i][0] * position.x) + (frustumPlanes[i][1] * position.y) + (frustumPlanes[i][2] * position.z) + frustumPlanes[i][3]) < -radius){
                return false;
            }
        }
        return true;
    }

    const glm::mat4 &getCameraMatrix() override {
        if (cameraAttachment->isDirty()) {
            this->dirty = true;
            glm::vec3 tempCenter;
            cameraAttachment->getCameraVariables(position, tempCenter, up, right);
        }
        return cameraTransformMatrix;
    }

    const glm::mat4 &getCameraMatrixConst() const override {
        return cameraTransformMatrix;
    }

    const glm::mat4& getProjectionMatrix() const override {
        return orthogonalProjectionMatrix;
    }

    const glm::mat4& getOrthogonalCameraMatrix()  {
        return cameraTransformMatrix;
    }

    void recalculateView(const PerspectiveCamera* playerCamera) noexcept {
        const std::vector<std::vector<glm::vec4>>& playerFrustumCorners = playerCamera->getFrustumCorners();
        glm::vec3 firstLine = (-1.0f * position) + center;
        glm::mat4 lightView = glm::lookAt(firstLine,
                                          center,
                                          glm::vec3(0.0f, 1.0f, 0.0f));
        this->orthogonalProjectionMatrix = calculateOrthogonalForCascade(playerFrustumCorners[cascadeIndex], lightView, center);
        calculateFrustumPlanes(lightView, this->orthogonalProjectionMatrix, this->frustumPlanes);
        calculateFrustumCorners(lightView,this->orthogonalProjectionMatrix,this->frustumCorners);
        cameraTransformMatrix = orthogonalProjectionMatrix * lightView;


        bool debugDrawLines = debugDrawLinesOption.getOrDefault(false);
        if(debugDrawLines) {
            glm::vec3 color;
            if(cascadeIndex == 0) {
                color = glm::vec3(255, 0, 0);
            } else if(cascadeIndex == 1) {
                color = glm::vec3(255, 255, 0);
            } else if(cascadeIndex == 2) {
                color = glm::vec3(0, 0, 255);
            } else if(cascadeIndex == 3) {
                color = glm::vec3(255, 255, 255);
            }
            debugDrawFrustum(playerFrustumCorners[cascadeIndex], color, playerDrawLineBufferId, frameCounter);
            debugDrawFrustum(frustumCorners, color, drawLineBufferId, frameCounter);

            frameCounter++;
        }

    }

    void debugDrawFrustum(const std::vector<glm::vec4> &frustumCorners, const glm::vec3 &color, uint32_t &drawLineBufferId, long frameCounter) {

        if(frameCounter % 60 == 0) {
            if(drawLineBufferId != 0 ) {
                options->getLogger()->clearLineBuffer(drawLineBufferId);
            }

            //std::cout << "creating lines" << std::endl;
            drawLineBufferId = options->getLogger()->drawLine(frustumCorners[0], frustumCorners[2], color, color, true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[2], frustumCorners[6], color, color, true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[4], frustumCorners[6], color, color, true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[4], frustumCorners[0], color, color, true);

            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[1], frustumCorners[3], color, color, true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[3], frustumCorners[7], color, color, true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[5], frustumCorners[7], color, color, true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[5], frustumCorners[1], color, color, true);

            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[1], frustumCorners[3], color, color, true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[0], frustumCorners[1], color, color, true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[2], frustumCorners[3], color, color, true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[4], frustumCorners[5], color, color, true);
            options->getLogger()->drawLine(drawLineBufferId, frustumCorners[6], frustumCorners[7], color, color, true);

        }
    }

    glm::mat4 calculateOrthogonalForCascade(const std::vector<glm::vec4> &playerFrustumCascadeCorners, const glm::mat4 &lightViewMatrix, glm::vec3& centerToUse) const {
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
        float maxZ = std::numeric_limits<float>::lowest();
        for (const auto& corner : playerFrustumCascadeCorners) {
            const auto trf = lightViewMatrix * corner;
            minX = std::min(minX, trf.x);
            maxX = std::max(maxX, trf.x);
            minY = std::min(minY, trf.y);
            maxY = std::max(maxY, trf.y);
            maxZ = std::max(maxZ, trf.z);
        }

        constexpr float zMultiplier = 7.0f;
        if (maxZ < 0) {
            maxZ /= zMultiplier;
        } else {
            maxZ *= zMultiplier;
        }
        return glm::ortho(minX, maxX, minY, maxY, (float)lightOrthogonalProjectionZBottom.getOrDefault(-5000.0f), maxZ);
    }
};


#endif //LIMONENGINE_ORTHOGRAPHICCAMERA_H
