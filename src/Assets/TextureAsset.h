//
// Created by Engin Manap on 1.03.2016.
//

#ifndef LIMONENGINE_TEXTURE_H
#define LIMONENGINE_TEXTURE_H

#include <string>
#include <iostream>
#include <SDL2/SDL_image.h>
#include "Asset.h"
#include "AssetManager.h"

class TextureAsset : public Asset {
protected:
    std::string name;
    uint32_t textureBufferID;
    uint32_t height = 0;
    uint32_t width;

public:
    TextureAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &files);

    ~TextureAsset();

    uint32_t getID() const {
        return textureBufferID;
    }

    std::string getName() const {
        return name;
    }

    uint32_t getHeight() const {
        return height;
    }

    uint32_t getWidth() const {
        return width;
    }
};

#endif //LIMONENGINE_TEXTURE_H
