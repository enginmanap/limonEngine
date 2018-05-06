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
    enum EditorModes {ROTATE_MODE, TRANSLATE_MODE, SCALE_MODE};

    struct GizmoRequest {
        bool isRequested = false;
        bool useSnap = false;
        float snap[3] = {1.0f, 1.0f, 1.0f};
        EditorModes mode = TRANSLATE_MODE; //translate is best, because it allows direct use for Light
        bool isEdited = false; //FIXME we should return this flag only, and gizmo request should be a reference parameter.
    };
    enum ObjectTypes { PLAYER, LIGHT, MODEL, SKYBOX };

    virtual ObjectTypes getTypeID() const = 0;
    virtual std::string getName() const = 0;
    virtual GizmoRequest addImGuiEditorElements() {GizmoRequest gr; return gr;};

    virtual uint32_t getWorldObjectID() = 0;
    virtual ~GameObject() {};
};


#endif //LIMONENGINE_GAMEOBJECT_H
