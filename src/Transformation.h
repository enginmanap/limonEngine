//
// Created by engin on 5/10/18.
//

#ifndef LIMONENGINE_TRANSFORMATION_H
#define LIMONENGINE_TRANSFORMATION_H


#include <functional>
#include <glm/glm.hpp>
#include <tinyxml2.h>


class Transformation {
    /* EDITOR INFORMATION PART */
    enum EditorModes {ROTATE_MODE, TRANSLATE_MODE, SCALE_MODE};
    struct ImGuizmoState {
        bool useSnap = false;
        float snap[3] = {1.0f, 1.0f, 1.0f};
        EditorModes mode = TRANSLATE_MODE;
    };
    /* EDITOR INFORMATION PART */

    glm::mat4 worldTransform;//private

    void setWorldTransform(const glm::mat4& transform){
        this->worldTransform = transform;
        isDirty = false;
    }

    void propagateUpdate(){
        if(this->updateCallback) {
            updateCallback();
        }
    }
protected:
    glm::vec3 translate;
    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    bool isDirty = true;
    bool rotated = false;

    glm::mat4 generateWorldTransformDefault(){
        return glm::translate(glm::mat4(1.0f), translate) * glm::mat4_cast(orientation) *
                               glm::scale(glm::mat4(1.0f), scale);
    }

    std::function<glm::mat4()> generateWorldTransform;
    std::function<void()> updateCallback;
public:

    Transformation() {
        generateWorldTransform = std::bind(&Transformation::generateWorldTransformDefault, this);
    }

    void setUpdateCallback(std::function<void()> updateCallback) {
        this->updateCallback = updateCallback;
    }

    void setGenerateWorldTransform(std::function<glm::mat4()> generateWorldTransform) {
        this->generateWorldTransform = generateWorldTransform;
    }

    void addScale(const glm::vec3 &scale) {
        this->scale *= scale;
        isDirty = true;
        propagateUpdate();
    }

    void setScale(const glm::vec3 &scale) {
        this->scale = scale;
        isDirty = true;
        propagateUpdate();
    }

    const glm::vec3 &getTranslate() const {
        return translate;
    }

    const glm::vec3 &getScale() const {
        return scale;
    }

    const glm::quat &getOrientation() const {
        return orientation;
    }

    void addTranslate(const glm::vec3 &translate) {
        this->translate += translate;
        isDirty = true;
        propagateUpdate();
    }

    void setTranslate(const glm::vec3 &translate) {
        this->translate = translate;
        isDirty = true;
        propagateUpdate();
    }

    void setOrientation(const glm::quat &orientation) {
        this->orientation = glm::normalize(orientation);
        rotated = this->orientation.w < 0.99; // with rotation w gets smaller.
        isDirty = true;
        propagateUpdate();
    }

    void addOrientation(const glm::quat &orientation) {
        this->orientation *= orientation;
        this->orientation = glm::normalize(this->orientation);
        rotated = this->orientation.w < 0.99; // with rotation w gets smaller.
        isDirty = true;
        propagateUpdate();
    }

    void setTransformationsNotPropagate(const glm::vec3& translate) {
        this->translate = translate;
        isDirty = true;
    }

    void setTransformationsNotPropagate(const glm::vec3& translate, const::glm::quat& orientation) {
        this->translate = translate;
        this->orientation = orientation;
        isDirty = true;
    }

    void setTransformationsNotPropagate(const glm::vec3& translate, const::glm::quat& orientation, const glm::vec3& scale) {
        this->translate = translate;
        this->orientation = orientation;
        this->scale = scale;
        isDirty = true;
    }

    bool isRotated() const {
        return rotated;
    }

    const glm::mat4 &getWorldTransform() {
        if (isDirty) {
            this->setWorldTransform(generateWorldTransform());
        }
        return worldTransform;
    }

    bool addImGuiEditorElements(const glm::mat4& cameraMatrix, const glm::mat4& perspectiveMatrix);

    void addImGuizmoElements(const ImGuizmoState& editorState, const glm::mat4& cameraMatrix, const glm::mat4& perspectiveMatrix);

    void combine(const Transformation &otherTransformation);

    void getDifference(const Transformation& otherTransformation, glm::vec3 &translate, glm::vec3 &scale, glm::quat &rotation) const;

    bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode) const;

    bool deserialize(tinyxml2::XMLElement *transformationNode);
};


#endif //LIMONENGINE_TRANSFORMATION_H
