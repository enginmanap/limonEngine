//
// Created by engin on 13.05.2018.
//

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
