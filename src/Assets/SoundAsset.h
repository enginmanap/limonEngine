//
// Created by engin on 15.07.2018.
//

#ifndef LIMONENGINE_SOUNDASSET_H
#define LIMONENGINE_SOUNDASSET_H


#include "Asset.h"

class SoundAsset : public Asset {
    unsigned int channels;
    uint32_t sampleRate;
    uint64_t sampleCount;
    int16_t* soundData = nullptr; //PCM 16bit, prefer single channel
    std::string name;

public:

    SoundAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList);
#ifdef CEREAL_SUPPORT
    SoundAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList, cereal::BinaryInputArchive& binaryArchive) :
    Asset(assetManager, assetID, fileList, binaryArchive) {
        static_assert(true, "SoundAsset doesn't support Cereal Loading");
    }
#endif
    ~SoundAsset();

    unsigned int getChannels() const {
        return channels;
    }

    uint32_t getSampleRate() const {
        return sampleRate;
    }

    uint64_t getSampleCount() const {
        return sampleCount;
    }

    const int16_t *getSoundData() const {
        return soundData;
    }

    const std::string& getName() const {
        return name;
    }
};


#endif //LIMONENGINE_SOUNDASSET_H
