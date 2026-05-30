//
// Created by engin on 16.07.2018.
//

#ifndef LIMONENGINE_SOUND_H
#define LIMONENGINE_SOUND_H


#include <memory>
#include "GameObject.h"
#include "../Attachable.h"
#include "../Editor/ImGuiResult.h"
#include "../Editor/ImGuiRequest.h"

class SoundAsset;
class AssetManager;

class Sound : public GameObject, public Attachable {
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
    float referenceDistance = 10.0f;
    float maxDistance = 100.0f;
    bool looped = false;
    bool autoPlay = false;
    bool temporary = false;

    Transformation transformation;

public:
    Sound(uint32_t worldID, std::shared_ptr<AssetManager> assetManager, const std::string &filename);
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

    void onTransformUpdated() noexcept override;

    // --- Attachable interface ---
    Transformation* getTransformation() override { return &transformation; }
    const Transformation* getTransformation() const override { return &transformation; }

    /** Game object methods */
    GameObject::ObjectTypes getTypeID() const override {
        return ObjectTypes::SOUND;
    }

    std::string getName() const override {
        return name;
    }

    uint32_t getWorldObjectID() const override {
        return worldID;
    }

    float getGain() const {
        return gain;
    }

    float getReferenceDistance() const {
        return referenceDistance;
    }

    void setReferenceDistance(float distance) {
        referenceDistance = distance;
    }

    float getMaxDistance() const {
        return maxDistance;
    }

    void setMaxDistance(float distance) {
        maxDistance = distance;
    }

    bool isLooped() const {
        return looped;
    }

    bool isAutoPlay() const {
        return autoPlay;
    }

    void setAutoPlay(bool autoPlay) {
        this->autoPlay = autoPlay;
    }

    bool isListenerRelative() const {
        return listenerRelative;
    }

    bool isTemporary() const {
        return temporary;
    }

    void setTemporary(bool temporary) {
        this->temporary = temporary;
    }

    State getState();

    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request) override;
};


#endif //LIMONENGINE_SOUND_H
