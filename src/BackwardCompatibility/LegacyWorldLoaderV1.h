//
// Created by Engin Manap on 11/11/2026.
//

#ifndef LIMONENGINE_LEGACYWORLDLOADERV1_H
#define LIMONENGINE_LEGACYWORLDLOADERV1_H

#include <map>
#include "../WorldLoader.h"

class ModelGroup;

//save version 1 worlds: children nested inside their parent, the player attachment saved with the player, sizes 2.54 times real
class LegacyWorldLoaderV1 : public WorldLoader {
    void prepareDocument(tinyxml2::XMLNode *worldNode) const override;
    bool loadObjectsFromXML(tinyxml2::XMLNode *worldNode, World *world, LimonAPI *limonAPI) const override;
    void loadPlayerAttachment(tinyxml2::XMLElement *playerNode, World *world, LimonAPI *limonAPI) const override;

    bool loadObjectGroups(tinyxml2::XMLNode *worldNode, World *world, LimonAPI *limonAPI,
                          std::vector<Model*> &notStaticObjects, bool &isAIGridStartPointSet, glm::vec3 &aiGridStartPoint) const;

    //last element in the vector is the parent of all
    static std::vector<std::unique_ptr<ObjectInformation>> loadObject(World *world, std::shared_ptr<AssetManager> assetManager, tinyxml2::XMLElement *objectNode,
                                                                       std::unordered_map<std::string, std::shared_ptr<Sound>> &requiredSounds, LimonAPI *limonAPI,
                                                                       PhysicalRenderable *parentObject);

    static ModelGroup *loadModelGroup(World *world, GraphicsInterface* graphicsWrapper, std::shared_ptr<AssetManager> assetManager, tinyxml2::XMLElement *modelGroupNode,
                                      std::unordered_map<std::string, std::shared_ptr<Sound>> &requiredSounds,
                                      std::map<uint32_t, ModelGroup *> &childGroups,
                                      std::vector<std::unique_ptr<ObjectInformation>> &childObjects, LimonAPI *limonAPI,
                                      ModelGroup *parentGroup);

public:
    explicit LegacyWorldLoaderV1(const WorldLoader &worldLoader) : WorldLoader(worldLoader) {}
};

#endif //LIMONENGINE_LEGACYWORLDLOADERV1_H
