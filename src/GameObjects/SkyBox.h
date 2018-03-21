//
// Created by Engin Manap on 1.03.2016.
//

#ifndef LIMONENGINE_SKYBOX_H
#define LIMONENGINE_SKYBOX_H

#include <string>

#include "../Renderable.h"
#include "../Assets/CubeMapAsset.h"
#include "../Assets/AssetManager.h"

class SkyBox : public Renderable, public GameObject {
    uint32_t objectID;
    AssetManager *assetManager;
    std::string path;
    std::vector<glm::vec3> vertices;
    std::vector<glm::mediump_uvec3> faces;

    CubeMapAsset *cubeMap;

public:
    SkyBox(uint32_t objectID, AssetManager *assetManager, std::string path, std::string right, std::string left,
           std::string top, std::string down, std::string back, std::string front);

    void render();

    void setupForTime(long time __attribute__((unused))) {};

    ~SkyBox() {
        assetManager->freeAsset(cubeMap->getNames());
    }

    /************Game Object methods **************/
    uint32_t getWorldObjectID() {
        return objectID;
    };

    ObjectTypes getTypeID() const {
        return GameObject::SKYBOX;
    }

    std::string getName() const {
        return path;
    };
    /************Game Object methods **************/

};

#endif //LIMONENGINE_SKYBOX_H
