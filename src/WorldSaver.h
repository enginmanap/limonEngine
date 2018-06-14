//
// Created by engin on 24.03.2018.
//

#ifndef LIMONENGINE_WORLDSAVER_H
#define LIMONENGINE_WORLDSAVER_H

class World;

class WorldSaver {
private:
    static bool fillObjects(tinyxml2::XMLDocument& mapFile, tinyxml2::XMLElement * objectsNode, const World* world );
    static bool fillLights(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *lightsNode, const World *world);
    static bool addSky(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *skyNode, const World *world);
    static bool fillLoadedAnimations(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *loadedAnimationsNode, const World *world);
    static bool fillTriggers(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *triggersNode, const World *world);
    static bool fillOnloadActions(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *onloadActionsNode, const World *world);
    static bool fillOnloadAnimations(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *onloadAnimationsNode, const World *world);

public:
    static bool saveWorld(const std::string& mapName, const World* world);
};


#endif //LIMONENGINE_WORLDSAVER_H
