//
// Created by engin on 19.03.2026.
//

#ifndef LIMONENGINE_ATTACHABLE_H
#define LIMONENGINE_ATTACHABLE_H


#include "Transformation.h"
#include "HasTransform.h"
#include <vector>
#include <algorithm>
#include <tinyxml2.h>

class Attachable : public virtual HasTransform {
protected:
    Attachable* parentObject = nullptr;
    int32_t parentBoneID = -1;
    std::vector<Attachable*> children;

public:
    virtual ~Attachable() noexcept {
        // Null out children's parent pointer. Derived destructors handle transform cleanup.
        for (auto child : children) {
            child->parentObject = nullptr;
        }
    }

    // Called whenever this object's world transform changes — either because the
    // object itself was moved or because a parent in the hierarchy moved.
    // Default is a no-op; override to sync derived state (physics bodies, cached
    // positions, etc.).
    virtual void onTransformUpdated() {}

    // Returns the transform point to attach children to. Models override this
    // to return bone-specific transforms when a bone is selected.
    virtual Transformation* getAttachmentTransformFor(int32_t /*boneID*/) {
        return getTransformation();
    }

    // Wire parent link while treating the child's CURRENT Single values as the local offset.
    // Use this when the caller has already computed the desired local-space transform and
    // created the object there.
    void attachToWithLocalOffset(Attachable* parent, int32_t boneID = -1) {
        this->parentObject = parent;
        this->parentBoneID = boneID;
        parent->addChild(this);

        Transformation* myTransform    = this->getTransformation();
        Transformation* parentTransform = parent->getAttachmentTransformFor(boneID);

        // Capture Single values before setParentTransform overwrites them with world values.
        glm::vec3 localTranslate    = myTransform->getTranslateSingle();
        glm::vec3 localScale        = myTransform->getScaleSingle();
        glm::quat localOrientation  = myTransform->getOrientationSingle();

        myTransform->setParentTransform(parentTransform);
        // Restore the caller's intended local offset.
        myTransform->setTransformations(localTranslate, localScale, glm::normalize(localOrientation));
    }

    // Wire both the relationship and the Transformation parent link in one call.
    // The child keeps its current WORLD position; the local offset is derived automatically.
    // Use this when the child was created at the intended world position (e.g. from a raycast hit).
    void attachTo(Attachable* parent, int32_t boneID = -1) {
        this->parentObject = parent;
        this->parentBoneID = boneID;
        parent->addChild(this);

        Transformation* myTransform    = this->getTransformation();
        Transformation* parentTransform = parent->getAttachmentTransformFor(boneID);

        // Snapshot world matrices before the parent link changes anything. Use the child's
        // OFFSET-FREE world (decomposed members) rather than getWorldTransform(): for a
        // PhysicalRenderable the latter is processTransformForPyhsics(), which bakes in centerOffset,
        // and generateWorldTransformWithParent re-applies that offset — double-counting it. The world
        // members are the offset-free world the parent composition actually reconstructs against.
        myTransform->getWorldTransform();//refresh world members
        glm::mat4 myWorld     = glm::translate(glm::mat4(1.0f), myTransform->getTranslate()) *
                                glm::mat4_cast(myTransform->getOrientation()) *
                                glm::scale(glm::mat4(1.0f), myTransform->getScale());
        glm::mat4 parentWorld = parentTransform->getWorldTransform();

        myTransform->setParentTransform(parentTransform);

        // Rewrite Single values to the local offset so that
        // parent_world * local == my_world (object stays at its world position).
        glm::mat4 localMatrix = glm::inverse(parentWorld) * myWorld;
        glm::vec3 localTranslate, localScale, skew;
        glm::quat localOrientation;
        glm::vec4 perspective;
        glm::decompose(localMatrix, localScale, localOrientation, localTranslate, skew, perspective);
        myTransform->setTransformations(localTranslate, localScale, glm::normalize(localOrientation));
    }

    // Remove the relationship and the Transformation parent link.
    void detach() noexcept {
        if (!parentObject) return;
        this->getTransformation()->removeParentTransform();
        parentObject->removeChild(this);
        parentObject = nullptr;
        parentBoneID = -1;
    }

    virtual void setParentObject(Attachable* parent, int32_t boneID = -1) {
        this->parentObject = parent;
        this->parentBoneID = boneID;
    }

    virtual void removeParentObject() {
        if (this->parentObject) {
            this->parentObject->removeChild(this);
            this->parentObject = nullptr;
            this->parentBoneID = -1;
        }
    }

    virtual void addChild(Attachable* child) {
        if (std::find(children.begin(), children.end(), child) == children.end()) {
            children.push_back(child);
        }
    }

    virtual bool removeChild(Attachable* child) {
        auto it = std::remove(children.begin(), children.end(), child);
        if (it != children.end()) {
            children.erase(it, children.end());
            return true;
        }
        return false;
    }

    virtual Attachable* getParentObject() const { return parentObject; }
    virtual int32_t getParentBoneID() const { return parentBoneID; }
    virtual const std::vector<Attachable*>& getChildren() const { return children; }
    virtual bool hasChildren() const { return !children.empty(); }

    // Default: not part of hierarchical (Object/Child) serialization.
    // PhysicalRenderable redeclares this as pure virtual so Models must implement it.
    virtual bool fillObjects(tinyxml2::XMLDocument& /*document*/, tinyxml2::XMLElement* /*objectsNode*/) const {
        return false;
    }
};


#endif //LIMONENGINE_ATTACHABLE_H
