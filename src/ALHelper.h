//
// Created by engin on 11.07.2018.
//

#ifndef OPENAL_ALHELPER_H
#define OPENAL_ALHELPER_H

#include "../libs/OpenAL-Soft/include/AL/alc.h"
#include "../libs/OpenAL-Soft/include/AL/al.h"

#include <memory>

#define NUM_BUFFERS 3
#define BUFFER_ELEMENT_COUNT 4096

class ALHelper {
    SDL_SpinLock playRequestLock;
    SDL_Thread *thread;

    struct PlayingSound {
        uint64_t sampleCountToPlay;
        ALuint source;
        unsigned int sampleRate;
        ALenum format;
        ALuint buffers[NUM_BUFFERS];
        int16_t *data;
        int16_t *nextDataToBuffer;

        bool isFinished();

        ~PlayingSound();
    };

    ALCdevice* dev;
    ALCcontext *ctx;

    bool running;

    std::vector<std::unique_ptr<PlayingSound>> playingSounds;
    std::vector<std::string> playRequests;

    static inline ALenum to_al_format(short channels, short samples)
    {
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

    static int staticSoundManager(void* objectPointer) {
        return static_cast<ALHelper*>(objectPointer)->soundManager();
    }

    int soundManager();

    bool startPlay(const std::string &wavFileName, std::unique_ptr<PlayingSound>& sound);

    bool refreshBuffers(std::unique_ptr<PlayingSound>& sound);//this method updates some of the values of parameter

public:
    ALHelper();
    ~ALHelper();

    void play(const std::string &wavFileName);
};


#endif //OPENAL_ALHELPER_H
