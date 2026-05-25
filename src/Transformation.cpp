//
// Created by engin on 13.05.2018.
//

#include <tinyxml2.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <glm/ext.hpp>
#include "Transformation.h"
#include "../libs/ImGui/imgui.h"
#include "../libs/ImGuizmo/ImGuizmo.h"

bool
Transformation::addImGuiEditorElements(const glm::mat4 &cameraMatrix, const glm::mat4 &perspectiveMatrix, bool is2D, bool hasAttachableParent) {
    static ImGuizmoState editorState;
    static glm::vec3 preciseTranslatePoint = translate;

    // True only when there is a genuine hierarchy parent (not an animation-internal parent).
    const bool isChildWithParent = (this->parentTransform != nullptr) && hasAttachableParent;

    if(isChildWithParent) {
        ImGui::Text("This transform has a parent, and it is relative.");
    }

    bool updated = false;
    bool crudeUpdated = false;

    if (ImGui::IsKeyPressed(ImGuiKey_LeftCtrl)) {
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
            if(isChildWithParent) {
                ImGui::Text("World Translate X: %s", std::to_string(translate.x).c_str());
                ImGui::Text("World Translate Y: %s", std::to_string(translate.y).c_str());
                ImGui::Text("World Translate Z: %s", std::to_string(translate.z).c_str());
            }
            // Children show local offset; roots (and animation-internal parents) show world position.
            glm::vec3 tempTranslate = isChildWithParent ? translateSingle : translate;
            const char* posLabel = isChildWithParent ? "Local Position X" : "Precise Position X";
            updated =
                    ImGui::DragFloat(posLabel, &(tempTranslate.x), 0.01f, preciseTranslatePoint.x - 5.0f,
                                     preciseTranslatePoint.x + 5.0f) || updated;
            posLabel = isChildWithParent ? "Local Position Y" : "Precise Position Y";
            updated =
                    ImGui::DragFloat(posLabel, &(tempTranslate.y), 0.01f, preciseTranslatePoint.y - 5.0f,
                                     preciseTranslatePoint.y + 5.0f) || updated;
            if(!is2D) {
                posLabel = isChildWithParent ? "Local Position Z" : "Precise Position Z";
                updated = ImGui::DragFloat(posLabel, &(tempTranslate.z), 0.01f,
                                           preciseTranslatePoint.z - 5.0f,
                                           preciseTranslatePoint.z + 5.0f) || updated;
                ImGui::NewLine();
                posLabel = isChildWithParent ? "Crude Local X" : "Crude Position X";
                crudeUpdated =
                        ImGui::SliderFloat(posLabel, &(tempTranslate.x), -100.0f, 100.0f) || crudeUpdated;
                posLabel = isChildWithParent ? "Crude Local Y" : "Crude Position Y";
                crudeUpdated =
                        ImGui::SliderFloat(posLabel, &(tempTranslate.y), -100.0f, 100.0f) || crudeUpdated;
                posLabel = isChildWithParent ? "Crude Local Z" : "Crude Position Z";
                crudeUpdated =
                        ImGui::SliderFloat(posLabel, &(tempTranslate.z), -100.0f, 100.0f) || crudeUpdated;
            }
            if (updated || crudeUpdated) {
                setTranslate(tempTranslate);
            }
            if (crudeUpdated) {
                preciseTranslatePoint = isChildWithParent ? translateSingle : translate;
            }
            ImGui::NewLine();
            ImGui::Checkbox("##SnapCheckBox", &(editorState.useSnap));
            if(ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Builtin Camera tags are:");
                ImGui::Text("shortcut S");
                ImGui::EndTooltip();
            }
            ImGui::SameLine();
            ImGui::InputFloat3("Snap", &(editorState.snap[0]));
            break;
        }
        case ROTATE_MODE: {
            if(isChildWithParent) {
                ImGui::Text("World Rotate X: %s",  std::to_string(orientation.x).c_str());
                ImGui::Text("World Rotate Y: %s",  std::to_string(orientation.y).c_str());
                ImGui::Text("World Rotate Z: %s",  std::to_string(orientation.z).c_str());
                ImGui::Text("World Rotate W: %s",  std::to_string(orientation.w).c_str());
            }
            glm::quat tempOrientation = isChildWithParent ? orientationSingle : orientation;
            if(!is2D) {
                updated = ImGui::DragFloat("Rotate X", &(tempOrientation.x), 0.001f, -1.0f, 1.0f) || updated;
                updated = ImGui::DragFloat("Rotate Y", &(tempOrientation.y), 0.001f, -1.0f, 1.0f) || updated;
                updated = ImGui::DragFloat("Rotate Z", &(tempOrientation.z), 0.001f, -1.0f, 1.0f) || updated;
                updated = ImGui::DragFloat("Rotate W", &(tempOrientation.w), 0.001f, -1.0f, 1.0f) || updated;
            } else {
                ImGui::Text("Rotate W %f", tempOrientation.w);
            }
            if (updated || crudeUpdated) {
                setOrientation(tempOrientation);
            }
            ImGui::NewLine();
            ImGui::Checkbox("##SnapCheckBoxForAngle", &(editorState.useSnap));
            ImGui::SameLine();
            ImGui::InputFloat("Angle Snap", &(editorState.snap[0]));
            break;
        }
        case SCALE_MODE: {
            if(isChildWithParent) {
                ImGui::Text("World Scale X: %s", std::to_string(scale.x).c_str());
                ImGui::Text("World Scale Y: %s", std::to_string(scale.y).c_str());
                ImGui::Text("World Scale Z: %s", std::to_string(scale.z).c_str());
            }

            glm::vec3 tempScale = isChildWithParent ? scaleSingle : scale;
            updated = ImGui::DragFloat("Scale X", &(tempScale.x), 0.01, 0.01f, 10.0f) || updated;
            updated = ImGui::DragFloat("Scale Y", &(tempScale.y), 0.01, 0.01f, 10.0f) || updated;
            if(!is2D) {
                updated = ImGui::DragFloat("Scale Z", &(tempScale.z), 0.01, 0.01f, 10.0f) || updated;
                ImGui::NewLine();
                updated = ImGui::SliderFloat("Massive Scale X", &(tempScale.x), 0.01f, 100.0f) || updated;
                updated = ImGui::SliderFloat("Massive Scale Y", &(tempScale.y), 0.01f, 100.0f) || updated;
                updated = ImGui::SliderFloat("Massive Scale Z", &(tempScale.z), 0.01f, 100.0f) || updated;
            }
            //it is possible to enter any scale now. If user enters 0, don't update
            if ((updated || crudeUpdated) && (tempScale.x != 0.0f && tempScale.y != 0.0f && tempScale.z != 0.0f)) {
                setScale(tempScale);
            }
            ImGui::NewLine();
            ImGui::Checkbox("##SnapCheckBoxScaleSnap", &(editorState.useSnap));
            ImGui::SameLine();
            ImGui::InputFloat("Scale Snap", &(editorState.snap[0]));
            break;
        }
    }
    updated = addImGuizmoElements(editorState, cameraMatrix, perspectiveMatrix, is2D) || updated;
    return updated || crudeUpdated;
}


