//
// Created by engin on 14/01/2024.
//

#ifndef IMGUIREQUEST_H
#define IMGUIREQUEST_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <functional>
#include <vector>
#include <cstdint>
#include "limonAPI/LimonTypes.h"
class Camera;
class ImGuiHelper;

//Renders editable ImGui widgets for a parameter list and returns whether all parameters are set.
//Implemented by the Editor; injected here as a callback so callers need not depend on the Editor type.
typedef std::function<bool(std::vector<LimonTypes::GenericParameter> &, uint32_t)> GenerateEditorElementsCallback;

struct ImGuiRequest {
    const glm::mat4& perspectiveCameraMatrix;
    const glm::mat4 orthogonalCameraMatrix = glm::lookAt(glm::vec3(0, 0, 1), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4& perspectiveMatrix;
    const glm::mat4& orthogonalMatrix;

    const uint32_t& screenHeight;
    const uint32_t& screenWidth;

    const Camera* playerCamera;
    GenerateEditorElementsCallback generateEditorElementsForParameters;
    ImGuiHelper* imgGuiHelper = nullptr;

    ImGuiRequest(const glm::mat4 &perspectiveCameraMatrix, const glm::mat4 &perspectiveMatrix,
                 const glm::mat4 &orthogonalMatrix, const uint32_t &screenHeight, const uint32_t &screenWidth,
                 const Camera* playerCamera, GenerateEditorElementsCallback generateEditorElementsForParameters, ImGuiHelper* imgGuiHelper)
            : perspectiveCameraMatrix(perspectiveCameraMatrix), perspectiveMatrix(perspectiveMatrix),
              orthogonalMatrix(orthogonalMatrix), screenHeight(screenHeight), screenWidth(screenWidth),
              playerCamera(playerCamera), generateEditorElementsForParameters(std::move(generateEditorElementsForParameters)), imgGuiHelper(imgGuiHelper) {}
};



#endif //IMGUIREQUEST_H
