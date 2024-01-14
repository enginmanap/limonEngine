//
// Created by engin on 8.03.2018.
//

#ifndef LIMONENGINE_GAMEOBJECT_H
#define LIMONENGINE_GAMEOBJECT_H

#include <string>

#include "API/LimonAPI.h"
#include "Editor/ImGuiRequest.h"
#include "Editor/ImGuiResult.h"

/**
 * This class is used to provide a polymorphic way of determining type and name of the object.
 */
class GameObject {
public:


    enum ObjectTypes { PLAYER, LIGHT, MODEL, SKYBOX, TRIGGER, GUI_TEXT, GUI_IMAGE, GUI_BUTTON, GUI_ANIMATION, SOUND, MODEL_GROUP, PARTICLE_EMITTER, GPU_PARTICLE_EMITTER };

    virtual ObjectTypes getTypeID() const = 0;
    virtual std::string getName() const = 0;
    virtual ImGuiResult addImGuiEditorElements(const ImGuiRequest &request [[gnu::unused]]) {ImGuiResult imGuiResult; return imGuiResult;};

    virtual void interact(LimonAPI *limonAPI [[gnu::unused]], std::vector<LimonTypes::GenericParameter> &interactionData [[gnu::unused]]) {};

    virtual uint32_t getWorldObjectID() const = 0;
    virtual ~GameObject() = default;
};


#endif //LIMONENGINE_GAMEOBJECT_H
