//
// Created by engin on 11.07.2018.
//

#ifndef OPENAL_ALHELPER_H
#define OPENAL_ALHELPER_H

#include "../libs/OpenAL-Soft/include/AL/alc.h"
#include "../libs/OpenAL-Soft/include/AL/al.h"

#include <memory>
#include <unordered_map>
#include <vector>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "SDL2Helper.h"
#include "SDL2MultiThreading.h"
#include "limonAPI/LimonTypes.h"

class SoundAsset;

#define NUM_BUFFERS 3
#define BUFFER_ELEMENT_COUNT 8192

class ALHelper {
    friend class World;

public:
    enum class DistanceModel {
        LINEAR_CLAMPED,
        INVERSE_CLAMPED,
        EXPONENT_CLAMPED
    };

private:
    struct PlayingSound {
        uint32_t soundID;
        std::shared_ptr<SoundAsset> asset;
        uint64_t sampleCountToPlay = 0;
        ALuint source = 0;
        ALenum format = AL_FORMAT_STEREO16;
        ALuint buffers[NUM_BUFFERS];
        float gain = 1.0f; //normalized 0..1 (per-source); multiplied by channel and master gain on apply
        LimonTypes::AudioChannel channel = LimonTypes::AudioChannel::SFX;
        const int16_t *nextDataToBuffer = nullptr;
        bool looped = false;
        bool paused = false;
        bool stopped = false;
        glm::vec3 position = glm::vec3(0,0,0);
        bool isPositionRelative = true;
        float referenceDistance = 2.0f;
        float maxDistance = 50.0f;
        // Time-based gain fade, advanced by the audio thread. fadeDurationMs == 0 means no active fade.
        float fadeFrom = 1.0f;
        float fadeTarget = 1.0f;
        float fadeElapsedMs = 0.0f;
        float fadeDurationMs = 0.0f;
        bool stopAtFadeEnd = false; //when a fade-out completes, stop (and unloop) the source so it gets cleaned up
        bool isFinished();
        PlayingSound(uint32_t id): soundID(id) {};

        ~PlayingSound();
    };

    SDL_SpinLock playRequestLock;
    SDL_Thread *thread = nullptr;
    SDL2MultiThreading::SpinLock removeSoundLock;

    ALCdevice *dev;
    ALCcontext *ctx;

    glm::vec3 ListenerPosition = glm::vec3(0.0f,0.0f,0.0f);
    DistanceModel distanceModel = DistanceModel::LINEAR_CLAMPED;
    float channelGain[(size_t)LimonTypes::AudioChannel::COUNT] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    bool running = true;
    bool paused = false;
    bool resumed = false;
    uint32_t soundRequestID = 1;
    std::unordered_map<uint32_t, std::unique_ptr<PlayingSound>> playingSounds;
    std::vector<std::unique_ptr<PlayingSound>> playRequests;

    static inline ALenum to_al_format(short channels, short samples) {
        bool stereo = (channels > 1);

        switch (samples) {
            case 16:
                if (stereo)
                    return AL_FORMAT_STEREO16;
                else
                    return AL_FORMAT_MONO16;
            case 8:
                if (stereo)
                    return AL_FORMAT_STEREO8;
                else
                    return AL_FORMAT_MONO8;
            default:
                return -1;
        }
    }

    static int staticSoundManager(void *objectPointer) {
        return static_cast<ALHelper *>(objectPointer)->soundManager();
    }

    int soundManager();

    /** Single choke point for writing AL_GAIN: combines per-source, channel and master gain. */
    void applyEffectiveGain(PlayingSound &sound);

    bool startPlay(std::unique_ptr<PlayingSound> &sound);

    bool refreshBuffers(std::unique_ptr<PlayingSound> &sound);//this method updates some of the values of parameter

    uint32_t getNextRequestID(){
        return soundRequestID++;
    }

    void pausePlay() {
        this->paused = true;
    }

    void resumePlay() {
        this->resumed = true;
    }

    std::string getErrorString(ALenum error ) {
        switch (error) {
            case AL_NO_ERROR:
                return "No error";
            case AL_INVALID_NAME:
                return "invalid name";
            case AL_INVALID_ENUM:
                return "invalid enum";
            case AL_INVALID_VALUE:
                return "invalid value";
            case AL_INVALID_OPERATION:
                return "invalid operaion";
            case AL_OUT_OF_MEMORY:
                return "out of memory";
            default:
                return "unknown error";
        }
    }

public:
    ALHelper();

    ~ALHelper();

    uint32_t play(const std::shared_ptr<SoundAsset> soundAsset, bool looped, float gain = 1.0f,
                  float referenceDistance = 2.0f, float maxDistance = 50.0f,
                  LimonTypes::AudioChannel channel = LimonTypes::AudioChannel::SFX, float fadeInSeconds = 0.0f);

    bool isPlaying(uint32_t soundID) {
        if(playingSounds.find(soundID) != playingSounds.end()) {
            return playingSounds[soundID]->looped || !playingSounds[soundID]->isFinished();
        }
        //it is possible that play is requested, but not yet started, they should be considered playing too, check it
        bool result = false;
        SDL_LockSpinlock(&playRequestLock);
        for (auto request = playRequests.begin(); request != playRequests.end(); ++request) {
            if((*request)->soundID == soundID) {
                result = true;
                break;
            }
        }
        SDL_UnlockSpinlock(&playRequestLock);
        return result;
    }

