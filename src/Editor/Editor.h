//
// Created by engin on 28/09/2021.
//

#ifndef LIMONENGINE_EDITOR_H
#define LIMONENGINE_EDITOR_H

#include "ImGui/imgui.h"
class World;
class PhysicalRenderable;


class Editor {
public:
    static void renderEditor(World& world);

private:
    static void buildTreeFromAllGameObjects(World& world);

    static void addAnimationDefinitionToEditor(World& world);
    static void createObjectTreeRecursive(World& world, PhysicalRenderable *physicalRenderable, uint32_t pickedObjectID,
                                          ImGuiTreeNodeFlags nodeFlags, ImGuiTreeNodeFlags leafFlags,
                                          std::vector<uint32_t> parentage);
};


#endif //LIMONENGINE_EDITOR_H
