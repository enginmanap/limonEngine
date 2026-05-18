//
// Created by engin on 19.03.2026.
//

#ifndef LIMONENGINE_ATTACHABLE_H
#define LIMONENGINE_ATTACHABLE_H


#include "Transformation.h"
#include <vector>
#include <algorithm>
#include <tinyxml2.h>

class Attachable {
protected:
    Attachable* parentObject = nullptr;
    int32_t parentBoneID = -1;
    std::vector<Attachable*> children;

public:
    virtual ~Attachable() {
        // Null out children's parent pointer. Derived destructors handle transform cleanup.
        for (auto child : children) {
            child->parentObject = nullptr;
        }
    }

    virtual Transformation* getTransformation() = 0;
    virtual const Transformation* getTransformation() const = 0;

    // Returns the transform point to attach children to. Models override this
    // to return bone-specific transforms when a bone is selected.
    virtual Transformation* getAttachmentTransformFor(int32_t /*boneID*/) {
        return getTransformation();
    }

    // Wire both the relationship and the Transformation parent link in one call.
    void attachTo(Attachable* parent, int32_t boneID = -1) {
        this->parentObject = parent;
        this->parentBoneID = boneID;
        parent->addChild(this);
        this->getTransformation()->setParentTransform(parent->getAttachmentTransformFor(boneID));
    }

    // Remove the relationship and the Transformation parent link.
    void detach() {
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
