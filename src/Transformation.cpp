//
// Created by engin on 13.05.2018.
//

#include <tinyxml2.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "Transformation.h"
#include "../libs/ImGui/imgui.h"
#include "../libs/ImGuizmo/ImGuizmo.h"


bool Transformation::addImGuiEditorElements(const glm::mat4& cameraMatrix, const glm::mat4& perspectiveMatrix) {
    static ImGuizmoState editorState;
    static glm::vec3 preciseTranslatePoint = translateSingle;

    if(this->parentTransform != nullptr) {
        ImGui::Text("This transform has a parent, and it is relative.");
    }

    bool updated = false;
    bool crudeUpdated = false;

    if (ImGui::IsKeyPressed(83)) {
        editorState.useSnap = !editorState.useSnap;
    }

    /*
     * at first we decide whether we are in rotation, scale or translate mode.
     */

    if (ImGui::RadioButton("Translate", editorState.mode == TRANSLATE_MODE)) {
        editorState.mode = TRANSLATE_MODE;
    }

    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", editorState.mode == ROTATE_MODE)) {
        editorState.mode = ROTATE_MODE;
    }

    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", editorState.mode == SCALE_MODE)) {
        editorState.mode = SCALE_MODE;
    }

    switch (editorState.mode) {
        case TRANSLATE_MODE: {
            if(this->parentTransform != nullptr ) {
                ImGui::Text((std::string("Current Total Translate X") + std::to_string(translate.x)).c_str());
                ImGui::Text((std::string("Current Total Translate Y") + std::to_string(translate.y)).c_str());
                ImGui::Text((std::string("Current Total Translate Z") + std::to_string(translate.z)).c_str());
            }
            glm::vec3 tempTranslate = translateSingle;
            updated =
                    ImGui::DragFloat("Precise Position X", &(tempTranslate.x), 0.01f, preciseTranslatePoint.x - 5.0f,
                                     preciseTranslatePoint.x + 5.0f) || updated;
            updated =
                    ImGui::DragFloat("Precise Position Y", &(tempTranslate.y), 0.01f, preciseTranslatePoint.y - 5.0f,
                                     preciseTranslatePoint.y + 5.0f) || updated;
            updated =
                    ImGui::DragFloat("Precise Position Z", &(tempTranslate.z), 0.01f, preciseTranslatePoint.z - 5.0f,
                                     preciseTranslatePoint.z + 5.0f) || updated;
            ImGui::NewLine();
            crudeUpdated =
                    ImGui::SliderFloat("Crude Position X", &(tempTranslate.x), -100.0f, 100.0f) || crudeUpdated;
            crudeUpdated =
                    ImGui::SliderFloat("Crude Position Y", &(tempTranslate.y), -100.0f, 100.0f) || crudeUpdated;
            crudeUpdated =
                    ImGui::SliderFloat("Crude Position Z", &(tempTranslate.z), -100.0f, 100.0f) || crudeUpdated;
            if (updated || crudeUpdated) {
                setTranslate(tempTranslate);
            }
            if (crudeUpdated) {
                preciseTranslatePoint = translateSingle;
            }
            ImGui::NewLine();
            ImGui::Checkbox("", &(editorState.useSnap));
            ImGui::SameLine();
            ImGui::InputFloat3("Snap", &(editorState.snap[0]));
            break;
        }
        case ROTATE_MODE: {
            glm::quat tempOrientation = orientationSingle;
            updated = ImGui::DragFloat("Rotate X", &(tempOrientation.x), -1.0f, 1.0f) || updated;
            updated = ImGui::DragFloat("Rotate Y", &(tempOrientation.y), -1.0f, 1.0f) || updated;
            updated = ImGui::DragFloat("Rotate Z", &(tempOrientation.z), -1.0f, 1.0f) || updated;
            updated = ImGui::DragFloat("Rotate W", &(tempOrientation.w), -1.0f, 1.0f) || updated;
            if (updated || crudeUpdated) {
                setOrientation(tempOrientation);
            }
            ImGui::NewLine();
            ImGui::Checkbox("", &(editorState.useSnap));
            ImGui::SameLine();
            ImGui::InputFloat("Angle Snap", &(editorState.snap[0]));
            break;
        }
        case SCALE_MODE: {
            if(this->parentTransform != nullptr ) {
                ImGui::Text((std::string("Current Total Scale X") + std::to_string(scale.x)).c_str());
                ImGui::Text((std::string("Current Total Scale Y") + std::to_string(scale.y)).c_str());
                ImGui::Text((std::string("Current Total Scale Z") + std::to_string(scale.z)).c_str());
            }

            glm::vec3 tempScale = scaleSingle;
            updated = ImGui::DragFloat("Scale X", &(tempScale.x), 0.01, 0.01f, 10.0f) || updated;
            updated = ImGui::DragFloat("Scale Y", &(tempScale.y), 0.01, 0.01f, 10.0f) || updated;
            updated = ImGui::DragFloat("Scale Z", &(tempScale.z), 0.01, 0.01f, 10.0f) || updated;
            ImGui::NewLine();
            updated = ImGui::SliderFloat("Massive Scale X", &(tempScale.x), 0.01f, 100.0f) || updated;
            updated = ImGui::SliderFloat("Massive Scale Y", &(tempScale.y), 0.01f, 100.0f) || updated;
            updated = ImGui::SliderFloat("Massive Scale Z", &(tempScale.z), 0.01f, 100.0f) || updated;
            //it is possible to enter any scale now. If user enters 0, don't update
            if ((updated || crudeUpdated) && (tempScale.x != 0.0f && tempScale.y != 0.0f && tempScale.z != 0.0f)) {
                setScale(tempScale);
            }
            ImGui::NewLine();
            ImGui::Checkbox("", &(editorState.useSnap));
            ImGui::SameLine();
            ImGui::InputFloat("Scale Snap", &(editorState.snap[0]));
            break;
        }
    }
    addImGuizmoElements(editorState, cameraMatrix, perspectiveMatrix);
    return updated || crudeUpdated;
}


