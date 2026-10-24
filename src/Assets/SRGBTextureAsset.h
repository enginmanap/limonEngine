#ifndef LIMONENGINE_SRGBTEXTUREASSET_H
#define LIMONENGINE_SRGBTEXTUREASSET_H

#include "TextureAsset.h"

//color textures, the GPU decodes them to linear on sampling. Always RGBA, SRGB8 isn't renderable so ES can't generate its mipmaps
class SRGBTextureAsset : public TextureAsset {
    bool convertToSupportedFormat() override;

public:
    SRGBTextureAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &files) :
            TextureAsset(assetManager, assetID, files) {}
};

#endif //LIMONENGINE_SRGBTEXTUREASSET_H
