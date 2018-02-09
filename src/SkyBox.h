//
// Created by Engin Manap on 1.03.2016.
//

#ifndef LIMONENGINE_SKYBOX_H
#define LIMONENGINE_SKYBOX_H

#include <string>

#include "Renderable.h"
#include "Assets/CubeMapAsset.h"
#include "Assets/AssetManager.h"

class SkyBox : public Renderable {
    std::vector<glm::vec3> vertices;
    std::vector<glm::mediump_uvec3> faces;

    CubeMapAsset *cubeMap;

public:
    SkyBox(AssetManager *assetManager, std::string path, std::string right, std::string left,
           std::string top, std::string down,
           std::string back, std::string front);

    void render();

    void setupForTime(long time) {};

    ~SkyBox() {
        delete cubeMap;
    }
};

#endif //LIMONENGINE_SKYBOX_H
