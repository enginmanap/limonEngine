//
// Created by engin on 13/11/2026.
//

#ifndef LIMONENGINE_EDITORCOMMAND_H
#define LIMONENGINE_EDITORCOMMAND_H

#include <cstdint>
#include <memory>
#include <string>

class Material;

//applied at the start of the next tick, so objects are named by ID, never by pointer: it may be gone by then
struct EditorCommand {
    enum class Kind {
        REMOVE_MODEL,
        ALTER_MESH_MATERIAL,
        REVERT_ALTERED_MATERIAL,
        RESET_MODEL_MATERIALS,
        SWITCH_MESH_MATERIAL,
        RESEAT_WORLD_MESHES,
        FLIP_MODEL
    };

    Kind kind;
    uint32_t objectID = 0;
    int32_t meshIndex = -1;
    std::string flipAxes;
    std::shared_ptr<Material> material;//switch: the new one, revert: the altered copy, reseat: the override
    std::shared_ptr<Material> sourceMaterial;//revert only
    std::shared_ptr<const Material> baseMaterial;//reseat only

    explicit EditorCommand(Kind kind) : kind(kind) {}
};

#endif //LIMONENGINE_EDITORCOMMAND_H