bool Transformation::addImGuizmoElements(const ImGuizmoState &editorState, const glm::mat4 &cameraMatrix,
                                         const glm::mat4 &perspectiveMatrix, bool is2D) {
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
    //before doing anything, make sure the values are actual.
    this->getWorldTransform();

    objectMatrix = glm::translate(glm::mat4(1.0f), translate) *
                   glm::mat4_cast(orientation) *
                   glm::scale(glm::mat4(1.0f), scale);


    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::LOCAL);
    if(is2D) {
        mCurrentGizmoMode = ImGuizmo::WORLD;
        ImGuizmo::SetOrthographic(true);
    } else {
        ImGuizmo::SetOrthographic(false);
    }
    float tempSnap[3] = {editorState.snap[0], editorState.snap[1], editorState.snap[2] };
    glm::mat4 deltaMatrix;
    // Save original matrix before Manipulate overwrites objectMatrix in place.
    const glm::mat4 originalObjectMatrix = objectMatrix;
    ImGuizmo::Manipulate(glm::value_ptr(cameraMatrix), glm::value_ptr(perspectiveMatrix), mCurrentGizmoOperation, mCurrentGizmoMode, glm::value_ptr(objectMatrix), glm::value_ptr(deltaMatrix), editorState.useSnap ? &(tempSnap[0]) : NULL);
    //now we should have object matrix updated, update the object
    glm::vec3 tempTranslate, tempScale, tempSkew;
    glm::vec4 tempPerspective;
    glm::quat tempOrientation;
    glm::decompose(deltaMatrix, tempScale, tempOrientation, tempTranslate, tempSkew, tempPerspective);
    switch (mCurrentGizmoOperation) {
        case ImGuizmo::TRANSLATE:
            if(tempTranslate != glm::vec3(0,0,0)) {
                addTranslate(tempTranslate);
                return true;
            }
            break;
        case ImGuizmo::ROTATE:
            {
                // deltaMatrix_math = M * R_rot * M^-1. Recover R_rot by the inverse conjugation.
                glm::mat4 localDeltaMatrix = glm::inverse(originalObjectMatrix) * deltaMatrix * originalObjectMatrix;
                glm::vec3 ls, lt, lsk;
                glm::vec4 lp;
                glm::quat localOrientation;
                glm::decompose(localDeltaMatrix, ls, localOrientation, lt, lsk, lp);
                if(localOrientation != glm::quat(1,0,0,0)) {
                    if(is2D) {
                        localOrientation.x = 0;
                        localOrientation.y = 0;
                        localOrientation = glm::normalize(localOrientation);
                    }
                    addOrientation(localOrientation);
                    return true;
                }
            }
            break;
        case ImGuizmo::SCALE:
            if(tempScale != glm::vec3(1,1,1)) {
                addScale(tempScale);
                return true;
            }
            break;
        case ImGuizmo::BOUNDS://not used
        case ImGuizmo::TRANSLATE_X://not used
        case ImGuizmo::TRANSLATE_Z://not used
        case ImGuizmo::TRANSLATE_Y://not used
        case ImGuizmo::ROTATE_X://not used
        case ImGuizmo::ROTATE_Y://not used
        case ImGuizmo::ROTATE_Z://not used
        case ImGuizmo::ROTATE_SCREEN://not used
        case ImGuizmo::SCALE_X://not used
        case ImGuizmo::SCALE_Y://not used
        case ImGuizmo::SCALE_Z://not used
        case ImGuizmo::SCALE_XU://not used
        case ImGuizmo::SCALE_YU://not used
        case ImGuizmo::SCALE_ZU://not used
        case ImGuizmo::SCALEU://not used
        case ImGuizmo::UNIVERSAL://not used
            break;
    }
    return false;
}

