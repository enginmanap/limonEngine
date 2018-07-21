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

    alDopplerFactor(0.5);

    if(!ctx) {
        throw("Audio context setup failed!");
    }
    this->running = true;
    SDL_AtomicUnlock(&playRequestLock);

    thread = SDL_CreateThread(&staticSoundManager, "soundManager", this);

}

int ALHelper::soundManager() {
    while(running) {
        if(playRequests.size() > 0) { //this might miss a request because not locking, but I am ok with 10ms delay at most
            SDL_AtomicLock(&playRequestLock);
            for (size_t i = 0; i < playRequests.size(); ++i) {
                std::unique_ptr<PlayingSound>& sound = playRequests.at(i);
                if (startPlay(sound)) {
                    playingSounds[sound->soundID] = std::move(sound);
                }
            }
        }
        playRequests.clear();//moving should invalidate, so I don't remove one by one
        SDL_AtomicUnlock(&playRequestLock);
        for (auto iterator = playingSounds.begin(); iterator != playingSounds.end(); ) {
            std::unique_ptr<PlayingSound>& temp = (*iterator).second;
            if(temp->isFinished()) {
                if(temp->looped) {
                    alSourceStop(temp->source);
                    ALuint buffers;
                    ALint val;

                    alGetSourcei(temp->source, AL_BUFFERS_PROCESSED, &val);

                    alSourceUnqueueBuffers(temp->source, val, &buffers);

                    temp->sampleCountToPlay = temp->asset->getSampleCount();
                    temp->nextDataToBuffer = temp->asset->getSoundData();


                    for (uint32_t i = 0; i < NUM_BUFFERS; ++i) {
                        uint32_t currentPlaySize = std::min((uint64_t)temp->sampleCountToPlay, (uint64_t)BUFFER_ELEMENT_COUNT);
                        temp->sampleCountToPlay = temp->sampleCountToPlay - currentPlaySize;

                        alBufferData(temp->buffers[i], temp->format, temp->nextDataToBuffer, currentPlaySize * sizeof(int16_t), temp->asset->getSampleRate());
                        temp->nextDataToBuffer = temp->nextDataToBuffer + currentPlaySize;
                        ALenum error;
                        if ((error = alGetError()) != AL_NO_ERROR) {
                            std::cerr << "Loop audio buffer data failed!" << alGetString(error) << std::endl;
                        }
                    }
                    alSourceQueueBuffers(temp->source, NUM_BUFFERS, temp->buffers);
                    alSourcePlay(temp->source);

                    ++iterator;
                } else {
                    iterator = playingSounds.erase(iterator);

                }
            } else {
                refreshBuffers(temp);
                ++iterator;
            }
        }
        SDL_Delay(10);
    }
    return 0;
}

uint32_t ALHelper::stop(uint32_t soundID) {
    if(playingSounds.find(soundID) != playingSounds.end()) {
        std::unique_ptr<PlayingSound>& sound = playingSounds[soundID];
        sound->looped = false;
        alSourceStop(sound->source);
        ALenum error;
        if ((error = alGetError()) != AL_NO_ERROR) {
            std::cerr << "Stop source failed! " << alGetString(error) << std::endl;
        }

    }
    return 0;
}

uint32_t ALHelper::play(const SoundAsset* soundAsset, bool looped) {
    uint32_t id = getNextRequestID();
    auto sound = std::make_unique<PlayingSound>(id);
    sound->asset = soundAsset;
    sound->looped = looped;

    SDL_AtomicLock(&playRequestLock);
    this->playRequests.push_back(std::move(sound));
    SDL_AtomicUnlock(&playRequestLock);
    return id;
}


bool ALHelper::startPlay(std::unique_ptr<PlayingSound> &sound) {

    alGenBuffers(NUM_BUFFERS, sound->buffers);
    alGenSources(1, &sound->source);

    alSourcef(sound->source,AL_GAIN,1000.0f);

    if(alGetError() != AL_NO_ERROR) {
        std::cerr << "Audio buffer setup failed!" << std::endl;
        return false;
    }

    sound->format = to_al_format(sound->asset->getChannels(), 16);

    sound->nextDataToBuffer = sound->asset->getSoundData();
    sound->sampleCountToPlay = sound->asset->getSampleCount();
    for (uint32_t i = 0; i < NUM_BUFFERS; ++i) {
        uint32_t currentPlaySize = std::min((uint64_t)sound->sampleCountToPlay, (uint64_t)BUFFER_ELEMENT_COUNT);
        sound->sampleCountToPlay = sound->sampleCountToPlay - currentPlaySize;

        alBufferData(sound->buffers[i], sound->format, sound->nextDataToBuffer, currentPlaySize * sizeof(int16_t), sound->asset->getSampleRate());
        sound->nextDataToBuffer = sound->nextDataToBuffer + currentPlaySize;
        ALenum error;
        if ((error = alGetError()) != AL_NO_ERROR) {
            std::cerr << "Audio buffer data failed with error " << alGetString(error) << std::endl;
        }
    }

    alSourceQueueBuffers(sound->source, NUM_BUFFERS, sound->buffers);
    alSourcePlay(sound->source);
    if(alGetError() != AL_NO_ERROR) {
        std::cerr << "Error starting sound playback" << std::endl;

        return false;
    }

    return true;
}


bool ALHelper::refreshBuffers(std::unique_ptr<ALHelper::PlayingSound> &sound) {
        ALuint buffer;
        ALint val;

        ALenum error;
//        Check if OpenAL is done with any of the queued buffers
        alGetSourcei(sound->source, AL_BUFFERS_PROCESSED, &val);
        if (alGetError() != AL_NO_ERROR) {
            std::cerr << "Error getting buffer state" << std::endl;
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
            uint32_t currentPlaySize = std::min((uint64_t) sound->sampleCountToPlay, (uint64_t) BUFFER_ELEMENT_COUNT);
            sound->sampleCountToPlay = sound->sampleCountToPlay - currentPlaySize;
            if(currentPlaySize > 0) {
                alBufferData(buffer, sound->format, sound->nextDataToBuffer, currentPlaySize * sizeof(int16_t),
                             sound->asset->getSampleRate());
                if ((error = alGetError()) != AL_NO_ERROR) {
                    std::cerr << "Error buffering alGenBuffers : " << alGetString(error) << std::endl;
                    return 1;
                }
                sound->nextDataToBuffer = sound->nextDataToBuffer + currentPlaySize;
                alSourceQueueBuffers(sound->source, 1, &buffer);
                if ((error = alGetError()) != AL_NO_ERROR) {
                    std::cerr << "Error source buffering : %s" << alGetString(error) << std::endl;
                    return 1;
                }
            }
        }

    return true;
}

bool ALHelper::PlayingSound::isFinished() {
    ALint source_state;
    alGetSourcei(source, AL_SOURCE_STATE, &source_state);
    ALenum error;
    if ((error = alGetError()) != AL_NO_ERROR) {
        std::cerr << "Error checking is finished! " << alGetString(error) << std::endl;
    }
    return source_state != AL_PLAYING;
}

ALHelper::PlayingSound::~PlayingSound() {
    alSourceUnqueueBuffers(source, NUM_BUFFERS, buffers);
    alDeleteBuffers(NUM_BUFFERS, buffers);
    alDeleteSources(1, &source);

    ALenum error;
    if ((error = alGetError()) != AL_NO_ERROR) {
        std::cerr << "Error deleting the playing source! " << alGetString(error) << std::endl;

        return;
    }
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