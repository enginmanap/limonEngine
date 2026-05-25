//
// Created by engin on 5/10/18.
//

#ifndef LIMONENGINE_TRANSFORMATION_H
#define LIMONENGINE_TRANSFORMATION_H

#include <functional>
#include <iostream>
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>
#include <tinyxml2.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "Utils/NoexceptFunction.h"

class Transformation {
    /* EDITOR INFORMATION PART */
    enum EditorModes {ROTATE_MODE, TRANSLATE_MODE, SCALE_MODE};
    struct ImGuizmoState {
        bool useSnap = false;
        float snap[3] = {1.0f, 1.0f, 1.0f};
        EditorModes mode = TRANSLATE_MODE;
    };
    /* EDITOR INFORMATION PART */

    mutable glm::mat4 worldTransform;//private

    NoexceptFunction updateCallback = nullptr;

    void notifyOwner() noexcept {
        updateCallback();
    }

    void propagateToChildren() {
        for (auto child : childTransforms) {
            child->isDirty = true;
            child->rotated = this->rotated;
            child->getWorldTransform();
            child->propagateUpdate();
        }
    }

    void propagateUpdate() noexcept {
        notifyOwner();
        propagateToChildren();
    }

    void setWorldTransform(const glm::mat4& transform) {
        this->worldTransform = transform;
        isDirty = false;
    }

    /**
     * If there is a parent, generateWorldTransform should count it.
     * Children are notified via propagateToChildren() inside propagateUpdate().
     */
    std::vector<Transformation*> childTransforms;
    Transformation* parentTransform = nullptr;

protected:
    glm::vec3 translate = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    mutable bool isDirty = true;
    bool rotated = false;

    //ATTENTION generateWorldTransform is not set when copy constructed
    std::function<glm::mat4()> generateWorldTransform;

    /**
     * Saving these values for each and every transform is not optimal, but there is a trade off:
     * 1) add a transformStack class, that adds a indirection and possibly data locality would suffer
     * 2) don't save it anywhere, calculate when need. CPU time would suffer
     * 3) save for all instances. Memory usage suffers, and by extension data locality suffers since there is more data.
     *
     * I choose 3. option, solely because it is easier to implement at the current state of the codebase. It can be reconsidered.
     *
     * This means even if there is no parent, single variants still needs to be updated.
     */
    glm::vec3 translateSingle = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 scaleSingle = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::quat orientationSingle = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    std::function<glm::mat4()> generateWorldTransformSingle = nullptr; //backup when parent is attached

    glm::mat4 generateWorldTransformDefault() const {
        return glm::translate(glm::mat4(1.0f), translateSingle) * glm::mat4_cast(orientationSingle) *
                               glm::scale(glm::mat4(1.0f), scaleSingle);
    }

    glm::mat4 generateRawWorldTransformWithOrWithoutParent() const {
        //glm::mat4 totalTransform = parentTransform->generateWorldTransformDefault() * this->generateWorldTransformSingle();
        Transformation* parent = this->parentTransform;
        glm::mat4 rawTotalTransform = this->generateWorldTransformDefault();
        while(parent != nullptr) {
            glm::mat4 parentTransformTemp = parent->generateWorldTransformDefault();
            rawTotalTransform = parentTransformTemp * rawTotalTransform;
            parent = parent->parentTransform;
        }
        return rawTotalTransform;
    }


    glm::mat4 generateWorldTransformWithParent(){
        //glm::mat4 totalTransform = parentTransform->generateWorldTransformDefault() * this->generateWorldTransformSingle();
        glm::mat4 rawTotalTransform = this->generateWorldTransformDefault();
        glm::mat4 totalTransform = this->generateWorldTransformSingle();
        Transformation* parent = this->parentTransform;
        while(parent != nullptr) {
            glm::mat4 parentTransformTemp = parent->generateWorldTransformDefault();
            rawTotalTransform = parentTransformTemp * rawTotalTransform;
            totalTransform = parentTransformTemp * totalTransform;
            parent = parent->parentTransform;
        }

        //glm::mat4 rawTotalTransform = TotalRawTransform * this->generateWorldTransformDefault();
        glm::vec3 temp1;//these are not used
        glm::vec4 temp2;

        glm::decompose(rawTotalTransform, scale, orientation, translate, temp1, temp2);//update the current of these
        return totalTransform;
    }

public:

    Transformation() {
        generateWorldTransform = std::bind(&Transformation::generateWorldTransformDefault, this);
    }

    ~Transformation() {
        disconnectFromStack();
    }

    Transformation(const Transformation& otherTransformation) {
        generateWorldTransform = std::bind(&Transformation::generateWorldTransformDefault, this);
        copyFromOtherTransform(otherTransformation);
    }

