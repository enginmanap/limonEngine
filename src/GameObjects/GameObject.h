//
// Created by engin on 8.03.2018.
//

#ifndef LIMONENGINE_GAMEOBJECT_H
#define LIMONENGINE_GAMEOBJECT_H

#include <string>
#include <glm/glm.hpp>
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
    };

    struct ImGuiRequest {
        const glm::mat4& perspectiveCameraMatrix;
        const glm::mat4& ortogonalCameraMatrix;
        const glm::mat4& perspectiveMatrix;
        const glm::mat4& ortogonalMatrix;

        const uint32_t screenHeight;
        const uint32_t screenWidth;
    };

    enum ObjectTypes { PLAYER, LIGHT, MODEL, SKYBOX, TRIGGER, GUI_TEXT };

    virtual ObjectTypes getTypeID() const = 0;
    virtual std::string getName() const = 0;
    virtual ImGuiResult addImGuiEditorElements(const ImGuiRequest &request __attribute((unused))) {ImGuiResult imGuiResult; return imGuiResult;};

    virtual uint32_t getWorldObjectID() = 0;
    virtual ~GameObject() {};
};


#endif //LIMONENGINE_GAMEOBJECT_H
