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

// Renders the selected model at the requested size, with its animation (or t-pose/base), but instead of using
// game time, uses wall time. A LOD level past 0 pins that level for this image, -1 renders as selection would.
// The skeleton overlay (lines+joints, selection highlight) is baked into the same returned image by the editor
// system, so the caller only ever gets one finished texture to display -- no camera/joint-transform data crosses
// this boundary.
typedef std::function<ImGuiImageWrapper*(Model*, int32_t, uint32_t, uint32_t)> RenderModelPreviewCallback;

//The model rendered at a LOD level's design size, next to the original at that same size. Judging "is this
//acceptable at 20 px" means looking at 20 px, so these come back small and are magnified by the panel.
struct LodComparisonImages {
    ImGuiImageWrapper* levelImage = nullptr;
    ImGuiImageWrapper* originalImage = nullptr;
};
typedef std::function<LodComparisonImages(Model*, int32_t, uint32_t)> RenderLodComparisonCallback;

struct ImGuiRequest {
    const glm::mat4& perspectiveCameraMatrix;
    const glm::mat4 orthogonalCameraMatrix = glm::lookAt(glm::vec3(0, 0, 1), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4& perspectiveMatrix;
    const glm::mat4& orthogonalMatrix;

    //by value, not by reference: Options returns these by value, so a reference member would dangle the moment
    //the request is built. It read as billions of metres in the LOD panel before anyone noticed
    const uint32_t screenHeight;
    const uint32_t screenWidth;

    const Camera* playerCamera;
    GenerateEditorElementsCallback generateEditorElementsForParameters;
    RenderModelPreviewCallback renderModelPreview;
    RenderLodComparisonCallback renderLodComparison;
    ImGuiHelper* imgGuiHelper = nullptr;
    //the material the Editor is currently altering for the picked object, so the object pane can draw its
    //widgets in place. Editor owns it and its edit window, nothing here holds it beyond the frame
    std::shared_ptr<Material> alteredMaterial = nullptr;
    //whatever is selected in the material list, for "Switch material" to apply. Not an edit, just a pick
    std::shared_ptr<Material> materialSelectedInList = nullptr;

    ImGuiRequest(const glm::mat4 &perspectiveCameraMatrix, const glm::mat4 &perspectiveMatrix,
                 const glm::mat4 &orthogonalMatrix, const uint32_t &screenHeight, const uint32_t &screenWidth,
                 const Camera* playerCamera, GenerateEditorElementsCallback generateEditorElementsForParameters,
                 RenderModelPreviewCallback renderModelPreview, RenderLodComparisonCallback renderLodComparison, ImGuiHelper* imgGuiHelper)
            : perspectiveCameraMatrix(perspectiveCameraMatrix), perspectiveMatrix(perspectiveMatrix),
              orthogonalMatrix(orthogonalMatrix), screenHeight(screenHeight), screenWidth(screenWidth),
              playerCamera(playerCamera), generateEditorElementsForParameters(std::move(generateEditorElementsForParameters)),
              renderModelPreview(std::move(renderModelPreview)), renderLodComparison(std::move(renderLodComparison)), imgGuiHelper(imgGuiHelper) {}
};



#endif //IMGUIREQUEST_H
