//
// Created by engin on 28/09/2021.
//

#ifndef LIMONENGINE_EDITOR_H
#define LIMONENGINE_EDITOR_H

#include "ImGui/imgui.h"
#include <set>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include "Editor/ImGuiRequest.h"

#define MAX_PRELOAD_MODEL_COUNT_EDITOR 10
class InputHandler;
class GameObject;
class World;
class PhysicalRenderable;
class ModelAsset;
class Texture;
class GraphicsPipelineStage;
class Model;
class GraphicsProgram;
class ImGuiImageWrapper;
class Material;
class ClosestNotMeConvexResultCallback;
class NodeGraph;
class PipelineExtension;
class IterationExtension;
class ImGuiHelper;

namespace EditorNS {
    //This is used as a global variable store. For multiple windows, ImGui doesn't provide anything else
    extern std::shared_ptr<const Material> selectedMeshesMaterial;
    extern std::shared_ptr<Material> selectedFromListMaterial;
}


class Editor {
    World* world;
    std::shared_ptr<Texture> colorTexture;
    std::shared_ptr<Texture> depthTexture;
    std::unique_ptr<GraphicsPipelineStage> backgroundRenderStage;

    std::vector<Model*> modelQueue;
    std::set<uint32_t> modelIdSet;
    Model* getModelAndMoveToEnd(const std::string& modelFilePath);
    Model *createRenderAndAddModelToLRU(const std::string &modelFileName, const glm::vec3 &newObjectPosition, std::shared_ptr<GraphicsProgram> graphicsProgram);
    ImGuiImageWrapper* wrapper = nullptr;

    std::unordered_map<std::string, std::shared_ptr<ModelAsset>> modelAssetsWaitingCPULoad;
    std::unordered_map<std::string, std::shared_ptr<ModelAsset>> modelAssetsPreloaded;

public:
    bool showNodeGraph = false;
    PipelineExtension *pipelineExtension = nullptr;
    IterationExtension *iterationExtension = nullptr;
    NodeGraph* nodeGraph = nullptr;
    ImGuiHelper *imgGuiHelper = nullptr;
    ImGuiRequest* request = nullptr;

    GameObject* pickedObject = nullptr;
    uint32_t pickedObjectID = 0xFFFFFFFF;
    Model* objectToAttach = nullptr;

    char worldSaveNameBuffer[256] = {0};
    char quitWorldNameBuffer[256] = {0};
    char extensionNameBuffer[32] = {0};

    Editor(World* world);
    ~Editor();
    void renderEditor(std::shared_ptr<GraphicsProgram> graphicsProgram);

    void addGUITextControls();
    void addGUIImageControls();
    void addGUIButtonControls();
    void addGUIAnimationControls();
    void addGUILayerControls();
    void addParticleEmitterEditor();
    void addSkyBoxControls();
    void drawNodeEditor();
    void createNodeGraph();

    void update(InputHandler &inputHandler);

private:
    void buildTreeFromAllGameObjects();
    std::unique_ptr<ClosestNotMeConvexResultCallback> convexSweepTestDown(Model * selectedObject) const;
    void addAnimationDefinitionToEditor();
    void createObjectTreeRecursive(PhysicalRenderable *physicalRenderable, uint32_t pickedObjectID,
                                          ImGuiTreeNodeFlags nodeFlags, ImGuiTreeNodeFlags leafFlags,
                                          std::vector<uint32_t> parentage);

    void renderSelectedObject(Model* model, std::shared_ptr<GraphicsProgram> graphicsProgram) const;

    void setTransformToModel(Model *model, const glm::vec3 &newObjectPosition);
};




#endif //LIMONENGINE_EDITOR_H