    bool changeGain(uint32_t soundID, float gain) {
        if(playingSounds.find(soundID) != playingSounds.end()) {
            std::unique_ptr<PlayingSound>& sound = playingSounds[soundID];
            sound->gain = gain;
            sound->fadeDurationMs = 0; //an explicit volume change cancels any running fade
            applyEffectiveGain(*sound);
            return true;
        }
        //it is possible that play is requested, but not yet started, they should be considered playing too, check it
        bool result = false;
        SDL_LockSpinlock(&playRequestLock);
        for (auto request = playRequests.begin(); request != playRequests.end(); ++request) {
            if((*request)->soundID == soundID) {
                (*request)->gain = gain;
                (*request)->fadeDurationMs = 0;
                result = true;
                break;
            }
        }
        SDL_UnlockSpinlock(&playRequestLock);
        return result;
    }

    /**
     * Ramp a sound's gain to targetGain over the given duration. seconds <= 0 applies instantly.
     * The ramp is advanced on the audio thread. If stopAtEnd is true and the target is 0, the source
     * is stopped (and unlooped) when the fade completes so it gets cleaned up (used for crossfade-out).
     */
    void fadeGain(uint32_t soundID, float targetGain, float seconds, bool stopAtEnd = false);

    /** Set the master gain for a channel (bus). Re-applies to all sounds currently on that channel. */
    void setChannelGain(LimonTypes::AudioChannel channel, float gain);
    float getChannelGain(LimonTypes::AudioChannel channel) const {
        return channelGain[(size_t)channel];
    }

    bool stop(uint32_t soundID);
    bool pause(uint32_t soundID);
    bool resume(uint32_t soundID);

    DistanceModel getDistanceModel() const {
        return distanceModel;
    }

    void setDistanceModel(DistanceModel model) {
        distanceModel = model;
        switch (model) {
            case DistanceModel::INVERSE_CLAMPED:  alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);  break;
            case DistanceModel::EXPONENT_CLAMPED: alDistanceModel(AL_EXPONENT_DISTANCE_CLAMPED); break;
            case DistanceModel::LINEAR_CLAMPED:
            default:                              alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);   break;
        }
    }

    inline void setListenerPositionAndOrientation(const glm::vec3 &position, const glm::vec3 &front, const glm::vec3 &up) {
        glm::vec3 velocity = this->ListenerPosition - position;
        this->ListenerPosition = position;
        ALfloat listenerOri[] = {front.x, front.y, front.z,
                                 up.x, up.y, up.z};
        ALenum error;

// Position ...
        alListenerfv(AL_POSITION, glm::value_ptr(this->ListenerPosition));
        if ((error = alGetError()) != AL_NO_ERROR) {
            std::cerr << "Set listener position failed! " << alGetString(error) << std::endl;
            return;
        }
// Velocity ...
        alListenerfv(AL_VELOCITY, glm::value_ptr(velocity));
        if ((error = alGetError()) != AL_NO_ERROR) {
            std::cerr << "Set listener velocity failed! " << alGetString(error) << std::endl;
            return;
        }
// Orientation ...
        alListenerfv(AL_ORIENTATION, listenerOri);
        if ((error = alGetError()) != AL_NO_ERROR) {
            std::cerr << "Set listener orientation failed! " << alGetString(error) << std::endl;
            return;
        }
    }

    void setSourcePosition(uint32_t soundID, bool isCameraRelative, const glm::vec3 &soundPosition) {
        if(playingSounds.find(soundID) != playingSounds.end()) {
            std::unique_ptr<PlayingSound>& sound =  playingSounds[soundID];

            if(isCameraRelative != sound->isPositionRelative) {
                if (isCameraRelative) {
                    alSourcei(sound->source, AL_SOURCE_RELATIVE, AL_TRUE);
                } else {
                    alSourcei(sound->source, AL_SOURCE_RELATIVE, AL_FALSE);
                }
                sound->isPositionRelative = isCameraRelative;
            }

            if(sound->position != soundPosition) {
                alSource3f(sound->source, AL_POSITION, soundPosition.x, soundPosition.y, soundPosition.z);

                alSource3f(sound->source, AL_VELOCITY, soundPosition.x - sound->position.x,
                           soundPosition.y - sound->position.y,
                           soundPosition.z - sound->position.z);
                sound->position = soundPosition;
            }

            ALenum error;
            if ((error = alGetError()) != AL_NO_ERROR) {
                std::cerr << "Error setting source position! " << alGetString(error) << std::endl;
                return;
            }
        } else {
            //sound is not found in playing sounds, try to find it in requests
            //it is possible that play is requested, but not yet started, they should be considered playing too, check it
            SDL_LockSpinlock(&playRequestLock);
            for (auto request = playRequests.begin(); request != playRequests.end(); ++request) {
                if((*request)->soundID == soundID) {
                    (*request)->isPositionRelative = isCameraRelative;
                    (*request)->position = soundPosition;
                    break;
                }
            }
            SDL_UnlockSpinlock(&playRequestLock);

            return;
        }
    }

    bool setLooped(uint32_t soundID, bool looped);
};


#endif //OPENAL_ALHELPER_H
