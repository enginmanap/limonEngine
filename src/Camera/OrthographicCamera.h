//
// Created by engin on 12/02/2023.
//

#ifndef LIMONENGINE_ORTHOGRAPHICCAMERA_H
#define LIMONENGINE_ORTHOGRAPHICCAMERA_H

#include <cmath>
#include "Camera.h"
#include "PhysicalRenderable.h"
#include "limonAPI/CameraAttachment.h"
#include "PerspectiveCamera.h"

class OrthographicCamera : public Camera {
    CameraAttachment* cameraAttachment;
    glm::vec3 position, center, up, right;
    glm::mat4 cameraMatrix;
    glm::mat4 orthogonalProjectionMatrix;
    glm::mat4 cameraProjectionMatrix;
    std::vector<glm::vec4> frustumPlanes;
    std::vector<glm::vec4>  frustumCorners;
    uint32_t cascadeIndex;
    OptionsUtil::Options *options;
    OptionsUtil::Options::Option<double> lightOrthogonalProjectionZBottom;
    uint32_t drawLineBufferId = 0;
    uint32_t playerDrawLineBufferId = 0;
    long frameCounter = 0;
    mutable bool dirty = true;
    OptionsUtil::Options::Option<bool> debugDrawLinesOption = options->getOption<bool>(HASH("debug_drawLines"));

    // Behaviour changes based on whether it is a player camera or a shadow camera, so we have a flag
    bool playerMode = false;
    float aspect = 1.0f;
    CameraAttachment::ProjectionParameters projectionParameters;
    std::vector<float> cascadeLimits;
    std::vector<glm::mat4> cascadeProjectionMatrices;
    std::vector<std::vector<glm::vec4>> playerFrustumCorners;

    // Builds the main + per-cascade orthographic projections from projectionParameters (player role only).
    void rebuildPlayerProjection() {
        const float invAspect = 1.0f / aspect; // width/height
        const float halfHeight = projectionParameters.orthographicHalfHeight;
        const float halfWidth  = halfHeight * invAspect;
        orthogonalProjectionMatrix = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight,
                                                projectionParameters.nearPlane, projectionParameters.farPlane);
        cascadeProjectionMatrices.clear();
        for (size_t i = 1; i < cascadeLimits.size(); ++i) {
            cascadeProjectionMatrices.emplace_back(glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight,
                                                              cascadeLimits[i - 1], cascadeLimits[i]));
        }
        playerFrustumCorners.resize(cascadeProjectionMatrices.size());
    }

