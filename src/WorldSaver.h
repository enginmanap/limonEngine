//
// Created by engin on 24.03.2018.
//

#ifndef LIMONENGINE_WORLDSAVER_H
#define LIMONENGINE_WORLDSAVER_H

class World;

class WorldSaver {
public:
    static bool saveWorld(const std::string& mapName, const World* world);
    static bool fillObjects(tinyxml2::XMLDocument& mapFile, tinyxml2::XMLElement * objectsNode, const World* world );

    static bool fillLights(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *lightsNode, const World *world);
};


#endif //LIMONENGINE_WORLDSAVER_H
