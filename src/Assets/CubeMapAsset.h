//
// Created by Engin Manap on 1.03.2016.
//

#ifndef UBERGAME_CUBEMAP_H
#define UBERGAME_CUBEMAP_H

#include <iostream>
#include <string>
#include <SDL2/SDL_image.h>

#include "../GLHelper.h"


class CubeMapAsset {
    GLHelper *glHelper;
    std::string path;
    std::string names[6];
    GLuint cubeMapBufferID;
public:
    CubeMapAsset(GLHelper *glHelper, std::string path,
                 std::string right, std::string left,
                 std::string top, std::string bottom,
                 std::string back, std::string front);

    ~CubeMapAsset() {
        glHelper->deleteTexture(cubeMapBufferID);
    }

    GLuint getID() const {
        return cubeMapBufferID;
    }

    //FIXME destructor needed.
};

#endif //UBERGAME_CUBEMAP_H
