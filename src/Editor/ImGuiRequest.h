//
// Created by engin on 14/01/2024.
//

#ifndef IMGUIREQUEST_H
#define IMGUIREQUEST_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <functional>
#include <memory>
#include <vector>
#include <cstdint>
#include "limonAPI/LimonTypes.h"
class Camera;
class ImGuiHelper;
class Material;
class Model;
class ImGuiImageWrapper;

//Renders editable ImGui widgets for a parameter list and returns whether all parameters are set.
//Implemented by the Editor; injected here as a callback so callers need not depend on the Editor type.
typedef std::function<bool(std::vector<LimonTypes::GenericParameter> &, uint32_t)> GenerateEditorElementsCallback;

// Renders the selected model with its animation (or t-pose/base), but instead of using game time, uses wall time.
// The skeleton overlay (lines+joints, selection highlight) is baked into the same returned image by the editor
// system, so the caller only ever gets one finished texture to display -- no camera/joint-transform data crosses
// this boundary.
typedef std::function<ImGuiImageWrapper*(Model*)> RenderBonePreviewCallback;

struct ImGuiRequest {
    const glm::mat4& perspectiveCameraMatrix;
    const glm::mat4 orthogonalCameraMatrix = glm::lookAt(glm::vec3(0, 0, 1), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4& perspectiveMatrix;
    const glm::mat4& orthogonalMatrix;

    const uint32_t& screenHeight;
    const uint32_t& screenWidth;

    const Camera* playerCamera;
    GenerateEditorElementsCallback generateEditorElementsForParameters;
    RenderBonePreviewCallback renderBonePreview;
    ImGuiHelper* imgGuiHelper = nullptr;
    //the material the Editor is currently altering for the picked object, so the object pane can draw its
    //widgets in place. Editor owns it and its edit window, nothing here holds it beyond the frame
    std::shared_ptr<Material> alteredMaterial = nullptr;
    //whatever is selected in the material list, for "Switch material" to apply. Not an edit, just a pick
    std::shared_ptr<Material> materialSelectedInList = nullptr;

    ImGuiRequest(const glm::mat4 &perspectiveCameraMatrix, const glm::mat4 &perspectiveMatrix,
                 const glm::mat4 &orthogonalMatrix, const uint32_t &screenHeight, const uint32_t &screenWidth,
                 const Camera* playerCamera, GenerateEditorElementsCallback generateEditorElementsForParameters,
                 RenderBonePreviewCallback renderBonePreview, ImGuiHelper* imgGuiHelper)
            : perspectiveCameraMatrix(perspectiveCameraMatrix), perspectiveMatrix(perspectiveMatrix),
              orthogonalMatrix(orthogonalMatrix), screenHeight(screenHeight), screenWidth(screenWidth),
              playerCamera(playerCamera), generateEditorElementsForParameters(std::move(generateEditorElementsForParameters)),
              renderBonePreview(std::move(renderBonePreview)), imgGuiHelper(imgGuiHelper) {}
};



#endif //IMGUIREQUEST_H
