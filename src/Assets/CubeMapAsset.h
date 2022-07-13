//
// Created by Engin Manap on 1.03.2016.
//

#ifndef LIMONENGINE_CUBEMAP_H
#define LIMONENGINE_CUBEMAP_H

#include <iostream>
#include <string>
#include <cassert>
#include <SDL2/SDL_image.h>

#include "API/Graphics/GraphicsInterface.h"
#include "Graphics/Texture.h"
#include "Asset.h"
#include "AssetManager.h"


class CubeMapAsset : public Asset {
    std::string path;
    std::string names[6];
    std::unique_ptr<Texture> texture;

protected:
    void loadInternal() override;

public:
    CubeMapAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList);
#ifdef CEREAL_SUPPORT
    CubeMapAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList, cereal::BinaryInputArchive& binaryArchive) :
    Asset(assetManager, assetID, fileList, binaryArchive) {
        assert(false && "Cubemap doesn't support Cereal Loading");
    }
#endif

    ~CubeMapAsset() {}

    uint32_t getID() const {
        return texture->getTextureID();
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
