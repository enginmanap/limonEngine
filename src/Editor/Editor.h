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
#include "Assets/Lod/LodLadder.h"
#include "Editor/ImGuiRequest.h"
#include "limonAPI/LimonTypes.h"

class InputHandler;
class Attachable;
class GameObject;
class World;
class PhysicalRenderable;
class ModelAsset;
class Texture;
class GraphicsPipelineStage;
class GraphicsPipeline;
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
class PreviewRenderer;

class Editor {
    World* world;

    //points every mesh in this world that uses baseMaterial at overrideMaterial
    void reseatWorldMeshes(const std::shared_ptr<const Material> &baseMaterial, const std::shared_ptr<Material> &overrideMaterial);
    void refreshTransparencyTagsOfMaterialUsers(const std::shared_ptr<const Material> &material);

    //splits an asset owned material off into a world owned one carrying the edit, on the first dirty frame
    std::shared_ptr<Material> ensureWorldOwnedMaterial(const std::shared_ptr<Material> &material);

    /**
     * The private material copy made by "Alter material for this model", and its open edit window.
     * Owned here rather than on Model, which has thousands of gameplay instances and no business
     * carrying editor state. Only one object is picked at a time, so one is enough.
     */
    struct AlteredMaterialEdit {
        uint32_t objectID = 0;
        int32_t meshIndex = -1;
        std::shared_ptr<Material> material = nullptr;
        /*
         * What the copy was made from, which is not necessarily the asset's material - the mesh may already
         * have been carrying an override. Closing without an edit has to put the mesh back on this one, and
         * "was it edited" is this one's content against the copy's.
         */
        std::shared_ptr<Material> sourceMaterial = nullptr;
    };
    AlteredMaterialEdit alteredMaterialEdit;

    /*
     * List Materials selection and the material it has armed. Members, not function local statics: an
     * Editor belongs to one World and worlds coexist, so a static here would let one world's editor commit
     * a material another world's editor armed, and reseat the wrong world's models.
     */
    //by registrationID, so a selection can never go stale because the material was edited
    uint32_t selectedRegistrationID = 0;
    //taken when the selection changes, so an asset owned material can be put back exactly when it splits
    std::shared_ptr<Material> selectedMaterialSnapshot = nullptr;
    std::shared_ptr<const Material> snapshotSourceMaterial = nullptr;

    //the material list's own copy of the registry, rebuilt only when the registry's version moves
    std::vector<std::pair<uint32_t, std::shared_ptr<Material>>> materialsSnapshot;
    uint32_t materialsSnapshotVersion = 0;
    //raw name, so a split keeps base_ next to its override instead of reordering the list
    static bool isMaterialListedBefore(const std::pair<uint32_t, std::shared_ptr<Material>> &left,
                                       const std::pair<uint32_t, std::shared_ptr<Material>> &right);
    //once per selection, or the list snaps back every time the user scrolls away from it
    uint32_t lastScrolledRegistrationID = 0;
    //what the material list currently has picked, handed to the object pane so "Switch material" can apply it
    std::shared_ptr<Material> materialSelectedInList = nullptr;
    //a mesh pick from the object pane, which the list follows. Carried on a member because the object pane
    //is drawn after the list, so it is consumed on the next frame
    std::shared_ptr<const Material> selectedMeshMaterial = nullptr;

    //creates the copy, points the mesh at it, opens its window
    void beginAlteredMaterialEdit(Model* model, int32_t meshIndex);

    //the Revert button: discard the alteration and put the mesh back on what it had
    void revertAlteredMaterialEdit();

    //the panel is going away for another reason. The alteration stands, we just let go of it
    void releaseAlteredMaterialEdit();

    //owns the Logger line buffer previewing a picked Emitter's spawn volume/velocity/despawn extents
    uint32_t particleEmitterDebugLineBufferId = 0;

    void clearParticleEmitterDebugBuffer();
    uint32_t lightDebugLineBufferId = 0;

    void clearLightDebugBuffer();

    // If selected object has line visualization, we this method will render it
    void renderSelectedObjectLineVisualization();

public:
    bool showNodeGraph = false;
    PipelineExtension *pipelineExtension = nullptr;
    IterationExtension *iterationExtension = nullptr;
    NodeGraph* nodeGraph = nullptr;
    ImGuiHelper *imgGuiHelper = nullptr;
    //Declared after imgGuiHelper on purpose: PreviewRenderer's constructor needs imgGuiHelper already built (to
    //share its font atlas for the bone-preview's dedicated ImGui context), and members are destroyed in reverse
    //declaration order, so previewRenderer (which tears down that dedicated context) is guaranteed to be
    //destroyed before imgGuiHelper is -- matching the ordering the code relied on before this was extracted.
    std::unique_ptr<PreviewRenderer> previewRenderer;
    ImGuiRequest* request = nullptr;

