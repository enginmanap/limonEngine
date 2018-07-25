//
// Created by engin on 16.07.2018.
//

#include "Sound.h"
#include "../Assets/AssetManager.h"
#include "../ALHelper.h"
#include "../Assets/SoundAsset.h"

Sound::Sound(uint32_t worldID, AssetManager *assetManager, const std::string &filename) : name(filename), worldID(worldID), assetManager(assetManager) {}

void Sound::setLoop(bool looped) {
    this->looped = looped;
    if(this->soundHandleID != 0) {
        assetManager->getAlHelper()->setLooped(soundHandleID, this->looped);
    }
}

void Sound::setStartPosition(float startSecond) {
    this->startSecond = startSecond;
    std::cerr << "This method [setStartPosition] is not implemented yet " << std::endl;
}

void Sound::setStopPosition(float stopPosition) {
    this->stopPosition = stopPosition;
    std::cerr << "This method [setStopPosition] is not implemented yet " << std::endl;
}

void Sound::play() {
    if(soundHandleID != 0) {
        if(!assetManager->getAlHelper()->isPlaying(soundHandleID)) {//don't play if already playing
            soundHandleID = assetManager->getAlHelper()->play(assetManager->loadAsset<SoundAsset>({this->name}),
                                                              this->looped);
        }
    } else {
        soundHandleID = assetManager->getAlHelper()->play(assetManager->loadAsset<SoundAsset>({this->name}),
                                                          this->looped);
    }
}

void Sound::stop() {
    assetManager->getAlHelper()->stop(soundHandleID);
}

void Sound::stopAfterFinish() {
    assetManager->getAlHelper()->setLooped(soundHandleID, false);
}

void Sound::setWorldPosition(glm::vec3 position, bool listenerRelative) {
    this->position = position;
    this->listenerRelative = listenerRelative;

    assetManager->getAlHelper()->setSourcePosition(soundHandleID, this->listenerRelative, this->position);
}