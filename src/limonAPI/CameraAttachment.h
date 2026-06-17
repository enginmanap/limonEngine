//
// Created by engin on 15.02.2018.
//

#ifndef LIMONENGINE_CAMERAATTACHMENT_H
#define LIMONENGINE_CAMERAATTACHMENT_H


#include <glm/vec3.hpp>

class CameraAttachment {
public:
    enum class ProjectionType { PERSPECTIVE, ORTHOGRAPHIC };

    /**
     * Describes the projection the engine should build for the camera this attachment drives.
     * The view pose still comes from getCameraVariables(); this only controls the projection.
     * Aspect ratio is supplied by the engine (from screen dimensions), so it is not stored here.
     *
     * Defaults describe a standard perspective camera; an orthographic attachment overrides
     * `type` and `orthographicHalfHeight`.
     */
    struct ProjectionParameters {
        ProjectionType type = ProjectionType::PERSPECTIVE;
        float verticalFieldOfView   = 1.0471975512f; ///< radians, PERSPECTIVE only (PI/3)
        float orthographicHalfHeight = 50.0f;         ///< world units, ORTHOGRAPHIC only (half-width = halfHeight * aspect)
        float nearPlane = 0.01f;
        float farPlane  = 10000.0f;

        bool operator==(const ProjectionParameters& other) const {
            return type == other.type
                && verticalFieldOfView == other.verticalFieldOfView
                && orthographicHalfHeight == other.orthographicHalfHeight
                && nearPlane == other.nearPlane
                && farPlane == other.farPlane;
        }
        bool operator!=(const ProjectionParameters& other) const { return !(*this == other); }
    };

    virtual bool isDirty() const = 0;

    virtual void clearDirty() = 0;
    virtual void getCameraVariables(glm::vec3 &position, glm::vec3 &center, glm::vec3 &up, glm::vec3 &right) = 0;

    /**
     * The projection the engine should use for the camera driven by this attachment.
     * Required: every attachment declares its projection explicitly.
     */
    virtual ProjectionParameters getProjection() const = 0;

    virtual ~CameraAttachment() = default;
};


#endif //LIMONENGINE_CAMERAATTACHMENT_H
