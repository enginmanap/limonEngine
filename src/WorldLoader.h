//
// Created by engin on 10.03.2018.
//

#ifndef LIMONENGINE_WORLDLOADER_H
#define LIMONENGINE_WORLDLOADER_H

#include <string>
#include <vector>
#include <tinyxml2.h>
#include "ALHelper.h"

class World;
class Options;
class GLHelper;
class AssetManager;
class GLHelper;
class ALHelper;


class WorldLoader {
    Options *options;
    GLHelper *glHelper;
    ALHelper *alHelper;
    AssetManager *assetManager;
    std::vector<World*> loadedWorlds;

    World* loadMapFromXML(const std::string& worldFileName) const;
    bool loadObjectsFromXML(tinyxml2::XMLNode *objectsNode, World* world)const;
    bool loadSkymap(tinyxml2::XMLNode *skymapNode, World* world) const;
    bool loadLights(tinyxml2::XMLNode *lightsNode, World* world) const;
    bool loadAnimations(tinyxml2::XMLNode *worldNode, World *world) const;
    bool loadTriggers(tinyxml2::XMLNode *worldNode, World *world) const;
    bool loadOnLoadActions(tinyxml2::XMLNode *worldNode, World *world) const;
    bool loadOnLoadAnimations(tinyxml2::XMLNode *worldNode, World *world) const;
    bool loadGUILayersAndElements(tinyxml2::XMLNode *worldNode, World *world) const;

    void attachedAPIMethodsToWorld(World *world) const;

public:
    WorldLoader(AssetManager *assetManager, GLHelper *glHelper, ALHelper *alHelper, Options *options);
    World* loadWorld(const std::string& worldFile) const;

    ~WorldLoader();


};


#endif //LIMONENGINE_WORLDLOADER_H
