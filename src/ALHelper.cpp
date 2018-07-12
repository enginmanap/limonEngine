//
// Created by engin on 11.07.2018.
//

#include "ALHelper.h"

#define DR_WAV_IMPLEMENTATION
#include "../libs/dr_wav.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

ALHelper::ALHelper() {
    dev = alcOpenDevice(NULL);
    if(!dev) {
        throw("Audio device setup failed!");
    }
    ctx = alcCreateContext(dev, NULL);
    alcMakeContextCurrent(ctx);
    if(!ctx) {
        throw("Audio context setup failed!");
    }
    this->running = true;
    SDL_AtomicUnlock(&playRequestLock);

    thread = SDL_CreateThread(&staticSoundManager, "soundManager", this);

}

bool ALHelper::startPlay(const std::string &wavFileName, std::unique_ptr<PlayingSound>& sound) {
    unsigned int channels;

    alGenBuffers(NUM_BUFFERS, sound->buffers);
    alGenSources(1, &sound->source);
    if(alGetError() != AL_NO_ERROR) {
        std::cerr << "Audio buffer setup failed!" << std::endl;
        return false;
    }



    sound->data = drwav_open_and_read_file_s16(wavFileName.c_str(), &channels, &sound->sampleRate, &sound->sampleCountToPlay);
    if (sound->data == NULL) {
        // Error opening and reading WAV file.
        std::cerr << "failed to read wav file" << std::endl;
        return false;
    }
    sound->nextDataToBuffer = sound->data;

    sound->format = to_al_format(channels, 16);

    for (uint32_t i = 0; i < NUM_BUFFERS; ++i) {
        uint32_t currentPlaySize = std::min((uint64_t)sound->sampleCountToPlay, (uint64_t)BUFFER_ELEMENT_COUNT);
        sound->sampleCountToPlay = sound->sampleCountToPlay - currentPlaySize;

        alBufferData(sound->buffers[i], sound->format, sound->nextDataToBuffer, currentPlaySize * sizeof(int16_t), sound->sampleRate);
        sound->nextDataToBuffer = sound->nextDataToBuffer + currentPlaySize;
        if(alGetError() != AL_NO_ERROR) {
            std::cerr << "Audio buffer data failed!" << std::endl;
            drwav_free(sound->data);
        }
    }

    alSourceQueueBuffers(sound->source, NUM_BUFFERS, sound->buffers);
    alSourcePlay(sound->source);
    if(alGetError() != AL_NO_ERROR)
    {
        fprintf(stderr, "Error starting :(\n");
        drwav_free(sound->data);
        return false;
    }

    return true;
}

int ALHelper::soundManager() {
    while(running) {
        SDL_AtomicLock(&playRequestLock);
        for (size_t i = 0; i < playRequests.size(); ++i) {
            auto sound = std::make_unique<PlayingSound>();
            if(startPlay(playRequests.at(i), sound)) {
                playingSounds.push_back(std::move(sound));
                std::cout << "Playing new sound" << std::endl;
            }
        }
        playRequests.clear();
        SDL_AtomicUnlock(&playRequestLock);
        for (auto iterator = playingSounds.begin(); iterator != playingSounds.end(); ) {
            if((*iterator)->isFinished()) {
                //iterator = playingSounds.erase(iterator);
                //std::cout << "removing sound" << std::endl;
                ++iterator;
            } else {
                refreshBuffers(*iterator);
                ++iterator;
            }
        }
        SDL_Delay(10);
    }
    return 0;
}

void ALHelper::play(const std::string &wavFileName) {
    SDL_AtomicLock(&playRequestLock);
    this->playRequests.push_back(wavFileName);
    SDL_AtomicUnlock(&playRequestLock);

}

bool ALHelper::refreshBuffers(std::unique_ptr<ALHelper::PlayingSound> &sound) {
    if(sound->sampleCountToPlay > 0) {
        ALuint buffer;
        ALint val;

        ALenum error;


//        Check if OpenAL is done with any of the queued buffers
        alGetSourcei(sound->source, AL_BUFFERS_PROCESSED, &val);
        if (alGetError() != AL_NO_ERROR) {
            fprintf(stderr, "Error getting buffer state:(\n");
            return 1;
        }
        if (val <= 0) {
            return true;
        }
//        For each processed buffer...
        while (val--) {
//            Read the next chunk of decoded data from the stream
//            Pop the oldest queued buffer from the source, fill it with the new data, then requeue it
            alSourceUnqueueBuffers(sound->source, 1, &buffer);
            //alBufferData(buffer, format, buf, ret, frequency);
            uint32_t currentPlaySize = std::min((uint64_t) sound->sampleCountToPlay, (uint64_t) BUFFER_ELEMENT_COUNT);
            sound->sampleCountToPlay = sound->sampleCountToPlay - currentPlaySize;

            alBufferData(buffer, sound->format, sound->nextDataToBuffer, currentPlaySize * sizeof(int16_t), sound->sampleRate);
            if ((error = alGetError()) != AL_NO_ERROR) {
                fprintf(stderr, "Error buffering alGenBuffers : %s", alGetString(error));
                return 1;
            }
            sound->nextDataToBuffer = sound->nextDataToBuffer + currentPlaySize;
            alSourceQueueBuffers(sound->source, 1, &buffer);
            if ((error = alGetError()) != AL_NO_ERROR) {
                fprintf(stderr, "Error source buffering : %s", alGetString(error));
                return 1;
            }
        }
    }

    return true;
}

bool ALHelper::PlayingSound::isFinished() {
    ALint source_state;
    alGetSourcei(source, AL_SOURCE_STATE, &source_state);
    return source_state != AL_PLAYING;
}

ALHelper::PlayingSound::~PlayingSound() {
        alDeleteBuffers(1, buffers);
        alDeleteSources(1, &source);
        drwav_free(data);
}

ALHelper::~ALHelper() {
    this->running = false;
    int threadReturnValue;
    SDL_WaitThread(thread, &threadReturnValue);
    dev = alcGetContextsDevice(ctx);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(ctx);
    alcCloseDevice(dev);

}
