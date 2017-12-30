//
// Created by Engin Manap on 1.03.2016.
//

#ifndef LIMONENGINE_TEXTURE_H
#define LIMONENGINE_TEXTURE_H

#include <string>
#include <iostream>
#include <SDL2/SDL_image.h>
#include "Asset.h"
#include "../GLHelper.h"
#include "AssetManager.h"

class TextureAsset : public Asset {
protected:
    std::string name;
    GLuint textureBufferID;

public:
    TextureAsset(AssetManager* assetManager, const std::vector<std::string> &files);

    ~TextureAsset();

    GLuint getID() const {
        return textureBufferID;
    }
};

#endif //LIMONENGINE_TEXTURE_H
