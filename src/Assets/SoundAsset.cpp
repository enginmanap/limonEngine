//
// Created by engin on 15.07.2018.
//

#include "SoundAsset.h"
#include "../../libs/dr_wav.h"

#include <iostream>
#include <vector>

SoundAsset::SoundAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList) : Asset(
        assetManager, assetID, fileList) {

    if (fileList.empty()) {
        std::cerr << "Sound load failed because file name vector is empty." << std::endl;
        exit(-1);
    }
    name = fileList[0];
    if (fileList.size() > 1) {
        std::cerr << "multiple files are sent to Sound Asset constructor, extra elements ignored." << std::endl;
    }
}

void SoundAsset::loadInternal() {
    soundData = drwav_open_and_read_file_s16(name.c_str(), &channels, &sampleRate, &sampleCount);
    if (soundData == nullptr) {
        // Error opening and reading WAV file.
        std::cerr << "failed to read wav file " << name << " , this case is not handled. Exiting" << std::endl;
        exit(-1);
    }
}

SoundAsset::~SoundAsset() {
    drwav_free(soundData);
    std::cout << "Sound asset " << this->name << " unloaded" << std::endl;
}