void Transformation::addImGuizmoElements(const ImGuizmoState& editorState, const glm::mat4& cameraMatrix, const glm::mat4& perspectiveMatrix) {
    ImGuizmo::OPERATION mCurrentGizmoOperation = ImGuizmo::TRANSLATE;

    switch (editorState.mode) {
        case EditorModes::TRANSLATE_MODE:
            mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
            break;
        case EditorModes::ROTATE_MODE:
            mCurrentGizmoOperation = ImGuizmo::ROTATE;
            break;
        case EditorModes::SCALE_MODE:
            mCurrentGizmoOperation = ImGuizmo::SCALE;
            break;
    }
    glm::mat4 objectMatrix;
    ImGuizmo::BeginFrame();
    glm::vec3 eulerRotation = glm::eulerAngles(orientation);

    eulerRotation = eulerRotation * 57.2957795f;
    ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(translate),
                                            glm::value_ptr(eulerRotation),
                                            glm::value_ptr(scale),
                                            glm::value_ptr(objectMatrix));

    static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    float tempSnap[3] = {editorState.snap[0], editorState.snap[1], editorState.snap[2] };
    ImGuizmo::Manipulate(glm::value_ptr(cameraMatrix), glm::value_ptr(perspectiveMatrix), mCurrentGizmoOperation, mCurrentGizmoMode, glm::value_ptr(objectMatrix), NULL, editorState.useSnap ? &(tempSnap[0]) : NULL);

    if(this->parentTransform != nullptr) {
        //first calculate the difference from parent if there is
        objectMatrix = glm::inverse(parentTransform->getWorldTransform()) * objectMatrix;

    }
    //now we should have object matrix updated, update the object
    glm::vec3 tempTranslate, tempScale;
    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(objectMatrix), glm::value_ptr(tempTranslate), glm::value_ptr(eulerRotation), glm::value_ptr(tempScale));
    glm::quat tempOrientation = glm::quat(eulerRotation / 57.2957795f);
    switch (mCurrentGizmoOperation) {
        case ImGuizmo::TRANSLATE:
            setTranslate(tempTranslate);
            break;
        case ImGuizmo::ROTATE:
            setOrientation(tempOrientation);
            break;
        case ImGuizmo::SCALE:
            setScale(tempScale);
            break;
        case ImGuizmo::BOUNDS://not used
            break;
    }
}

void Transformation::getDifference(const Transformation& otherTransformation, glm::vec3 &translateOut, glm::vec3 &scaleOut, glm::quat &rotationOut) const {
    translateOut = otherTransformation.translate - this->translate;
    scaleOut = otherTransformation.scale / this->scale;
    rotationOut = otherTransformation.orientation;
    rotationOut = glm::inverse(rotationOut);
    rotationOut = rotationOut * this->orientation;
}

