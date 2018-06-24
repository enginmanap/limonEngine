//
// Created by Engin Manap on 1.03.2016.
//

#ifndef LIMONENGINE_CUBEMAP_H
#define LIMONENGINE_CUBEMAP_H

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
    CubeMapAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList);

    ~CubeMapAsset() {
        assetManager->getGlHelper()->deleteTexture(cubeMapBufferID);
    }

    GLuint getID() const {
        return cubeMapBufferID;
    }

    std::vector<std::string> getNames() {
        //FIXME this shouldn't be required. Assets should have IDs too
        std::vector<std::string> result;
        result.push_back(path);
        for (int i = 0; i < 6; ++i) {
            result.push_back(names[i]);
        }
        return result;
    };
};

#endif //LIMONENGINE_CUBEMAP_H
