//
// Created by engin on 11.07.2018.
//

#ifndef OPENAL_ALHELPER_H
#define OPENAL_ALHELPER_H

#include "../libs/OpenAL-Soft/include/AL/alc.h"
#include "../libs/OpenAL-Soft/include/AL/al.h"

#include <memory>

class SoundAsset;

#define NUM_BUFFERS 3
#define BUFFER_ELEMENT_COUNT 4096

class ALHelper {
    SDL_SpinLock playRequestLock;
    SDL_Thread *thread = nullptr;

    glm::vec3 position;

    struct PlayingSound {
        const SoundAsset *asset;
        uint64_t sampleCountToPlay;
        ALuint source;
        ALenum format;
        ALuint buffers[NUM_BUFFERS];
        const int16_t *nextDataToBuffer;
        bool looped;

        bool isFinished();

        ~PlayingSound();
    };

    ALCdevice *dev;
    ALCcontext *ctx;

    bool running;

    std::vector<std::unique_ptr<PlayingSound>> playingSounds;
    std::vector<std::pair<bool, const SoundAsset *>> playRequests;

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

    bool startPlay(bool looped, std::unique_ptr<PlayingSound> &sound);

    bool refreshBuffers(std::unique_ptr<PlayingSound> &sound);//this method updates some of the values of parameter

public:
    ALHelper();

    ~ALHelper();

    void play(const SoundAsset *soundAsset, bool looped);

    void setListenerPositionAndOrientation(const glm::vec3 &position, const glm::vec3 &front, const glm::vec3 &up) {
        glm::vec3 velocity = this->position - position;
        this->position = position;
        ALfloat listenerOri[] = {front.x, front.y, front.z,
                                 up.x, up.y, up.z};
        ALenum error;

// Position ...
        alListenerfv(AL_POSITION, glm::value_ptr(this->position));
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
            std::cerr << "Set listener oerientation failed! " << alGetString(error) << std::endl;
            return;
        }
    }

};


#endif //OPENAL_ALHELPER_H
