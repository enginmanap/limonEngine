//
// Created by Engin Manap on 1.03.2016.
//

#ifndef UBERGAME_SKYBOX_H
#define UBERGAME_SKYBOX_H

#include <string>

#include "Model.h"
#include "CubeMap.h"


class SkyBox : public Model {

    CubeMap* cubeMap;
    GLuint renderProgram;
public:
    SkyBox(GLHelper* glHelper, std::string right, std::string left,
           std::string top, std::string down,
           std::string back, std::string front);


    void render();
};

#endif //UBERGAME_SKYBOX_H
