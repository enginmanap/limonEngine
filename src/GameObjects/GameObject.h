//
// Created by engin on 8.03.2018.
//

#ifndef LIMONENGINE_GAMEOBJECT_H
#define LIMONENGINE_GAMEOBJECT_H

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../GamePlay/LimonAPI.h"

/**
 * This class is used to provide a polymorphic way of determining type and name of the object.
 */
class GameObject {
public:
    /**
     * Since the world is not passed with ImGui request, changes to world must be returned using this struct
     */
    struct ImGuiResult {
        bool addAI = false;
        bool removeAI = false;
        bool updated = false;
        bool remove = false; //If removal requested
        std::string actorTypeName;
    };

    struct ImGuiRequest {
        const glm::mat4& perspectiveCameraMatrix;
        const glm::mat4 ortogonalCameraMatrix = glm::lookAt(glm::vec3(0,0,1), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4& perspectiveMatrix;
        const glm::mat4& ortogonalMatrix;

        const uint32_t& screenHeight;
        const uint32_t& screenWidth;

        LimonAPI* limonAPI = nullptr;

        ImGuiRequest(const glm::mat4 &perspectiveCameraMatrix, const glm::mat4 &perspectiveMatrix,
                     const glm::mat4 &ortogonalMatrix, const uint32_t &screenHeight, const uint32_t &screenWidth, LimonAPI* limonAPI)
                : perspectiveCameraMatrix(perspectiveCameraMatrix), perspectiveMatrix(perspectiveMatrix),
                  ortogonalMatrix(ortogonalMatrix), screenHeight(screenHeight), screenWidth(screenWidth), limonAPI(limonAPI) {}
    };

    enum ObjectTypes { PLAYER, LIGHT, MODEL, SKYBOX, TRIGGER, GUI_TEXT, GUI_IMAGE, GUI_BUTTON, GUI_ANIMATION, SOUND, MODEL_GROUP };

    virtual ObjectTypes getTypeID() const = 0;
    virtual std::string getName() const = 0;
    virtual ImGuiResult addImGuiEditorElements(const ImGuiRequest &request __attribute((unused))) {ImGuiResult imGuiResult; return imGuiResult;};

    virtual void interact(LimonAPI *limonAPI __attribute((unused)), std::vector<LimonAPI::ParameterRequest> &interactionData __attribute((unused))) {};

    virtual uint32_t getWorldObjectID() = 0;
    virtual ~GameObject() {};
};


#endif //LIMONENGINE_GAMEOBJECT_H