void Transformation::getDifferenceAddition(const Transformation &otherTransformation, glm::vec3 &translate,
                                           glm::vec3 &scale, glm::quat &rotation) const {

    translate = otherTransformation.translate - this->translate;
    scale = otherTransformation.scale / this->scale;
    rotation = this->orientation;
    rotation = glm::inverse(rotation);
    rotation = rotation * otherTransformation.orientation;
}

void Transformation::getDifferenceStacked(const Transformation &otherTransformation, glm::vec3 &translate,
                                          glm::vec3 &scale, glm::quat &rotation) const {
    //first, find out, what would convert this, to other. Simple substract won't work because these will stack, not add
    glm::mat4 currentWT = generateRawWorldTransformWithOrWithoutParent();
    glm::mat4 otherWt = otherTransformation.generateRawWorldTransformWithOrWithoutParent();
    glm::mat4 differenceWT = glm::inverse(currentWT) * otherWt;

    glm::vec3 temp1;
    glm::vec4 temp2;


    glm::decompose(differenceWT, scale, rotation, translate, temp1, temp2);
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

bool Transformation::serializeLocal(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode) const {
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

    glm::vec3 scale(1.0f, 1.0f, 1.0f);
    glm::vec3 translate(0.0f, 0.0f, 0.0f);
    glm::quat orientation(1.0f, 0.0f, 0.0f, 0.0f);

    transformationAttribute =  transformationNode->FirstChildElement("Scale");
    if (transformationAttribute == nullptr) {
        std::cout << "Transformation does not have scale." << std::endl;
    } else {
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("X");
        if(transformationAttributeAttribute != nullptr) {
            scale.x = std::stof(transformationAttributeAttribute->GetText());
        }
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("Y");
        if(transformationAttributeAttribute != nullptr) {
            scale.y = std::stof(transformationAttributeAttribute->GetText());
        }
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("Z");
        if(transformationAttributeAttribute != nullptr) {
            scale.z = std::stof(transformationAttributeAttribute->GetText());
        }
    }

    transformationAttribute =  transformationNode->FirstChildElement("Translate");
    if (transformationAttribute == nullptr) {
        std::cout << "Transformation does not have translate." << std::endl;
    } else {
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("X");
        if(transformationAttributeAttribute != nullptr) {
            translate.x = std::stof(transformationAttributeAttribute->GetText());
        }
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("Y");
        if(transformationAttributeAttribute != nullptr) {
            translate.y = std::stof(transformationAttributeAttribute->GetText());
        }
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("Z");
        if(transformationAttributeAttribute != nullptr) {
            translate.z = std::stof(transformationAttributeAttribute->GetText());
        }
    }

    transformationAttribute =  transformationNode->FirstChildElement("Rotate");
    if (transformationAttribute == nullptr) {
        std::cout << "Transformation does not have Rotation." << std::endl;
    } else {
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("X");
        if(transformationAttributeAttribute != nullptr) {
            orientation.x = std::stof(transformationAttributeAttribute->GetText());
        }
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("Y");
        if(transformationAttributeAttribute != nullptr) {
            orientation.y = std::stof(transformationAttributeAttribute->GetText());
        }
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("Z");
        if(transformationAttributeAttribute != nullptr) {
            orientation.z = std::stof(transformationAttributeAttribute->GetText());
        }
        transformationAttributeAttribute =  transformationAttribute->FirstChildElement("W");
        if(transformationAttributeAttribute != nullptr) {
            orientation.w = std::stof(transformationAttributeAttribute->GetText());
        }
    }

    // Apply all values at once — a single propagateUpdate fires with a consistent TRS state.
    setTransformations(translate, scale, orientation);
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