    /**
     * Asset side work that rewrites LOD ranges or writes a model back out. A render list carries a level index
     * into the mesh and the culling threads read the baked occluders, so none of this may happen while the
     * panel is being drawn. It is collected here and drained from applyDeferredAssetChanges, which World::play
     * calls before the tick starts, so the ranges are already rebuilt when culling reads them later that frame.
     */
    struct LodRegenerationRequest {
        std::shared_ptr<ModelAsset> modelAsset;
        bool clearOverrides = false;//back to the project options, otherwise recalibrate what it already has
        //-1 when this is not a triangle target request, otherwise the level whose count was typed
        int32_t triangleTargetLevel = -1;
        float triangleTargetRatio = 0.0f;
    };
    std::vector<LodRegenerationRequest> pendingLodRequests;
    std::set<std::shared_ptr<ModelAsset>> pendingAssetSaves;//a set, a world save and the panel can ask for the same asset
    bool worldSaveRequested = false;
    std::string pendingWorldSaveName;//copied at the click, the name box can change before the save runs
    bool modelConversionRequested = false;

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

    std::weak_ptr<GraphicsPipeline> fallbackPipeline;//editor only pipeline the world fell back to, reasons are shown while it is the one rendering
    std::vector<std::string> fallbackReasons;

    Editor(World* world);
    ~Editor();
    bool generateEditorElementsForParameters(std::vector<LimonTypes::GenericParameter> &runParameters, uint32_t index);

    //the LOD panel asks for these while it is being drawn, they run at the next frame boundary
    void requestLodRegeneration(std::shared_ptr<ModelAsset> modelAsset, bool clearOverrides);
    void requestLodTriangleTarget(std::shared_ptr<ModelAsset> modelAsset, size_t levelIndex, float targetRatio);
    void requestAssetSave(std::shared_ptr<ModelAsset> modelAsset);
    //assets first, so the log only says successful when the world and everything it uses reached disk
    void saveWorldFile(const std::string &worldName, bool allAssetsSaved);

    void requestModelConversionToBinary() {
        modelConversionRequested = true;
    }

    //World calls this between the render and the next culling pass, which is the only safe point for any of it
    void applyDeferredAssetChanges();
    ImGuiImageWrapper* renderModelPreview(Model* model, int32_t forcedLodLevel, uint32_t width, uint32_t height, std::shared_ptr<GraphicsProgram> graphicsProgram);
    LodComparisonImages renderLodComparison(Model* model, int32_t forcedLodLevel, uint32_t designSizePixels, std::shared_ptr<GraphicsProgram> graphicsProgram);
    void renderEditor(std::shared_ptr<GraphicsProgram> graphicsProgram);
    void applyPendingPick();

    void addGUITextControls();
    void addGUIImageControls();
    void addGUIButtonControls();
    void addGUIAnimationControls();
    void addGUILayerControls();
    void addParticleEmitterEditor(const glm::vec3 &newObjectPosition);
    void addSkyBoxControls();
    void drawNodeEditor();
    void createNodeGraph();
    //for when no render pipeline could be loaded, the node editor is where it gets rebuilt
    void showPipelineFallback(const std::shared_ptr<GraphicsPipeline> &fallbackPipeline, const std::vector<std::string> &reasons);

    void update(InputHandler &inputHandler);

    //called by World::switchPlayer when leaving editor mode
    void onEditorDisabled();

private:
    // Allows copying asseet trees (if recursive), uses clone method from attachable
    // Would fail if clone is not implemented, returns nullptr
    Attachable* copyAttachable(Attachable* source, bool recursive);
    // Reserves a new world ID for source and, if recursive, every descendant, without constructing
    // anything yet — so extension parameters can be remapped using a complete ID table.
    void buildCopyIDRemap(Attachable* source, bool recursive, std::unordered_map<uint32_t, uint32_t>& idRemap);
    Attachable* copyAttachableRecursive(Attachable* source, Attachable* newParent, bool recursive,
                                        const std::unordered_map<uint32_t, uint32_t>& idRemap);

    void buildTreeFromAllGameObjects();
    void putPickReferencedObjectButton(GameObject* referencedObject, const std::string& label);
    std::unique_ptr<ClosestNotMeConvexResultCallback> convexSweepTestDown(Model * selectedObject) const;
    void addAnimationDefinitionToEditor();
    void createObjectTreeRecursive(Attachable *attachable, uint32_t pickedObjectID,
                                          ImGuiTreeNodeFlags nodeFlags, ImGuiTreeNodeFlags leafFlags,
                                          std::vector<uint32_t> parentage,
                                          const std::string& filterText,
                                          const std::unordered_set<uint32_t>& filteredVisibleIDs);

    bool isListedUnderParent(const Attachable *attachable) const;

    bool buildFilteredVisibleIDs(Attachable *attachable, const std::string& filterText,
                                 std::unordered_set<uint32_t>& visibleIDs);

    void loadNodeGraphFile(const std::string &fileName);
    std::vector<NodeType*> buildAvailableNodeTypes();
};




#endif //LIMONENGINE_EDITOR_H
