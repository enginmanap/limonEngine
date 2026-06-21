//
// Created by engin on 2026.
//

#ifndef LIMONENGINE_CAMERARIG_H
#define LIMONENGINE_CAMERARIG_H

#include <string>

#include "GameObject.h"
#include "../Attachable.h"
#include "../Editor/ImGuiResult.h"
#include "../Editor/ImGuiRequest.h"
#include "limonAPI/CameraExtensionInterface.h"

/**
 * A first-class scene object that drives the player camera.
 *
 * CameraRig is the engine-side GameObject; the actual camera BEHAVIOUR (pose composition, projection)
 * is a plugin-authored CameraExtensionInterface it owns (composition, not inheritance — the plugin type
 * can't subclass an engine GameObject across the plugin boundary).
 *
 * Because CameraRig IS-A Attachable, "follow an object" uses the engine's standard attachment system:
 * attach the CameraRig to the target object/bone, and each frame the rig feeds its held behaviour the
 * resolved parent world transform (pre-decomposed) before the camera reads pose. An UNATTACHED rig has
 * no parent, so the held behaviour produces its own pose.
 *
 * Many CameraRigs may exist in a world, but only one is active at a time (World::activeCameraRig).
 */
class CameraRig : public GameObject, public Attachable {
    std::string name;
    uint32_t worldID;
    CameraExtensionInterface* heldAttachment = nullptr; // owned: the configured camera behaviour
    Transformation transformation;

public:
    CameraRig(uint32_t worldID, const std::string& name, CameraExtensionInterface* heldAttachment)
        : name(name), worldID(worldID), heldAttachment(heldAttachment) {}

    ~CameraRig() override {
        delete heldAttachment;
    }

    /** The camera behaviour this rig drives the player camera with. */
    CameraExtensionInterface* getHeldAttachment() const {
        return heldAttachment;
    }

    /** The registered type name of the held behaviour (used to recreate it on load). */
    std::string getRigTypeName() const {
        return heldAttachment != nullptr ? heldAttachment->getName() : "";
    }

    /**
     * Feed the held behaviour the parent (attachment target) world transform via the engine's attachment
     * system, pre-decomposed into translate/orientation/scale, before the camera reads pose. No-op when
     * unattached (the held behaviour produces its own pose) or when there is no behaviour.
     */
    void feedHeldAttachmentTransform();

    // --- Attachable ---
    Transformation* getTransformation() override { return &transformation; }
    const Transformation* getTransformation() const override { return &transformation; }

    // --- GameObject ---
    ObjectTypes getTypeID() const override { return ObjectTypes::CAMERA_RIG; }
    std::string getName() const override { return name; }
    uint32_t getWorldObjectID() const override { return worldID; }
    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request) override;
};

#endif //LIMONENGINE_CAMERARIG_H
