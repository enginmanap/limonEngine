//
// Created by engin on 12/02/2023.
//

#ifndef LIMONENGINE_CUBECAMERA_H
#define LIMONENGINE_CUBECAMERA_H


#include "Camera.h"
#include "PhysicalRenderable.h"
#include "limonAPI/CameraAttachment.h"

class CubeCamera : public Camera {
    std::vector<glm::mat4> renderMatrices;//these are used only for point lights for now
    CameraAttachment* cameraAttachment;
    glm::vec3 position, center, up, right;
    OptionsUtil::Options *options;
    OptionsUtil::Options::Option<long> shadowMapWidthOption;
    OptionsUtil::Options::Option<long> shadowMapHeightOption;
    bool dirty = true;
    float activeDistance = 0.0f;//if we don't set it, it would read as garbage untill camera owner sets it
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
        shadowMapWidthOption = options->getOption<long>(HASH("shadow_mapPointWidth"));
        shadowMapHeightOption = options->getOption<long>(HASH("shadow_mapPointHeight"));
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

    bool isVisible(const glm::vec3& aabbMin, const glm::vec3& aabbMax) const override {
        float radius = std::max((aabbMax.x - aabbMin.x) * 0.5f,
            std::max((aabbMax.y - aabbMin.y) * 0.5f, (aabbMax.z - aabbMin.z) * 0.5f));
        glm::vec3 sphereCenter = glm::vec3((aabbMax.x + aabbMin.x) * 0.5f, (aabbMax.y + aabbMin.y) * 0.5f, (aabbMax.z + aabbMin.z) * 0.5f);
        float distSquared = glm::distance2(sphereCenter, this->position);
        return distSquared < (activeDistance+radius) * (activeDistance+radius);
    }

    bool isVisible(const glm::vec3 &position, float radius) const override {
        return glm::distance2(this->position, position) < (activeDistance+radius) * (activeDistance+radius);
    }

    const glm::mat4& getCameraMatrix() override {
        if(isDirty()) {
            cameraAttachment->getCameraVariables(position,center,up,right);
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
    }

    float getPixelSize() const override {
        return 2.0f / (float)shadowMapHeightOption.getOrDefault(512);
    }

    bool isPixelGrowingWithDepth() const override {
        return true;
    }

    float getDepthTo(const glm::vec3& aabbMin, const glm::vec3& aabbMax) const override {
        //the face that sees a point looks down the axis the point is farthest along, so that is its depth there
        const glm::vec3 gap = getGapTo(aabbMin, aabbMax);
        return std::max(gap.x, std::max(gap.y, gap.z));
    }

    float getSmallestShadowScale(const glm::vec3& aabbMin, const glm::vec3& aabbMax) const override {
        //drawn biggest at the end of the range. A light inside the box gives 0
        const float boxToLight = glm::length(getGapTo(aabbMin, aabbMax));
        const float farthestShadow = std::max(activeDistance, boxToLight);
        return farthestShadow > 0.0f ? boxToLight / farthestShadow : 0.0f;
    }

    float getNearestShadowTo(const glm::vec3& viewer, const glm::vec3& aabbMin, const glm::vec3& aabbMax) const override {
        //the shadow lands in the cone from the light through the box's sphere, between the box and the range
        const float boxToLight = glm::length(getGapTo(aabbMin, aabbMax));
        const float viewerToLight = glm::distance(viewer, position);
        if (boxToLight <= 0.0f || viewerToLight <= 0.0f) {
            return boxToLight;//from the light every shadow is as near as it is magnified
        }
        const glm::vec3 lightToBox = (aabbMin + aabbMax) * 0.5f - position;
        const float centerToLight = glm::length(lightToBox);
        const float radius = glm::length(aabbMax - aabbMin) * 0.5f;
        //cosine of the viewer's angle outside the cone's side. Inside it, or a light inside the sphere, it is 1
        float cosOutside = 1.0f;
        if (centerToLight > radius) {
            const float sinCone = radius / centerToLight;
            const float cosCone = std::sqrt(1.0f - sinCone * sinCone);
            const float cosViewer = glm::dot(lightToBox / centerToLight, (viewer - position) / viewerToLight);
            const float sinViewer = std::sqrt(std::max(0.0f, 1.0f - cosViewer * cosViewer));
            if (sinViewer * cosCone - cosViewer * sinCone > 0.0f) {
                cosOutside = cosViewer * cosCone + sinViewer * sinCone;
            }
        }
        //distance over magnification is boxToLight * sqrt(x^2 - 2x cosOutside + 1), x = viewerToLight / the shadow's distance to the light
        const float viewerOverShadowDistance = glm::clamp(cosOutside, viewerToLight / std::max(activeDistance, boxToLight), viewerToLight / boxToLight);
        return boxToLight * std::sqrt(std::max(0.0f, viewerOverShadowDistance * viewerOverShadowDistance - 2.0f * viewerOverShadowDistance * cosOutside + 1.0f));
    }

    float getActiveDistance() const {
        return activeDistance;
    }

    //Onwer should set it (In current case, point light only
    void setActiveDistance(float activeDistance) {
        this->activeDistance = activeDistance;
        this->dirty = true;
    }

private:
    glm::vec3 getGapTo(const glm::vec3& aabbMin, const glm::vec3& aabbMax) const {
        return glm::max(glm::max(aabbMin - position, position - aabbMax), glm::vec3(0.0f));
    }

    void setShadowMatricesForPosition(){
        long sWidth, sHeight;
        sWidth = shadowMapWidthOption.getOrDefault(512);
        sHeight = shadowMapHeightOption.getOrDefault(512);

        OptionsUtil::Options::Option<double> near = options->getOption<double>(HASH("shadow_pointNearPlane"));
        OptionsUtil::Options::Option<double> far = options->getOption<double>(HASH("shadow_pointFarPlane"));

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
