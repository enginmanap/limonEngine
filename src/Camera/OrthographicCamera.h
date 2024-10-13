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
    std::vector<glm::vec4>frustumCorners;
    float backOffFactor; // how high the camera should be? we are selecting average of zfar and znear, and add that to player y
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
        this->frustumCorners.resize(8);
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
            /*
            glm::mat4 lightView = glm::lookAt((-1.0f * position * backOffFactor) + center,
                                              center,
                                              glm::vec3(0.0f, 1.0f, 0.0f));
            glm::vec3 tempCenter;
            cameraAttachment->getCameraVariables(up, tempCenter, position, right);
            this->center = tempCenter;

            glm::mat4 cameraTransformMatrix = orthogonalProjectionMatrix * lightView;
            std::vector<glm::vec4>thisFrustumCorners;
            thisFrustumCorners.resize(6);
            std::vector<glm::vec4>thisFrustumPlanes;
            thisFrustumPlanes.resize(6);
            calculateFrustumPlanes(lightView, orthogonalProjectionMatrix, thisFrustumPlanes, thisFrustumCorners);
            this->cameraTransformMatrix = cameraTransformMatrix;
            */
            cameraAttachment->clearDirty();
        }
        return cameraTransformMatrix;
    }

    glm::mat4 getProjectionMatrix() override {
        return orthogonalProjectionMatrix;
    }

    void recalculateView(const glm::mat4 &cameraViewMatrix, const glm::mat4 &cameraProjectionMatrix[[gnu::unused]]) noexcept {
        // FIXME don't recalc
        std::vector<glm::vec4>playerFrustumCorners;
        playerFrustumCorners.resize(8);
        float aspect = float(options->getScreenHeight()) / float(options->getScreenWidth());
        glm::mat4 perspectiveProjectionMatrix = glm::perspective(Options::PI/3.0f, 1.0f / aspect, 0.01f, 10.0f);

        calculateFrustumCorners(cameraViewMatrix, perspectiveProjectionMatrix, playerFrustumCorners);
        center = glm::vec3(0, 0, 0);
        for (const auto& v : playerFrustumCorners)
        {
            center += glm::vec3(v);
        }
        center /= playerFrustumCorners.size();

        glm::mat4 lightView = glm::lookAt((-1.0f * position) + center,
                                          center,
                                          glm::vec3(0.0f, 1.0f, 0.0f));

        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        float minZ = std::numeric_limits<float>::max();
        float maxZ = std::numeric_limits<float>::lowest();
        for (const auto& v : playerFrustumCorners)
        {
            const auto trf = lightView * v;
            minX = std::min(minX, trf.x);
            maxX = std::max(maxX, trf.x);
            minY = std::min(minY, trf.y);
            maxY = std::max(maxY, trf.y);
            minZ = std::min(minZ, trf.z);
            maxZ = std::max(maxZ, trf.z);
        }

        constexpr float zMult = 1000.0f;
        if (minZ < 0)
        {
            minZ *= zMult;
        }
        else
        {
            minZ /= zMult;
        }
        if (maxZ < 0)
        {
            maxZ /= zMult;
        }
        else
        {
            maxZ *= zMult;
        }

        this->orthogonalProjectionMatrix = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

        cameraTransformMatrix = orthogonalProjectionMatrix * lightView;
        calculateFrustumPlanes(lightView, orthogonalProjectionMatrix, this->frustumPlanes, this->frustumCorners);
        long debugDrawLines = 0;
        options->getOptionOrDefault("DebugDrawLines", debugDrawLines, 0);
        if(debugDrawLines) {
            static long frameCounter = 0;
            static uint32_t drawLineBufferId = 0;
            frameCounter++;
            if(frameCounter % 300) {
                if(drawLineBufferId != 0 ) {
                    options->getLogger()->clearLineBuffer(drawLineBufferId);
                }
                std::cout << "creating lines" << std::endl;
                 drawLineBufferId = options->getLogger()->drawLine(frustumCorners[0], frustumCorners[2], glm::vec3(255,0,0), glm::vec3(255,0,0), true);
                options->getLogger()->drawLine(drawLineBufferId, frustumCorners[2], frustumCorners[4], glm::vec3(255,0,0), glm::vec3(255,0,0), true);
                options->getLogger()->drawLine(drawLineBufferId, frustumCorners[4], frustumCorners[6], glm::vec3(255,0,0), glm::vec3(255,0,0), true);
                options->getLogger()->drawLine(drawLineBufferId, frustumCorners[6], frustumCorners[0], glm::vec3(255,0,0), glm::vec3(255,0,0), true);

                options->getLogger()->drawLine(drawLineBufferId, frustumCorners[1], frustumCorners[3], glm::vec3(255,0,0), glm::vec3(255,0,0), true);
                options->getLogger()->drawLine(drawLineBufferId, frustumCorners[3], frustumCorners[5], glm::vec3(255,0,0), glm::vec3(255,0,0), true);
                options->getLogger()->drawLine(drawLineBufferId, frustumCorners[5], frustumCorners[7], glm::vec3(255,0,0), glm::vec3(255,0,0), true);
                options->getLogger()->drawLine(drawLineBufferId, frustumCorners[7], frustumCorners[1], glm::vec3(255,0,0), glm::vec3(255,0,0), true);

                options->getLogger()->drawLine(drawLineBufferId, frustumCorners[1], frustumCorners[3], glm::vec3(255,0,0), glm::vec3(255,0,0), true);
                options->getLogger()->drawLine(drawLineBufferId, frustumCorners[0], frustumCorners[1], glm::vec3(255,0,0), glm::vec3(255,0,0), true);
                options->getLogger()->drawLine(drawLineBufferId, frustumCorners[2], frustumCorners[3], glm::vec3(255,0,0), glm::vec3(255,0,0), true);
                options->getLogger()->drawLine(drawLineBufferId, frustumCorners[4], frustumCorners[5], glm::vec3(255,0,0), glm::vec3(255,0,0), true);
                options->getLogger()->drawLine(drawLineBufferId, frustumCorners[6], frustumCorners[7], glm::vec3(255,0,0), glm::vec3(255,0,0), true);
            }

        }

    }
};


#endif //LIMONENGINE_ORTHOGRAPHICCAMERA_H
