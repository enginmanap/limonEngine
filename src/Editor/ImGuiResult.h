//
// Created by engin on 14/01/2024.
//

#ifndef IMGUIRESULT_H
#define IMGUIRESULT_H

#include <string>
#include <cstdint>
#include <memory>

class Material;

/**
 * Since the world is not passed with ImGui request, changes to world must be returned using this struct
 */
struct ImGuiResult {
    bool addAI = false;
    bool removeAI = false;
    bool updated = false;
    bool remove = false; //If removal requested
    bool materialChanged = false;
    //a material editor widget changed a value this frame. The pane uses it to split an asset owned material
    //off into a world owned one before the edit can reach anybody else
    bool materialDirty = false;
    //the copy and its edit window are owned by the Editor, these only say what the user asked for
    int32_t alterMaterialForMeshIndex = -1;//user pressed "Alter material for this model" for this mesh
    bool revertAlteredMaterial = false;
    //user picked a mesh, so the material list can follow along. Null means the pick did not change
    std::shared_ptr<const Material> selectedMeshMaterial = nullptr;
    bool recalculateTranslateForOnTop = false;
    bool putOnTop = false;
    bool flipChanged = false;
    bool massChanged = false;
    bool boneClicked = false;//user clicked inside the bone-exposure preview image; pixel coords below are local to it
    float boneClickPixelX = 0.0f;
    float boneClickPixelY = 0.0f;
    std::string actorTypeName;
    std::string newFlipAxes;
};



#endif //IMGUIRESULT_H