public:

    // Directional-shadow constructor: one camera per cascade, view/projection fit to the player frustum.
    OrthographicCamera(const std::string &name, OptionsUtil::Options *options, uint32_t cascadeIndex, CameraAttachment *cameraAttachment) :
            cameraAttachment(cameraAttachment),
            position(0,20,0),
            center(glm::vec3(0, 0, -1)),
            up(glm::vec3(0, 1, 0)),
            right(glm::vec3(-1, 0, 0)),
            cascadeIndex(cascadeIndex),
            options(options),
            lightOrthogonalProjectionZBottom(options->getOption<double>(HASH("shadow_directionalProjectionBackOff"))){
        this->name = name;

        frustumPlanes.resize(6);
        this->frustumCorners.resize(8);
    }

    // Player-camera constructor: an orthographic player view driven by the attachment's projection.
    OrthographicCamera(const std::string &name, OptionsUtil::Options *options, CameraAttachment *cameraAttachment) :
            cameraAttachment(cameraAttachment),
            position(0,20,0),
            center(glm::vec3(0, 0, -1)),
            up(glm::vec3(0, 1, 0)),
            right(glm::vec3(-1, 0, 0)),
            cascadeIndex(0),
            options(options),
            lightOrthogonalProjectionZBottom(options->getOption<double>(HASH("shadow_directionalProjectionBackOff"))){
        this->name = name;
        this->playerMode = true;
        frustumPlanes.resize(6);
        aspect = float(options->getScreenHeight()) / float(options->getScreenWidth());
        cascadeLimits = Camera::readCascadeLimits(options);
        projectionParameters = cameraAttachment->getProjection();
        rebuildPlayerProjection();
        cameraMatrix = glm::lookAt(position, position + center, up);
    }

    CameraTypes getType() const override {
        return CameraTypes::ORTHOGRAPHIC;
    };

    bool isDirty() const override {
        if (playerMode) {
            // As a player camera, attachment movement must propagate so the frame re-renders.
            return this->dirty || cameraAttachment->isDirty();
        }
        return this->dirty;// || cameraAttachment->isDirty(); this was here to allow clearDirty from culling threads. Now it is cleared by rendering.
    };

    void clearDirty() override {
        this->dirty = false;
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
            if (playerMode) {
                cameraAttachment->getCameraVariables(position, center, up, right);
                cameraMatrix = glm::lookAt(position, position + center, up);
                calculateFrustumPlanes(cameraMatrix, orthogonalProjectionMatrix, frustumPlanes);
                for (size_t i = 0; i < cascadeProjectionMatrices.size(); ++i) {
                    calculateFrustumCorners(cameraMatrix, cascadeProjectionMatrices[i], playerFrustumCorners[i]);
                }
                cameraAttachment->clearDirty();
            } else {
                glm::vec3 tempCenter;
                cameraAttachment->getCameraVariables(position, tempCenter, up, right);
            }
        }
        return cameraMatrix;
    }

    // Player-role cascade corners for CSM fitting (empty in the shadow role).
    const std::vector<std::vector<glm::vec4>>& getFrustumCorners() const override {
        return playerFrustumCorners;
    }

    const glm::mat4 &getCameraMatrixConst() const override {
        return cameraMatrix;
    }

    const glm::mat4& getProjectionMatrix() const override {
        return orthogonalProjectionMatrix;
    }

    const glm::mat4& getOrthogonalCameraMatrix()  {
        return cameraProjectionMatrix;
    }

    const glm::vec3& getPosition() const override {
        return position;
    }

    const glm::vec3& getCenter() const override {
        return center;
    }

    const glm::vec3& getUp() const override {
        return up;
    }

    void setCameraAttachment(CameraAttachment* cameraAttachment) override {
        this->cameraAttachment = cameraAttachment;
        if (playerMode) {
            projectionParameters = cameraAttachment->getProjection();
            rebuildPlayerProjection();
        }
    }

    void recalculateView(const Camera* playerCamera) noexcept {
        const std::vector<std::vector<glm::vec4>>& playerFrustumCorners = playerCamera->getFrustumCorners();
        // Projection-agnostic: any player camera that supplies cascade corners works here. A camera
        // that doesn't populate this cascade index yet (e.g. an orthographic camera before its
        // cascade generation lands) is simply skipped rather than special-cased by type.
        if(cascadeIndex >= playerFrustumCorners.size()) {
            return;
        }
        glm::vec3 firstLine = (-1.0f * position) + center;
        glm::mat4 lightView = glm::lookAt(firstLine,
                                          center,
                                          glm::vec3(0.0f, 1.0f, 0.0f));
        this->orthogonalProjectionMatrix = calculateOrthogonalForCascade(playerFrustumCorners[cascadeIndex], lightView, center);
        this->cameraMatrix = lightView;
        this->cameraProjectionMatrix = orthogonalProjectionMatrix * lightView;
        calculateFrustumPlanes(lightView, this->orthogonalProjectionMatrix, this->frustumPlanes);
        calculateFrustumCorners(lightView,this->orthogonalProjectionMatrix,this->frustumCorners);


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

    glm::mat4 calculateOrthogonalForCascade(const std::vector<glm::vec4> &playerFrustumCascadeCorners, glm::mat4 &lightViewMatrix, glm::vec3& centerToUse) const {
        centerToUse = glm::vec3(0, 0, 0);
        for (const auto& corner : playerFrustumCascadeCorners)
        {
            centerToUse += glm::vec3(corner);
        }
        centerToUse /= playerFrustumCascadeCorners.size();

        float radius = 0.0f;
        for (const auto& corner : playerFrustumCascadeCorners) {
            float distance = glm::length(glm::vec3(corner) - centerToUse);
            radius = std::max(radius, distance);
        }
        radius = std::ceil(radius * 16.0f) / 16.0f;

        lightViewMatrix = glm::lookAt(centerToUse,
                                      position + centerToUse,
                                      glm::vec3(0.0f, 1.0f, 0.0f));

        // We are switching to square shadow map, because I could not figure out a way to rotate light rendering and not cause shadow crawling.
        // If I find a way, we can switch back to rectangle shadow map.
        long shadowMapSize = options->getOption<long>(HASH("shadow_mapDirectionalSize")).getOrDefault(2048);
        float texelSize = (2.0f * radius) / (float)shadowMapSize;

        glm::vec3 viewTranslation = glm::vec3(lightViewMatrix[3]);
        glm::vec2 viewTranslationSnapped = glm::vec2(
            std::floor(viewTranslation.x / texelSize) * texelSize,
            std::floor(viewTranslation.y / texelSize) * texelSize
        );
        glm::vec2 snapOffset = viewTranslationSnapped - glm::vec2(viewTranslation.x, viewTranslation.y);

        lightViewMatrix[3][0] += snapOffset.x;
        lightViewMatrix[3][1] += snapOffset.y;

        float minX = -radius;
        float maxX = radius;
        float minY = -radius;
        float maxY = radius;

        float minZ = std::numeric_limits<float>::max();
        for (const auto& corner : playerFrustumCascadeCorners) {
            const auto trf = lightViewMatrix * corner;
            minZ = std::min(minZ, trf.z);
        }

        //because opengl assumes camera looks in -z direction, last 2 parameters are suppose to be -maxZ and -minZ
        return glm::ortho(minX, maxX, minY, maxY,  (float)lightOrthogonalProjectionZBottom.getOrDefault(-5000.0f), -minZ);
    }
};


#endif //LIMONENGINE_ORTHOGRAPHICCAMERA_H
