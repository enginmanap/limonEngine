//
// Created by engin on 28/09/2021.
//

#ifndef LIMONENGINE_EDITOR_H
#define LIMONENGINE_EDITOR_H

#include "ImGui/imgui.h"
#include <set>
#define MAX_PRELOAD_MODEL_COUNT_EDITOR 10
class World;
class PhysicalRenderable;
class ModelAsset;
class Texture;
class GraphicsPipelineStage;
class Model;
class GraphicsProgram;
class ImGuiImageWrapper;
class Material;

namespace EditorNS {
    //This is used as a global variable store. For multiple windows, ImGui doesn't provide anything else
    extern std::shared_ptr<const Material> selectedMaterial;
}
class Editor {
    World* world;
    std::shared_ptr<Texture> colorTexture;
    std::shared_ptr<Texture> depthTexture;
    std::unique_ptr<GraphicsPipelineStage> backgroundRenderStage;
    std::shared_ptr<GraphicsProgram> graphicsProgram;

    std::vector<Model*> modelQueue;
    std::set<uint32_t> modelIdSet;
    Model* getModelAndMoveToEnd(const std::string& modelFilePath);
    Model *createRenderAndAddModelToLRU(const std::string &modelFileName, const glm::vec3 &newObjectPosition);
    ImGuiImageWrapper* wrapper = nullptr;
public:
    Editor(World* world);
    void renderEditor();

private:
    std::unordered_map<std::string, std::shared_ptr<ModelAsset>> modelAssetsWaitingCPULoad;
    std::unordered_map<std::string, std::shared_ptr<ModelAsset>> modelAssetsPreloaded;
    void buildTreeFromAllGameObjects();

    void addAnimationDefinitionToEditor();
    void createObjectTreeRecursive(PhysicalRenderable *physicalRenderable, uint32_t pickedObjectID,
                                          ImGuiTreeNodeFlags nodeFlags, ImGuiTreeNodeFlags leafFlags,
                                          std::vector<uint32_t> parentage);

    void renderSelectedObject(Model* model);

    void setTransformToModel(Model *model, const glm::vec3 &newObjectPosition);
};


#endif //LIMONENGINE_EDITOR_H
