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
#include "limonAPI/ActorInterface.h"

class World;
class GraphicsInterface;
class AssetManager;
class GraphicsInterface;
class ALHelper;
class InputHandler;
class Model;

class WorldLoader {
public:
    struct ObjectInformation {
        Model* model = nullptr;
        ActorInterface* modelActor = nullptr;
        bool isAIGridStartPointSet = false;
        glm::vec3 aiGridStartPoint = glm::vec3(0,0,0);
    };

private:

    OptionsUtil::Options *options;
    GraphicsInterface* graphicsWrapper;
    ALHelper *alHelper;
    std::shared_ptr<AssetManager> assetManager;
    InputHandler* inputHandler;

    World *loadMapFromXML(const std::string &worldFileName, LimonAPI *limonAPI) const;
    bool loadObjectGroupsFromXML(tinyxml2::XMLNode *worldNode, World *world, LimonAPI *limonAPI,
            std::vector<Model*> &notStaticObjects, bool &isAIGridStartPointSet, glm::vec3 &aiGridStartPoint) const;
    bool loadObjectsFromXML(tinyxml2::XMLNode *objectsNode, World *world, LimonAPI *limonAPI) const;
    bool loadSkymap(tinyxml2::XMLNode *skymapNode, World* world) const;
    bool loadLights(tinyxml2::XMLNode *lightsNode, World* world) const;
    bool loadParticleEmitters(tinyxml2::XMLNode *EmittersNode, World* world) const;
    bool loadGPUParticleEmitters(tinyxml2::XMLNode *GPUEmittersNode, World* world) const;
    bool loadAnimations(tinyxml2::XMLNode *worldNode, World *world) const;
    bool loadTriggers(tinyxml2::XMLNode *worldNode, World *world) const;
    bool loadOnLoadActions(tinyxml2::XMLNode *worldNode, World *world) const;
    bool loadOnLoadAnimations(tinyxml2::XMLNode *worldNode, World *world) const;
    bool loadGUILayersAndElements(tinyxml2::XMLNode *worldNode, World *world) const;
    bool loadMaterials(tinyxml2::XMLNode *worldNode, World *world) const;

    void attachedAPIMethodsToWorld(World *world, LimonAPI *limonAPI) const;

public:
    WorldLoader(std::shared_ptr<AssetManager> assetManager, InputHandler *inputHandler, OptionsUtil::Options *options);

    std::unique_ptr<std::string> getLoadingImage(const std::string &worldFile) const;

    World *loadWorld(const std::string &worldFile, LimonAPI *limonAPI) const;

    static std::vector<std::unique_ptr<ObjectInformation>> loadObject( std::shared_ptr<AssetManager> assetManager, tinyxml2::XMLElement *objectNode,
                                                                       std::unordered_map<std::string, std::shared_ptr<Sound>> &requiredSounds, LimonAPI *limonAPI,
                                                                       PhysicalRenderable *parentObject);


    static bool loadVec3(tinyxml2::XMLNode* vectorNode, glm::vec3& vector);
};


#endif //LIMONENGINE_WORLDLOADER_H
