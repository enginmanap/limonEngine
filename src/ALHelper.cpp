//
// Created by engin on 11.07.2018.
//

#include "ALHelper.h"
#include "Assets/SoundAsset.h"

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

int ALHelper::soundManager() {
    while(running) {
        SDL_AtomicLock(&playRequestLock);
        for (size_t i = 0; i < playRequests.size(); ++i) {
            auto sound = std::make_unique<PlayingSound>();
            sound->asset = playRequests.at(i).second;
            if(startPlay(playRequests.at(i).first, sound)) {

                playingSounds.push_back(std::move(sound));
                std::cout << "Playing new sound" << std::endl;
            }
        }
        playRequests.clear();
        SDL_AtomicUnlock(&playRequestLock);
        for (auto iterator = playingSounds.begin(); iterator != playingSounds.end(); ) {
            if((*iterator)->isFinished()) {
                if((*iterator)->looped) {
                    std::unique_ptr<PlayingSound>& temp = (*iterator);

                    temp->sampleCountToPlay = temp->asset->getSampleCount();
                    temp->nextDataToBuffer = temp->asset->getSoundData();
                    alSourceQueueBuffers(temp->source, NUM_BUFFERS, temp->buffers);
                    alSourcePlay(temp->source);

                    ++iterator;
                } else {
                    iterator = playingSounds.erase(iterator);
                }
            } else {
                refreshBuffers(*iterator);
                ++iterator;
            }
        }
        SDL_Delay(10);
    }
    return 0;
}

void ALHelper::play(const SoundAsset* soundAsset, bool looped) {
    SDL_AtomicLock(&playRequestLock);
    this->playRequests.push_back(std::make_pair(looped,soundAsset));
    SDL_AtomicUnlock(&playRequestLock);
}


bool ALHelper::startPlay(bool looped, std::unique_ptr<PlayingSound> &sound) {

    alGenBuffers(NUM_BUFFERS, sound->buffers);
    alGenSources(1, &sound->source);
    if(alGetError() != AL_NO_ERROR) {
        std::cerr << "Audio buffer setup failed!" << std::endl;
        return false;
    }

    sound->looped = looped;
    sound->format = to_al_format(sound->asset->getChannels(), 16);


    sound->nextDataToBuffer = sound->asset->getSoundData();
    sound->sampleCountToPlay = sound->asset->getSampleCount();
    for (uint32_t i = 0; i < NUM_BUFFERS; ++i) {
        uint32_t currentPlaySize = std::min((uint64_t)sound->sampleCountToPlay, (uint64_t)BUFFER_ELEMENT_COUNT);
        sound->sampleCountToPlay = sound->sampleCountToPlay - currentPlaySize;

        alBufferData(sound->buffers[i], sound->format, sound->nextDataToBuffer, currentPlaySize * sizeof(int16_t), sound->asset->getSampleRate());
        sound->nextDataToBuffer = sound->nextDataToBuffer + currentPlaySize;
        if(alGetError() != AL_NO_ERROR) {
            std::cerr << "Audio buffer data failed!" << std::endl;
        }
    }

    alSourceQueueBuffers(sound->source, NUM_BUFFERS, sound->buffers);
    alSourcePlay(sound->source);
    if(alGetError() != AL_NO_ERROR)
    {
        fprintf(stderr, "Error starting :(\n");
        return false;
    }

    return true;
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

            alBufferData(buffer, sound->format, sound->nextDataToBuffer, currentPlaySize * sizeof(int16_t), sound->asset->getSampleRate());
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
