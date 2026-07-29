//
// Created by engin on 28/09/2021.
//

#ifndef LIMONENGINE_EDITOR_H
#define LIMONENGINE_EDITOR_H

#include "ImGui/imgui.h"
#include <algorithm>
#include <set>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <glm/glm.hpp>
#include "Editor/ImGuiRequest.h"
#include "limonAPI/LimonTypes.h"

#define MAX_PRELOAD_MODEL_COUNT_EDITOR 10
class InputHandler;
class Attachable;
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
class NodeType;
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

    // We can't re-use the background renderer state for animation/bone preview, as editor might have both visible at the same time.
    struct BonePreviewState {
        // We can't use the main context, as it is mid frame, so we create a separate one for this.
        // We do reuse the font atlas, so it is a very lightweight thingy.
        ImGuiContext* imGuiContext = nullptr;

        std::shared_ptr<Texture> colorTexture;
        std::shared_ptr<Texture> depthTexture;
        std::unique_ptr<GraphicsPipelineStage> renderStage;
        ImGuiImageWrapper* wrapper = nullptr;
        // We can't use the actual rig-id, because that would corrupt the models state, so we will reserve another ID and use that.
        uint32_t rigId = 0;
        uint64_t startWallTime = 0;//wall time when the currently-previewed model/animation was first shown, for looping playback
        uint32_t modelObjectID = 0xFFFFFFFF;//tracks which model+animation startWallTime belongs to
        std::string animationName;
    };
    BonePreviewState bonePreview;

    // We might or might not render add object preview, same with animation/bone preview. If neither rendered, we don't need to
    // clean up/restore so we flag if any of them did render.
    // TODO: this flag might not be needed, or we might have a better solution, needs re-check
    bool offscreenPreviewRenderedThisFrame = false;
    void beginOffscreenModelPreview(GraphicsPipelineStage* targetStage, std::shared_ptr<GraphicsProgram> graphicsProgram);
    void finalizeOffscreenModelPreviews(std::shared_ptr<GraphicsProgram> graphicsProgram);

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
    GameObject* pendingPickedObject = nullptr;
    bool hasPendingPick = false;
    uint32_t pickedObjectID = 0xFFFFFFFF;
    Attachable* objectToAttach = nullptr;

    char worldSaveNameBuffer[256] = {0};
    char quitWorldNameBuffer[256] = {0};
    char objectFilterBuffer[128] = {0};
    char extensionNameBuffer[32] = {0};
    std::string shownExtensionParametersName;//tracks which extension's parameters are currently mirrored in startingPlayer.parameters

    char cameraExtensionNameBuffer[32] = {0};//selected csm type in the "Add Camera Rig" creation combo

    char nodeGraphFileNameBuffer[512] = {0};//prefilled with currently loaded node graph file, editable to load a different one

    Editor(World* world);
    ~Editor();
    bool generateEditorElementsForParameters(std::vector<LimonTypes::GenericParameter> &runParameters, uint32_t index);
    ImGuiImageWrapper* renderBonePreview(Model* model, std::shared_ptr<GraphicsProgram> graphicsProgram);
    void renderEditor(std::shared_ptr<GraphicsProgram> graphicsProgram);
    void applyPendingPick();

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
                                          std::vector<uint32_t> parentage,
                                          const std::string& filterText,
                                          const std::unordered_set<uint32_t>& filteredVisibleIDs);

    bool buildFilteredVisibleIDs(PhysicalRenderable *physicalRenderable, const std::string& filterText,
                                 std::unordered_set<uint32_t>& visibleIDs);

    void renderSelectedObject(Model* model, std::shared_ptr<GraphicsProgram> graphicsProgram);

    void bakeSkeletonOverlay(Model* model, const std::vector<glm::mat4> &jointTransforms,
                             const glm::mat4 &previewCameraMatrix, const glm::mat4 &previewProjectionMatrix,
                             std::shared_ptr<GraphicsProgram> graphicsProgram);

    void setTransformToModel(Model *model, const glm::vec3 &newObjectPosition);

    void loadNodeGraphFile(const std::string &fileName);
    std::vector<NodeType*> buildAvailableNodeTypes();
};




#endif //LIMONENGINE_EDITOR_H
