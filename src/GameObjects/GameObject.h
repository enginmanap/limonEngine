//
// Created by engin on 8.03.2018.
//

#ifndef LIMONENGINE_GAMEOBJECT_H
#define LIMONENGINE_GAMEOBJECT_H

#include <string>

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
    };

    enum ObjectTypes { PLAYER, LIGHT, MODEL, SKYBOX, TRIGGER };

    virtual ObjectTypes getTypeID() const = 0;
    virtual std::string getName() const = 0;
    virtual ImGuiResult addImGuiEditorElements(const glm::mat4 &cameraMatrix __attribute__((unused)), const glm::mat4 &perspectiveMatrix __attribute__((unused))) {ImGuiResult imGuiResult; return imGuiResult;};

    virtual uint32_t getWorldObjectID() = 0;
    virtual ~GameObject() {};
};


#endif //LIMONENGINE_GAMEOBJECT_H
