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
#include "AssetManager.h"


class CubeMapAsset : public Asset {
    std::string path;
    std::string names[6];
    GLuint cubeMapBufferID;
public:
    CubeMapAsset(AssetManager* assetManager, const std::vector<std::string> &fileList);

    ~CubeMapAsset() {
        assetManager->getGlHelper()->deleteTexture(cubeMapBufferID);
    }

    GLuint getID() const {
        return cubeMapBufferID;
    }

    //FIXME destructor needed.
};

#endif //UBERGAME_CUBEMAP_H