bool Transformation::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode) const {
    tinyxml2::XMLElement* currentElement;
    tinyxml2::XMLElement *classNode = document.NewElement("Transformation");
    parentNode->InsertEndChild(classNode);

    tinyxml2::XMLElement *parent = document.NewElement("Scale");
    currentElement = document.NewElement("X");
    currentElement->SetText(scaleSingle.x);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("Y");
    currentElement->SetText(scaleSingle.y);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("Z");
    currentElement->SetText(scaleSingle.z);
    parent->InsertEndChild(currentElement);
    classNode->InsertEndChild(parent);

    parent = document.NewElement("Translate");
    currentElement = document.NewElement("X");
    currentElement->SetText(translateSingle.x);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("Y");
    currentElement->SetText(translateSingle.y);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("Z");
    currentElement->SetText(translateSingle.z);
    parent->InsertEndChild(currentElement);
    classNode->InsertEndChild(parent);

    parent = document.NewElement("Rotate");
    currentElement = document.NewElement("X");
    currentElement->SetText(orientationSingle.x);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("Y");
    currentElement->SetText(orientationSingle.y);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("Z");
    currentElement->SetText(orientationSingle.z);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("W");
    currentElement->SetText(orientationSingle.w);
    parent->InsertEndChild(currentElement);
    classNode->InsertEndChild(parent);

    return true;
}

bool Transformation::deserialize(tinyxml2::XMLElement *transformationNode) {
    tinyxml2::XMLElement* transformationAttribute;
    tinyxml2::XMLElement* transformationAttributeAttribute;
    transformationAttribute =  transformationNode->FirstChildElement("Scale");
    if (transformationAttribute == nullptr) {
        std::cout << "Transformation does not have scale." << std::endl;
    } else {
        glm::vec3 scale;
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("X");
        if(transformationAttributeAttribute != nullptr) {
            scale.x = std::stof(transformationAttributeAttribute->GetText());
        } else {
            scale.x = 1.0;
        }
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("Y");
        if(transformationAttributeAttribute != nullptr) {
            scale.y = std::stof(transformationAttributeAttribute->GetText());
        } else {
            scale.y = 1.0;
        }
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("Z");
        if(transformationAttributeAttribute != nullptr) {
            scale.z = std::stof(transformationAttributeAttribute->GetText());
        } else {
            scale.z = 1.0;
        }
        setScale(scale);
    }

    transformationAttribute =  transformationNode->FirstChildElement("Translate");
    if (transformationAttribute == nullptr) {
        std::cout << "Transformation does not have translate." << std::endl;
    } else {
        glm::vec3 translate;
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("X");
        if(transformationAttributeAttribute != nullptr) {
            translate.x = std::stof(transformationAttributeAttribute->GetText());
        } else {
            translate.x = 0.0;
        }
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("Y");
        if(transformationAttributeAttribute != nullptr) {
            translate.y = std::stof(transformationAttributeAttribute->GetText());
        } else {
            translate.y = 0.0;
        }
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("Z");
        if(transformationAttributeAttribute != nullptr) {
            translate.z = std::stof(transformationAttributeAttribute->GetText());
        } else {
            translate.z = 0.0;
        }
        setTranslate(translate);
    }

    transformationAttribute =  transformationNode->FirstChildElement("Rotate");
    if (transformationAttribute == nullptr) {
        std::cout << "Transformation does not have Rotation." << std::endl;
    } else {
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("X");
        glm::quat orientation;
        if(transformationAttributeAttribute != nullptr) {
            orientation.x = std::stof(transformationAttributeAttribute->GetText());
        } else {
            orientation.x = 0.0;
        }
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("Y");
        if(transformationAttributeAttribute != nullptr) {
            orientation.y = std::stof(transformationAttributeAttribute->GetText());
        } else {
            orientation.y = 0.0;
        }
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("Z");
        if(transformationAttributeAttribute != nullptr) {
            orientation.z = std::stof(transformationAttributeAttribute->GetText());
        } else {
            orientation.z = 0.0;
        }
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("W");
        if(transformationAttributeAttribute != nullptr) {
            orientation.w = std::stof(transformationAttributeAttribute->GetText());
        } else {
            orientation.w = 0.0;
        }
        setOrientation(orientation);
    }
    //now propagate the load
    propagateUpdate();
    return true;
}

void Transformation::combine(const Transformation &otherTransformation) {
    this->orientation *= otherTransformation.getOrientation();
    this->orientation = glm::normalize(this->orientation);
    rotated = this->orientation.w < 0.99; // with rotation w gets smaller.

    this->scale *= otherTransformation.getScale();

    this->translate += otherTransformation.getTranslate();

    isDirty = true;

    propagateUpdate();
}
