//
// Created by engin on 10.03.2018.
//

#ifndef LIMONENGINE_WORLDLOADER_H
#define LIMONENGINE_WORLDLOADER_H

#include <string>
#include <vector>
#include <tinyxml2.h>

class World;
class Options;
class GLHelper;
class AssetManager;
class GLHelper;


class WorldLoader {
    Options *options;
    GLHelper *glHelper;
    AssetManager *assetManager;
    std::vector<World*> loadedWorlds;

    bool loadMapFromXML(const std::string& worldFileName, World* world) const;
    bool loadObjectsFromXML(tinyxml2::XMLNode *objectsNode, World* world)const;
    bool loadSkymap(tinyxml2::XMLNode *skymapNode, World* world) const;
    bool loadLights(tinyxml2::XMLNode *lightsNode, World* world) const;
    bool loadAnimations(tinyxml2::XMLNode *worldNode, World *world) const;
    bool loadTriggers(tinyxml2::XMLNode *worldNode, World *world) const;
    bool loadOnLoadActions(tinyxml2::XMLNode *worldNode, World *world) const;
public:
    WorldLoader(AssetManager* assetManager, GLHelper* glHelper, Options* options);
    World* loadWorld(const std::string& worldFile) const;

    ~WorldLoader();


};


#endif //LIMONENGINE_WORLDLOADER_H
