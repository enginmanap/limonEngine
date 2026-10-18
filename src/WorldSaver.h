//
// Created by engin on 24.03.2018.
//

#ifndef LIMONENGINE_WORLDSAVER_H
#define LIMONENGINE_WORLDSAVER_H

#include <map>
#include <memory>
#include <string>
#include <glm/vec3.hpp>
#include <tinyxml2.h>

class World;
class Model;
class Material;

class WorldSaver {
private:
    //the materials objects name by key, and whether the key is also a world override. Written once under <Materials>
    typedef std::map<std::string, std::pair<std::shared_ptr<const Material>, bool>> MaterialPool;

    static bool fillObjects(tinyxml2::XMLDocument& mapFile, tinyxml2::XMLElement * objectsNode, const World* world, MaterialPool &pool);
    static bool verifyMaterialKeys(const tinyxml2::XMLElement *objectsNode, const MaterialPool &pool);
    static bool fillObjectGroups(tinyxml2::XMLDocument& mapFile, tinyxml2::XMLElement * objectGroupsNode, const World* world );
    static bool fillLights(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *lightsNode, const World *world);
    static bool fillSounds(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *soundsNode, const World *world);
    static bool fillEmitters(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *EmittersNode, const World *world);
    static bool fillLoadedAnimations(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *loadedAnimationsNode, const World *world);
    static bool fillTriggers(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *triggersNode, const World *world);
    static bool fillOnloadActions(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *onloadActionsNode, const World *world);
    static bool fillOnloadAnimations(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *onloadAnimationsNode, const World *world);
    static bool fillGUILayersAndElements(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *GUILayersListNode, const World *world);
    static bool fillMaterials(tinyxml2::XMLDocument & document, tinyxml2::XMLElement * materialsNode, const World * world, MaterialPool &pool);
public:
    static bool saveWorld(const std::string& mapName, const World* world);
};


#endif //LIMONENGINE_WORLDSAVER_H