    void disconnectFromStack() {
        std::vector<Transformation*> childTransformsBackup = childTransforms;//why backup? because remove parent invalidates
        for (auto iterator = childTransformsBackup.begin(); iterator != childTransformsBackup.end(); ++iterator) {
            (*iterator)->removeParentTransform();
        }
        removeParentTransform();
    }

    void copyFromOtherTransform(const Transformation &otherTransformation) {
        if(otherTransformation.parentTransform != nullptr) {
            setParentTransform(otherTransformation.parentTransform);
        }

        translate = otherTransformation.translate;
        scale = otherTransformation.scale;
        orientation = otherTransformation.orientation;

        translateSingle = otherTransformation.translateSingle;
        scaleSingle = otherTransformation.scaleSingle;
        orientationSingle = otherTransformation.orientationSingle;

        isDirty = otherTransformation.isDirty;
        rotated = otherTransformation.rotated;
        worldTransform = otherTransformation.worldTransform;
    }

    Transformation& operator=(const Transformation& otherTransformation) {
        //this operator is required by std::vector. This will be a bumpy ride

        //first of, disconnect this method from any child and parent.
        disconnectFromStack();
        //now set the parent as the other transformation
        copyFromOtherTransform(otherTransformation);

        return *this;
    }

    const Transformation* getParentTransform() const {
        return parentTransform;
    }

    void setParentTransform(Transformation* transformation) {
        if(this->parentTransform == transformation) {
            return; //no op
        }
        if(this->parentTransform != nullptr) {
            removeParentTransform();
        }
        this->parentTransform = transformation;

        this->generateWorldTransformSingle = this->generateWorldTransform;
        this->generateWorldTransform = std::bind(&Transformation::generateWorldTransformWithParent, this);
        this->isDirty = true;

        this->scaleSingle = this->scale;
        this->translateSingle = this->translate;
        this->orientationSingle = this->orientation;

        transformation->childTransforms.push_back(this);

        this->getWorldTransform();
        this->propagateUpdate();
    }

    void removeParentTransform() noexcept {
        if(this->parentTransform == nullptr) {
            return;
        }

        // Ensure world composites are current while the parent is still connected.
        if(this->isDirty) {
            this->getWorldTransform();
        }

        // Remove from parent's children list
        auto element = std::find(this->parentTransform->childTransforms.begin(), this->parentTransform->childTransforms.end(), this);
        if(element != this->parentTransform->childTransforms.end()) {
            this->parentTransform->childTransforms.erase(element);
        } else {
            std::cerr << "Parent transform doesn't have this child in the list, this shouldn't have happened!" << std::endl;
        }

        // Clear parent references
        this->parentTransform = nullptr;
        this->generateWorldTransform = this->generateWorldTransformSingle;
        this->generateWorldTransformSingle = nullptr;

        // scale/translate/orientation are now fresh world composites — use them as the
        // new standalone values so the object stays at its current world position.
        this->scaleSingle = this->scale;
        this->translateSingle = this->translate;
        this->orientationSingle = this->orientation;

        this->isDirty = true;
        this->getWorldTransform();
        this->propagateUpdate();
    }

    void setUpdateCallback(NoexceptFunction callback) {
        updateCallback = std::move(callback);
    }

    void setGenerateWorldTransform(std::function<glm::mat4()> generateWorldTransform) {
        this->generateWorldTransform = generateWorldTransform;
    }

    void addScale(const glm::vec3 &scale) {
        //If there is no parent, keep both single and normal the same
        //because sets are used rarely but gets are used often. It will remove a branch from gets.
        if(this->parentTransform == nullptr) {
            this->scale *= scale;
        }
        this->scaleSingle *= scale;
        isDirty = true;
        propagateUpdate();
    }

    void setScale(const glm::vec3 &scale) {
        if(this->parentTransform == nullptr) {
            this->scale = scale;
        }
        this->scaleSingle = scale;
        isDirty = true;
        propagateUpdate();
    }


    const glm::vec3 getTranslateSingle() const {
        return translateSingle;
    }

    const glm::vec3 getTranslate() const {
        return translate;
    }

    const glm::vec3 getScaleSingle() const {
        return scaleSingle;
    }

    const glm::vec3 getScale() const {
        return scale;
    }

    const glm::quat getOrientationSingle() const {
        return orientationSingle;
    }

    const glm::quat getOrientation() const {
        return orientation;
    }

    void addTranslate(const glm::vec3 &translate) {
        if(this->parentTransform == nullptr) {
            this->translateSingle += translate;
            this->translate = this->translateSingle;
        } else {
            // translate is a world-space delta; convert to parent-local space.
            // w=0 treats the delta as a direction (not a point) so parent translation is ignored.
            glm::vec3 localDelta = glm::vec3(glm::inverse(this->parentTransform->getWorldTransform()) * glm::vec4(translate, 0.0f));
            this->translateSingle += localDelta;
        }
        isDirty = true;
        propagateUpdate();
    }

