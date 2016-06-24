//
// Created by Engin Manap on 1.03.2016.
//

#ifndef UBERGAME_SKYBOX_H
#define UBERGAME_SKYBOX_H

#include <string>

#include "Renderable.h"
#include "CubeMap.h"

//FIXME model constructor has a model in it. They should have a common parent.
class SkyBox : public Renderable {
    std::vector<glm::vec3> vertices;
    std::vector<glm::mediump_uvec3> faces;

    CubeMap *cubeMap;

public:
    SkyBox(GLHelper *glHelper, std::string path, std::string right, std::string left,
           std::string top, std::string down,
           std::string back, std::string front);

    void render(Light *light);

    ~SkyBox() {
        delete cubeMap;
    }
};

#endif //UBERGAME_SKYBOX_H
