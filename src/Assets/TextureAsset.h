//
// Created by Engin Manap on 1.03.2016.
//

#ifndef LIMONENGINE_TEXTURE_H
#define LIMONENGINE_TEXTURE_H

#include <string>
#include <iostream>
#include <SDL2/SDL_image.h>
#include "GLHelper.h"
#include "Asset.h"
#include "AssetManager.h"

class TextureAsset : public Asset {
protected:
    std::vector<std::string> name;//1) single element filename, 2) First element embedded texture ID, second element model.
    std::shared_ptr<GLHelper::Texture> texture;

public:
    TextureAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &files);

    ~TextureAsset();

    uint32_t getID() const {
        return texture->getTextureID();
    }

    const std::shared_ptr<GLHelper::Texture> &getTexture() const {
        return texture;
    }

    std::shared_ptr<GLHelper::Texture> &getTexture() {
        return texture;
    }

    std::vector<std::string> getName() const {
        return name;
    }

    uint32_t getHeight() const {
        return texture->getHeight();
    }

    uint32_t getWidth() const {
        return texture->getWidth();
    }
};

#endif //LIMONENGINE_TEXTURE_H
