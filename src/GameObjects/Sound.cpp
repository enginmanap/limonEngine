//
// Created by engin on 16.07.2018.
//

#include "Sound.h"
#include "../Assets/AssetManager.h"
#include "../ALHelper.h"
#include "../Assets/SoundAsset.h"

Sound::Sound(uint32_t worldID, AssetManager *assetManager, const std::string &filename) : name(filename), worldID(worldID), assetManager(assetManager) {
    this->soundAsset = assetManager->loadAsset<SoundAsset>({filename});
}

void Sound::setLoop(bool looped) {
    this->looped = looped;
}

void Sound::setStartPosition(float startSecond) {
    this->startSecond = startSecond;
}

void Sound::setStopPosition(float stopPosition) {
    this->stopPosition = stopPosition;
}

void Sound::play() {
    soundHandleID = assetManager->getAlHelper()->play(this->soundAsset, this->looped);
}

void Sound::stop() {
    assetManager->getAlHelper()->stop(soundHandleID);
}

void Sound::setWorldPosition(glm::vec3 position, bool listenerRelative) {
    this->position = position;
    this->listenerRelative = listenerRelative;

    assetManager->getAlHelper()->setSourcePosition(soundHandleID, this->listenerRelative, this->position);
}
