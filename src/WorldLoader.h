//
// Created by engin on 10.03.2018.
//

#ifndef LIMONENGINE_WORLDLOADER_H
#define LIMONENGINE_WORLDLOADER_H

#include <Material.h>
#include <string>
#include <vector>
#include <tinyxml2.h>

#include "ALHelper.h"
#include "InputHandler.h"
#include "limonAPI/LimonAPI.h"
#include "GameObjects/Sound.h"
#include "GameObjects/Model.h"
#include "limonAPI/ActorInterface.h"
#include "Profiler/ProfilerSystem.h"

class World;
class GraphicsInterface;
class AssetManager;
class GraphicsInterface;
class ALHelper;
class InputHandler;
class Model;
class Attachable;
class FrameTimeTracker;
struct FrameResourceHandles;

struct PackagingCallbacks;

class WorldLoader {
public:
    struct ObjectInformation {
        Model* model = nullptr;
        ActorInterface* modelActor = nullptr;
        bool isAIGridStartPointSet = false;
        glm::vec3 aiGridStartPoint = glm::vec3(0,0,0);
    };

    struct PendingAttachment {
        Attachable* child;
        uint32_t parentID;
        int32_t parentBoneID;
        const char* childTypeName; // just for the "not found" log line
    };

private:
    // bone parent goes through setParentObject/addChild/setParentTransform (bone transforms are
    // identity at load, attachTo's world->local math would be wrong), everything else uses attachTo.
    // no ModelGroup branch here, only loadObjectsFromXML needs that.
    static void resolvePendingAttachments(World *world, const std::vector<PendingAttachment> &pending);

    //objects name their override materials by key in the world's <Materials>, so it is indexed before they load
    static void indexSavedMaterials(tinyxml2::XMLNode *worldNode, World *world);
    //before any texture loads, so every load in this map uses its choice
    static void loadTextureColorSpaces(tinyxml2::XMLNode *worldNode, World *world);
    static std::shared_ptr<Material> resolveSavedMaterial(World *world, const std::string &key);
    static std::vector<Model::MeshMaterialOverride> readMeshMaterialOverrides(World *world, tinyxml2::XMLElement *objectNode);

    //the document is the save version's, loadWorld picked the loader for it
    World *loadMapFromXML(tinyxml2::XMLDocument &xmlDoc, LimonAPI *limonAPI) const;
    bool loadObjectGroupsFromXMLV2(tinyxml2::XMLNode *worldNode, World *world) const;
    bool loadSkymap(tinyxml2::XMLNode *skymapNode, World* world) const;
    bool loadLights(tinyxml2::XMLNode *lightsNode, World* world) const;
    bool loadSounds(tinyxml2::XMLNode *worldNode, World* world) const;
    bool loadCameraRigs(tinyxml2::XMLNode *worldNode, World* world) const;
    bool loadParticleEmitters(tinyxml2::XMLNode *EmittersNode, World* world) const;
    bool loadGPUParticleEmitters(tinyxml2::XMLNode *GPUEmittersNode, World* world) const;
    bool loadAnimations(tinyxml2::XMLNode *worldNode, World *world) const;
    bool loadTriggers(tinyxml2::XMLNode *worldNode, World *world) const;
    bool loadOnLoadActions(tinyxml2::XMLNode *worldNode, World *world) const;
    bool loadOnLoadAnimations(tinyxml2::XMLNode *worldNode, World *world) const;
    bool loadGUILayersAndElements(tinyxml2::XMLNode *worldNode, World *world) const;
    bool loadMaterials(tinyxml2::XMLNode *worldNode, World *world) const;

protected:
    OptionsUtil::Options *options;
    GraphicsInterface* graphicsWrapper;
    ALHelper *alHelper;
    std::shared_ptr<AssetManager> assetManager;
    InputHandler* inputHandler;
    ProfilerSystem* profilerSystem;
    FrameTimeTracker* frameTimeTracker;
    const FrameResourceHandles* frameResourceHandles;
    const PackagingCallbacks *packagingCallbacks;

    //where older save versions differ, LegacyWorldLoaderV1 overrides these
    virtual void prepareDocument(tinyxml2::XMLNode *worldNode [[gnu::unused]]) const {}
    virtual bool loadObjectsFromXML(tinyxml2::XMLNode *worldNode, World *world, LimonAPI *limonAPI) const;
    virtual void loadPlayerAttachment(tinyxml2::XMLElement *playerNode [[gnu::unused]], World *world [[gnu::unused]], LimonAPI *limonAPI [[gnu::unused]]) const {}//saved as objects parented to the player

public:
    virtual ~WorldLoader() = default;

    WorldLoader(std::shared_ptr<AssetManager> assetManager, InputHandler *inputHandler, OptionsUtil::Options *options, ProfilerSystem* profilerSystem, FrameTimeTracker* frameTimeTracker, const FrameResourceHandles* frameResourceHandles,
                const PackagingCallbacks *packagingCallbacks);

    std::unique_ptr<std::string> getLoadingImage(const std::string &worldFile) const;

    World *loadWorld(const std::string &worldFile, LimonAPI *limonAPI) const;

    static std::vector<std::unique_ptr<ObjectInformation>> loadObjectV2(World *world, std::shared_ptr<AssetManager> assetManager, tinyxml2::XMLElement *objectNode,
                                                                        std::unordered_map<std::string, std::shared_ptr<Sound>> &requiredSounds, LimonAPI *limonAPI);
    static void loadObjectTags(tinyxml2::XMLElement *objectNode, Model *model);
};


#endif //LIMONENGINE_WORLDLOADER_H
