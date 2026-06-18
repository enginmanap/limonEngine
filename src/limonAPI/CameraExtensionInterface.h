//
// Created by engin on 2026.
//

#ifndef LIMONENGINE_CAMERAEXTENSIONINTERFACE_H
#define LIMONENGINE_CAMERAEXTENSIONINTERFACE_H

#include <string>
#include <map>
#include <vector>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include "CameraAttachment.h"
#include "LimonTypes.h"

class LimonAPI;

/**
 * A registered, configurable camera rig.
 *
 * On shared library load, void registerCameraExtensions(std::map<std::string, CameraExtensionInterface*(*)(LimonAPI*)>*)
 * should be callable; for each rig type, its name and a constructor are inserted into the map.
 *
 * A CameraExtensionInterface IS-A CameraAttachment, so the engine's player camera consumes it directly for
 * pose (getCameraVariables) and projection (getProjection).
 *
 * Attachment bridging (the engine is the Attachable, on the rig's behalf):
 *   - A rig may follow a world object by returning its id from getAttachedObjectID() (and optionally a
 *     bone from getAttachedBoneID()).
 *   - Each frame, BEFORE getCameraVariables(), the engine resolves that object/bone's world transform via
 *     the transform/bone system and hands it to the rig through setAttachmentTransform() as already-
 *     decomposed translate / orientation / scale (the engine does the single decomposition, so no rig —
 *     and in particular no Python rig — has to decompose a matrix every frame).
 *   - getCameraVariables() then composes the rig's own local offset and any custom behaviour (smoothing,
 *     collision pushback, look-ahead) on top of that engine-provided parent transform.
 *   - A rig that returns 0 from getAttachedObjectID() is "unattached" and produces its pose itself.
 *
 * Single-active: many rig types may be registered, but only one is bound to the player camera at a time.
 */
class CameraExtensionInterface : public CameraAttachment {
    static std::map<std::string, CameraExtensionInterface*(*)(LimonAPI*)>* extensionTypesMap;
protected:
    LimonAPI* limonAPI = nullptr;
    std::vector<LimonTypes::GenericParameter> parameters;

    static std::map<std::string, CameraExtensionInterface*(*)(LimonAPI*)>* getMap() {
        // never deleted (exists until program termination) because we can't guarantee destruction order
        if (!extensionTypesMap) {
            extensionTypesMap = new std::map<std::string, CameraExtensionInterface*(*)(LimonAPI*)>();
        }
        return extensionTypesMap;
    }

    explicit CameraExtensionInterface(LimonAPI* limonAPI) : limonAPI(limonAPI) {}

public:
    static std::vector<std::string> getExtensionNames() {
        std::vector<std::string> names;
        for (auto it = getMap()->begin(); it != getMap()->end(); it++) {
            names.push_back(it->first);
        }
        return names;
    }

    static void registerType(const std::string& typeName, CameraExtensionInterface*(*constructor)(LimonAPI*)) {
        (*getMap())[typeName] = constructor;
    }

    static CameraExtensionInterface* createExtension(const std::string& name, LimonAPI* apiInstance) {
        auto it = getMap()->find(name);
        if (it == getMap()->end()) {
            return nullptr;
        }
        return it->second(apiInstance);
    }

    virtual std::string getName() const = 0;

    /** Configurable parameters of this rig (drives load/save and editor editing). */
    virtual std::vector<LimonTypes::GenericParameter> getParameters() const {
        return this->parameters;
    }

    virtual void setParameters(std::vector<LimonTypes::GenericParameter> parameters) {
        this->parameters = parameters;
    }

    /**
     * World object id this rig follows, or 0 if it is unattached (produces its pose itself).
     * When non-zero, the engine feeds the resolved world transform via setAttachmentTransform().
     */
    virtual uint32_t getAttachedObjectID() const { return 0; }

    /** Bone of the attached object to follow, or -1 for the object's origin transform. */
    virtual int32_t getAttachedBoneID() const { return -1; }

    /**
     * Engine-provided world transform of the attachment target (object or bone), pre-decomposed into
     * translate / orientation / scale and set before getCameraVariables() each frame. Default no-op for
     * rigs that don't attach. Components are passed (not a matrix) so rigs never decompose.
     */
    virtual void setAttachmentTransform(const glm::vec3& position [[gnu::unused]],
                                        const glm::quat& orientation [[gnu::unused]],
                                        const glm::vec3& scale [[gnu::unused]]) {}

    ~CameraExtensionInterface() override = default;
};


template<typename T>
CameraExtensionInterface* createCameraExtension(LimonAPI* limonAPI) {
    return new T(limonAPI);
}

template<typename T>
class CameraExtensionRegister : CameraExtensionInterface {
    std::string getName() const override {
        return "This object is not meant to be used";
    }
    bool isDirty() const override { return false; }
    void clearDirty() override {}
    void getCameraVariables(glm::vec3& /*position*/, glm::vec3& /*center*/, glm::vec3& /*up*/, glm::vec3& /*right*/) override {}
    CameraAttachment::ProjectionParameters getProjection() const override { return CameraAttachment::ProjectionParameters{}; }

public:
    explicit CameraExtensionRegister(const std::string& name) : CameraExtensionInterface(nullptr) {
        getMap()->insert(std::make_pair(name, &createCameraExtension<T>));
    }
};


#endif //LIMONENGINE_CAMERAEXTENSIONINTERFACE_H
