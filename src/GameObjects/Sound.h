//
// Created by engin on 16.07.2018.
//

#ifndef LIMONENGINE_SOUND_H
#define LIMONENGINE_SOUND_H


#include <memory>
#include "GameObject.h"

class SoundAsset;
class AssetManager;

class Sound : public GameObject {
public:
    enum class State { STOPPED, PLAYING, STOP_AFTER_FINISH, PAUSED };
private:
    std::string name;
    uint32_t worldID;
    uint32_t soundHandleID = 0;
    std::shared_ptr<AssetManager> assetManager;

    glm::vec3 position = glm::vec3(0,0,0);
    bool listenerRelative = true; //by default plays at the listener position

    State playState = State::STOPPED;
    float startSecond = 0;
    float stopPosition = 0;
    float gain = 1000;//default
    bool looped = false;

public:
    Sound(uint32_t worldID,  std::shared_ptr<AssetManager> assetManager, const std::string &filename);
    ~Sound();

    void setLoop(bool looped);

    void setStartPosition(float startSecond);

    void setStopPosition(float stopPosition);

    void play();

    void stop();

    void pause();
    void resume();

    void stopAfterFinish();

    bool changeGain(float gain);

    void setWorldPosition(glm::vec3 position, bool listenerRelative = false);

    /** Game object methods */
    GameObject::ObjectTypes getTypeID() const {
        return SOUND;
    }

    std::string getName() const {
        return name;
    }

    uint32_t getWorldObjectID() const override {
        return worldID;
    }

    float getGain() const {
        return gain;
    }

    State getState();

    /** Game object methods */
};


#endif //LIMONENGINE_SOUND_H
