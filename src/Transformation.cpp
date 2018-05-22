//
// Created by engin on 13.05.2018.
//

#include <tinyxml2.h>
#include <glm/gtc/quaternion.hpp>
#include "Transformation.h"
#include "../libs/ImGui/imgui.h"
#include "../libs/ImGuizmo/ImGuizmo.h"


bool Transformation::addImGuiEditorElements(const glm::mat4& cameraMatrix, const glm::mat4& perspectiveMatrix) {
    static ImGuizmoState editorState;
    static glm::vec3 preciseTranslatePoint = translate;

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
            glm::vec3 tempTranslate = translate;
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
                preciseTranslatePoint = translate;
            }
            ImGui::NewLine();
            ImGui::Checkbox("", &(editorState.useSnap));
            ImGui::SameLine();
            ImGui::InputFloat3("Snap", &(editorState.snap[0]));
            break;
        }
        case ROTATE_MODE: {
            glm::quat tempOrientation = orientation;
            updated = ImGui::SliderFloat("Rotate X", &(tempOrientation.x), -1.0f, 1.0f) || updated;
            updated = ImGui::SliderFloat("Rotate Y", &(tempOrientation.y), -1.0f, 1.0f) || updated;
            updated = ImGui::SliderFloat("Rotate Z", &(tempOrientation.z), -1.0f, 1.0f) || updated;
            updated = ImGui::SliderFloat("Rotate W", &(tempOrientation.w), -1.0f, 1.0f) || updated;
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
            glm::vec3 tempScale = scale;
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

    //now we should have object matrix updated, update the object
    glm::vec3 scale, translate;
    glm::quat rotation;
    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(objectMatrix), glm::value_ptr(translate), glm::value_ptr(eulerRotation), glm::value_ptr(scale));
    rotation = glm::quat(eulerRotation / 57.2957795f);
    switch (mCurrentGizmoOperation) {
        case ImGuizmo::TRANSLATE:
            setTranslate(translate);
            break;
        case ImGuizmo::ROTATE:
            setOrientation(rotation);
            break;
        case ImGuizmo::SCALE:
            setScale(scale);
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
    currentElement->SetText(scale.x);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("Y");
    currentElement->SetText(scale.y);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("Z");
    currentElement->SetText(scale.z);
    parent->InsertEndChild(currentElement);
    classNode->InsertEndChild(parent);

    parent = document.NewElement("Translate");
    currentElement = document.NewElement("X");
    currentElement->SetText(translate.x);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("Y");
    currentElement->SetText(translate.y);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("Z");
    currentElement->SetText(translate.z);
    parent->InsertEndChild(currentElement);
    classNode->InsertEndChild(parent);

    parent = document.NewElement("Rotate");
    currentElement = document.NewElement("X");
    currentElement->SetText(orientation.x);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("Y");
    currentElement->SetText(orientation.y);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("Z");
    currentElement->SetText(orientation.z);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("W");
    currentElement->SetText(orientation.w);
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
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("X");
        if(transformationAttributeAttribute != nullptr) {
            this->scale.x = std::stof(transformationAttributeAttribute->GetText());
        } else {
            this->scale.x = 1.0;
        }
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("Y");
        if(transformationAttributeAttribute != nullptr) {
            this->scale.y = std::stof(transformationAttributeAttribute->GetText());
        } else {
            this->scale.y = 1.0;
        }
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("Z");
        if(transformationAttributeAttribute != nullptr) {
            this->scale.z = std::stof(transformationAttributeAttribute->GetText());
        } else {
            this->scale.z = 1.0;
        }
    }

    transformationAttribute =  transformationNode->FirstChildElement("Translate");
    if (transformationAttribute == nullptr) {
        std::cout << "Transformation does not have translate." << std::endl;
    } else {
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("X");
        if(transformationAttributeAttribute != nullptr) {
            this->translate.x = std::stof(transformationAttributeAttribute->GetText());
        } else {
            this->translate.x = 0.0;
        }
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("Y");
        if(transformationAttributeAttribute != nullptr) {
            this->translate.y = std::stof(transformationAttributeAttribute->GetText());
        } else {
            this->translate.y = 0.0;
        }
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("Z");
        if(transformationAttributeAttribute != nullptr) {
            this->translate.z = std::stof(transformationAttributeAttribute->GetText());
        } else {
            this->translate.z = 0.0;
        }
    }

    transformationAttribute =  transformationNode->FirstChildElement("Rotate");
    if (transformationAttribute == nullptr) {
        std::cout << "Transformation does not have Rotation." << std::endl;
    } else {
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("X");
        if(transformationAttributeAttribute != nullptr) {
            this->orientation.x = std::stof(transformationAttributeAttribute->GetText());
        } else {
            this->orientation.x = 0.0;
        }
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("Y");
        if(transformationAttributeAttribute != nullptr) {
            this->orientation.y = std::stof(transformationAttributeAttribute->GetText());
        } else {
            this->orientation.y = 0.0;
        }
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("Z");
        if(transformationAttributeAttribute != nullptr) {
            this->orientation.z = std::stof(transformationAttributeAttribute->GetText());
        } else {
            this->orientation.z = 0.0;
        }
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("W");
        if(transformationAttributeAttribute != nullptr) {
            this->orientation.w = std::stof(transformationAttributeAttribute->GetText());
        } else {
            this->orientation.w = 0.0;
        }
        this->orientation = glm::normalize(this->orientation);
    }
    //now propagate the load
    propagateUpdate();
    return true;
}
