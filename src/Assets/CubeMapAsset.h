//
// Created by Engin Manap on 1.03.2016.
//

#ifndef UBERGAME_CUBEMAP_H
#define UBERGAME_CUBEMAP_H

#include <iostream>
#include <string>
#include <SDL2/SDL_image.h>

#include "../GLHelper.h"
#include "Asset.h"


class CubeMapAsset : public Asset {
    GLHelper *glHelper;
    std::string path;
    std::string names[6];
    GLuint cubeMapBufferID;
public:
    CubeMapAsset(GLHelper *glHelper, const std::vector<std::string> &fileList);

    ~CubeMapAsset() {
        glHelper->deleteTexture(cubeMapBufferID);
    }

    GLuint getID() const {
        return cubeMapBufferID;
    }

    //FIXME destructor needed.
};

#endif //UBERGAME_CUBEMAP_H