    void setTranslate(const glm::vec3 &translate) {
        if(this->parentTransform == nullptr) {
            this->translate = translate;
        }
        this->translateSingle = translate;
        isDirty = true;
        propagateUpdate();
    }

    void setOrientation(const glm::quat &orientation) {
        this->orientationSingle = glm::normalize(orientation);
        if(this->parentTransform == nullptr) {
            this->orientation = orientationSingle;
        }
        rotated = this->orientation.w < 0.99 || this->orientationSingle.w < 0.99; // with rotation w gets smaller.
        isDirty = true;
        propagateUpdate();
    }

    void addOrientation(const glm::quat &orientation) {
        this->orientationSingle *= orientation;
        this->orientationSingle = glm::normalize(this->orientationSingle);
        if(this->parentTransform == nullptr) {
            this->orientation = this->orientationSingle;
        }

        rotated = this->orientationSingle.w < 0.99; // with rotation w gets smaller.
        isDirty = true;
        propagateUpdate();
    }

    void setTransformationsNotPropagate(const glm::vec3& translate) {
        if(this->parentTransform == nullptr) {
            this->translate = translate;
        }
        this->translateSingle = translate;
        isDirty = true;
        propagateToChildren();
    }

    void setTransformationsNotPropagate(const glm::vec3& translate, const::glm::quat& orientation) {
        this->translateSingle = translate;
        this->orientationSingle = glm::normalize(orientation);

        if(this->parentTransform == nullptr) {
            this->translate = translate;
            this->orientation = orientationSingle;
        }
        isDirty = true;
        propagateToChildren();
    }

    void setTransformationsNotPropagate(const glm::vec3& translate, const::glm::quat& orientation, const glm::vec3& scale) {
        this->translateSingle = translate;
        this->orientationSingle = glm::normalize(orientation);
        this->scaleSingle = scale;
        rotated = this->orientation.w < 0.99; // with rotation w gets smaller.

        if(this->parentTransform == nullptr) {
            this->scale = scale;
            this->orientation = orientationSingle;//this is intentional, because it needs normalization
            this->translate = translate;
        }
        isDirty = true;
        propagateToChildren();
    }

    void setTransformations(const glm::vec3& translate, const::glm::quat& orientation) {
        this->translateSingle = translate;
        this->orientationSingle = glm::normalize(orientation);

        if(this->parentTransform == nullptr) {
            this->translate = translate;
            this->orientation = orientationSingle;
        }
        isDirty = true;
        propagateUpdate();
    }

    void setTransformations(const glm::vec3& translate, const glm::vec3& scale, const::glm::quat& orientation) {
        this->translateSingle = translate;
        this->orientationSingle = glm::normalize(orientation);
        this->scaleSingle = scale;
        if(this->parentTransform == nullptr) {
            this->scale = scale;
            this->orientation = orientationSingle;//this is intentional, because it needs normalization
            this->translate = translate;
        }
        rotated = this->orientationSingle.w < 0.99; // with rotation w gets smaller.
        isDirty = true;
        propagateUpdate();
    }

    bool isRotated() const {
        return rotated;
    }

    const glm::mat4 &getWorldTransform() const {
        if (isDirty) {
            this->worldTransform = generateWorldTransform();
            isDirty = false;
        }
        return worldTransform;
    }

    void markDirty() {
        isDirty = true;
    }

    bool addImGuiEditorElements(const glm::mat4 &cameraMatrix, const glm::mat4 &perspectiveMatrix, bool is2D = false, bool hasAttachableParent = false);

    bool addImGuizmoElements(const ImGuizmoState &editorState, const glm::mat4 &cameraMatrix,
                                 const glm::mat4 &perspectiveMatrix, bool is2D);

    void combine(const Transformation &otherTransformation);

    void getDifferenceAddition(const Transformation &otherTransformation, glm::vec3 &translate, glm::vec3 &scale,
                               glm::quat &rotation) const;

    void getDifferenceStacked(const Transformation& otherTransformation, glm::vec3 &translate, glm::vec3 &scale, glm::quat &rotation) const;

    bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode) const;

    // Saves translateSingle/scaleSingle/orientationSingle (local values relative to bone).
    // Required for bone-attached objects: boneTransforms are identity at load time, so
    // attachTo cannot correctly compute local from world for bone parents.
    bool serializeLocal(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode) const;

    bool deserialize(tinyxml2::XMLElement *transformationNode);

};


#endif //LIMONENGINE_TRANSFORMATION_H
