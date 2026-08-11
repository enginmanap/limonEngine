//
// Created by engin on 16.07.2018.
//

#ifndef LIMONENGINE_SOUND_H
#define LIMONENGINE_SOUND_H


#include <memory>
#include <tinyxml2.h>
#include "GameObject.h"
#include "../Attachable.h"
#include "../Editor/ImGuiResult.h"
#include "../Editor/ImGuiRequest.h"
#include "../limonAPI/LimonTypes.h"

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
    float gain = 1.0f;//default, normalized 0..1
    float referenceDistance = 2.0f;
    float maxDistance = 50.0f;
    bool looped = false;
    bool autoPlay = false;
    bool temporary = false;
    LimonTypes::AudioChannel channel = LimonTypes::AudioChannel::SFX;

    Transformation transformation;

public:
    Sound(uint32_t worldID, std::shared_ptr<AssetManager> assetManager, const std::string &filename);

    // Copies settings only; soundHandleID/playState stay at their fresh-construction defaults so the
    // clone gets its own OpenAL source instead of fighting the original over one playback handle.
    Sound(const Sound& other, uint32_t newObjectID);

    Attachable* clone(uint32_t newObjectID, LimonAPI* limonAPI,
                      const std::unordered_map<uint32_t, uint32_t>& idRemap) const override;

    ~Sound();

    void setLoop(bool looped);

    void setStartPosition(float startSecond);

    void setStopPosition(float stopPosition);

    void play(float fadeInSeconds = 0.0f);

    /** Fade the gain to zero over the given duration, then stop. Used for music crossfade-out. */
    void fadeOutAndStop(float seconds);

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

    LimonTypes::AudioChannel getChannel() const {
        return channel;
    }

    void setChannel(LimonTypes::AudioChannel channel) {
        this->channel = channel;
    }

    bool isTemporary() const {
        return temporary;
    }

    void setTemporary(bool temporary) {
        this->temporary = temporary;
    }

    State getState();

    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request) override;

    void serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *soundsNode) const; // skips temporary sounds

    // nullptr on a malformed entry means skip it, not abort the list. unlike Light, IDs are used
    // verbatim here - no reassignment, a missing/colliding one is corruption to report, not a legacy
    // format to migrate.
    static Sound *deserialize(tinyxml2::XMLElement *soundNode, std::shared_ptr<AssetManager> assetManager,
                               bool &hasParent, uint32_t &parentID, int32_t &parentBoneID);
};


#endif //LIMONENGINE_SOUND_H
