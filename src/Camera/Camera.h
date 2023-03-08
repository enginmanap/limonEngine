//
// Created by engin on 09/02/2023.
//

#ifndef LIMONENGINE_CAMERA_H
#define LIMONENGINE_CAMERA_H

#include "Renderable.h"

class Camera {
protected:
    std::string name;

    static void calculateFrustumPlanes(const glm::mat4 &cameraMatrix,
                                       const glm::mat4 &projectionMatrix, std::vector<glm::vec4> &planes) {
        assert(planes.size() == 6);
        glm::mat4 clipMat;

        for(int i = 0; i < 4; i++) {
            glm::vec4 cameraRow = cameraMatrix[i];
            clipMat[i].x =  cameraRow.x * projectionMatrix[0].x + cameraRow.y * projectionMatrix[1].x +
                            cameraRow.z * projectionMatrix[2].x + cameraRow.w * projectionMatrix[3].x;
            clipMat[i].y =  cameraRow.x * projectionMatrix[0].y + cameraRow.y * projectionMatrix[1].y +
                            cameraRow.z * projectionMatrix[2].y + cameraRow.w * projectionMatrix[3].y;
            clipMat[i].z =  cameraRow.x * projectionMatrix[0].z + cameraRow.y * projectionMatrix[1].z +
                            cameraRow.z * projectionMatrix[2].z + cameraRow.w * projectionMatrix[3].z;
            clipMat[i].w =  cameraRow.x * projectionMatrix[0].w + cameraRow.y * projectionMatrix[1].w +
                            cameraRow.z * projectionMatrix[2].w + cameraRow.w * projectionMatrix[3].w;
        }

        planes[RIGHT].x = clipMat[0].w - clipMat[0].x;
        planes[RIGHT].y = clipMat[1].w - clipMat[1].x;
        planes[RIGHT].z = clipMat[2].w - clipMat[2].x;
        planes[RIGHT].w = clipMat[3].w - clipMat[3].x;
        planes[RIGHT] = glm::normalize(planes[RIGHT]);

        planes[LEFT].x = clipMat[0].w + clipMat[0].x;
        planes[LEFT].y = clipMat[1].w + clipMat[1].x;
        planes[LEFT].z = clipMat[2].w + clipMat[2].x;
        planes[LEFT].w = clipMat[3].w + clipMat[3].x;
        planes[LEFT] = glm::normalize(planes[LEFT]);

        planes[BOTTOM].x = clipMat[0].w + clipMat[0].y;
        planes[BOTTOM].y = clipMat[1].w + clipMat[1].y;
        planes[BOTTOM].z = clipMat[2].w + clipMat[2].y;
        planes[BOTTOM].w = clipMat[3].w + clipMat[3].y;
        planes[BOTTOM] = glm::normalize(planes[BOTTOM]);

        planes[TOP].x = clipMat[0].w - clipMat[0].y;
        planes[TOP].y = clipMat[1].w - clipMat[1].y;
        planes[TOP].z = clipMat[2].w - clipMat[2].y;
        planes[TOP].w = clipMat[3].w - clipMat[3].y;
        planes[TOP] = glm::normalize(planes[TOP]);

        planes[BACK].x = clipMat[0].w - clipMat[0].z;
        planes[BACK].y = clipMat[1].w - clipMat[1].z;
        planes[BACK].z = clipMat[2].w - clipMat[2].z;
        planes[BACK].w = clipMat[3].w - clipMat[3].z;
        planes[BACK] = glm::normalize(planes[BACK]);

        planes[FRONT].x = clipMat[0].w + clipMat[0].z;
        planes[FRONT].y = clipMat[1].w + clipMat[1].z;
        planes[FRONT].z = clipMat[2].w + clipMat[2].z;
        planes[FRONT].w = clipMat[3].w + clipMat[3].z;
        planes[FRONT] = glm::normalize(planes[FRONT]);
    }


public:

    enum FrustumSide
    {
        RIGHT	= 0,		// The RIGHT side of the frustum
        LEFT	= 1,		// The LEFT	 side of the frustum
        BOTTOM	= 2,		// The BOTTOM side of the frustum
        TOP		= 3,		// The TOP side of the frustum
        BACK	= 4,		// The BACK	side of the frustum
        FRONT	= 5			// The FRONT side of the frustum
    };

    enum class CameraTypes {PERSPECTIVE, ORTHOGRAPHIC, CUBE, INVALID};

    const std::string& getName() const {
        return name;
    }

    virtual ~Camera() = default;

    virtual CameraTypes getType() const = 0;

    virtual bool isDirty() const = 0;

    virtual void clearDirty() = 0;

    virtual bool isVisible(const PhysicalRenderable& renderable) const = 0;

    virtual glm::mat4 getCameraMatrix() = 0;

};


#endif //LIMONENGINE_CAMERA_H
