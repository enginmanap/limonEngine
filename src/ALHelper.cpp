//
// Created by engin on 11.07.2018.
//

#include "ALHelper.h"
#include "Assets/SoundAsset.h"

#define DR_WAV_IMPLEMENTATION
#include "../libs/dr_wav.h"

#include <stdio.h>
#include <sys/types.h>
#include <SDL_atomic.h>
#include <SDL_thread.h>
#include <iostream>
#include <SDL_timer.h>

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
    SDL_AtomicUnlock(&playRequestLock);

    thread = SDL_CreateThread(&staticSoundManager, "soundManager", this);

}

int ALHelper::soundManager() {
    while(running || paused) {
        if(paused && running) { //the first cycle after pause request
            for (auto iterator = playingSounds.begin(); iterator != playingSounds.end();++iterator) {
                alSourcePause(iterator->second->source);
            }
            running = false;
        } else if(resumed) {
            for (auto iterator = playingSounds.begin(); iterator != playingSounds.end();++iterator) {
                if(!iterator->second->stopped && !iterator->second->paused) {//don't start stopped sounds.
                    alSourcePlay(iterator->second->source);
                }
            }
            running = true;
            paused = false;
            resumed = false;
        } else if(running) {
            if (playRequests.size() > 0) { //this might miss a request because not locking, but I am ok with 10ms delay at most
                SDL_AtomicLock(&playRequestLock);
                for (size_t i = 0; i < playRequests.size(); ++i) {
                    std::unique_ptr<PlayingSound> &sound = playRequests.at(i);
                    if (startPlay(sound)) {
                        playingSounds[sound->soundID] = std::move(sound);
                    }
                }
                playRequests.clear();//moving should invalidate, so I don't remove one by one
                SDL_AtomicUnlock(&playRequestLock);
            }
            removeSoundLock.lock();
            for (auto iterator = playingSounds.begin(); iterator != playingSounds.end();) {
                std::unique_ptr<PlayingSound> &temp = (*iterator).second;
                ALint state;
                alGetSourcei(temp->source, AL_SOURCE_STATE, &state);
                if(state == AL_STOPPED && !temp->looped) {
                    iterator = playingSounds.erase(iterator);
                } else if (temp->isFinished()) {
                    if (temp->looped) {
                        alSourceStop(temp->source);
                        ALuint buffers[NUM_BUFFERS];
                        ALint val;
                        ALenum error;

                        alGetSourcei(temp->source, AL_BUFFERS_PROCESSED, &val);
                        alSourceUnqueueBuffers(temp->source, val, buffers);
                        if ((error = alGetError()) != AL_NO_ERROR) {
                            std::cerr << "Loop audio buffer data failed!" << alGetString(error) << std::endl;
                        }

                        temp->sampleCountToPlay = temp->asset->getSampleCount();
                        temp->nextDataToBuffer = temp->asset->getSoundData();

                        for (uint32_t i = 0; i < NUM_BUFFERS; ++i) {
                            uint32_t currentPlaySize = std::min((uint64_t) temp->sampleCountToPlay,
                                                                (uint64_t) BUFFER_ELEMENT_COUNT);
                            temp->sampleCountToPlay = temp->sampleCountToPlay - currentPlaySize;
                            alBufferData(temp->buffers[i], temp->format, temp->nextDataToBuffer,
                                         currentPlaySize * sizeof(int16_t), temp->asset->getSampleRate());
                            temp->nextDataToBuffer = temp->nextDataToBuffer + currentPlaySize;
                            if ((error = alGetError()) != AL_NO_ERROR) {
                                std::cerr << "Loop audio buffer data failed!" << alGetString(error) << std::endl;
                            }
                        }
                        alSourceQueueBuffers(temp->source, NUM_BUFFERS, temp->buffers);
                        alSourcePlay(temp->source);
                        iterator++;
                    } else {//non looped finished sound
                        iterator = playingSounds.erase(iterator);


                    }
                } else {
                    refreshBuffers(temp);
                    ++iterator;
                }
            }
            removeSoundLock.unlock();
        }
        SDL_Delay(10);
    }
    return 0;
}

bool ALHelper::stop(uint32_t soundID) {
    if(playingSounds.find(soundID) != playingSounds.end()) {
        std::unique_ptr<PlayingSound>& sound = playingSounds[soundID];
        sound->looped = false;
        removeSoundLock.lock();
        alSourceStop(sound->source);
        sound->stopped = true;
        removeSoundLock.unlock();
        ALenum error;
        if ((error = alGetError()) != AL_NO_ERROR) {
            std::cerr << "Stop source failed! " << alGetString(error) << std::endl;
            return false;
        }
        return true;
    } else {
        bool result = false;
        SDL_AtomicLock(&playRequestLock);
        for (auto request = playRequests.begin(); request != playRequests.end(); ++request) {
            if((*request)->soundID == soundID) {
                this->playRequests.erase(request);
                result = true;
                break;
            }
        }
        SDL_AtomicUnlock(&playRequestLock);
        return result;
    }
}

bool ALHelper::pause(uint32_t soundID) {
    if(playingSounds.find(soundID) != playingSounds.end()) {
        std::unique_ptr<PlayingSound>& sound = playingSounds[soundID];
        removeSoundLock.lock();
        alSourcePause(sound->source);
        sound->paused = true;
        removeSoundLock.unlock();
        ALenum error;
        if ((error = alGetError()) != AL_NO_ERROR) {
            std::cerr << "Pause source failed! " << alGetString(error) << std::endl;
            return false;
        }
        return true;
    } else {
        bool result = false;
        SDL_AtomicLock(&playRequestLock);
        for (auto request = playRequests.begin(); request != playRequests.end(); ++request) {
            if((*request)->soundID == soundID) {
                (*request)->paused = true;
                result = true;
                break;
            }
        }
        SDL_AtomicUnlock(&playRequestLock);
        return result;
    }
}

bool ALHelper::resume(uint32_t soundID) {
    if(playingSounds.find(soundID) != playingSounds.end()) {
        std::unique_ptr<PlayingSound>& sound = playingSounds[soundID];
        removeSoundLock.lock();
        alSourcePlay(sound->source);
        sound->paused = false;
        removeSoundLock.unlock();
        ALenum error;
        if ((error = alGetError()) != AL_NO_ERROR) {
            std::cerr << "Pause source failed! " << alGetString(error) << std::endl;
            return false;
        }
        return true;
    } else {
        bool result = false;
        SDL_AtomicLock(&playRequestLock);
        for (auto request = playRequests.begin(); request != playRequests.end(); ++request) {
            if((*request)->soundID == soundID) {
                (*request)->paused = false;
                result = true;
                break;
            }
        }
        SDL_AtomicUnlock(&playRequestLock);
        return result;
    }
}

uint32_t ALHelper::play(std::shared_ptr<SoundAsset> soundAsset, bool looped, float gain) {
    uint32_t id = getNextRequestID();
    auto sound = std::unique_ptr<PlayingSound>(new PlayingSound(id));
    sound->asset = soundAsset;
    sound->looped = looped;
    sound->gain = gain;

    SDL_AtomicLock(&playRequestLock);
    this->playRequests.push_back(std::move(sound));
    SDL_AtomicUnlock(&playRequestLock);
    return id;
}

bool ALHelper::startPlay(std::unique_ptr<PlayingSound> &sound) {

    alGenBuffers(NUM_BUFFERS, sound->buffers);
    alGenSources(1, &sound->source);

    alSourcef(sound->source,AL_GAIN,sound->gain);

    if (sound->isPositionRelative) {
        alSourcei(sound->source, AL_SOURCE_RELATIVE, AL_TRUE);
    } else {
        alSourcei(sound->source, AL_SOURCE_RELATIVE, AL_FALSE);
    }

    alSource3f(sound->source, AL_POSITION, sound->position.x, sound->position.y, sound->position.z);

    ALenum error = alGetError();
    if(error != AL_NO_ERROR) {
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
    if(!sound->paused) {
        alSourcePlay(sound->source);
    }
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
    return !(source_state == AL_PLAYING || source_state == AL_PAUSED);
}

ALHelper::PlayingSound::~PlayingSound() {
    if(source != 0) { //if 0 it means requested and removed before start playing
        alSourceStop(source);
        ALenum error;
        ALint val;
        ALuint tempBuffers[NUM_BUFFERS];

        alGetSourcei(source, AL_BUFFERS_PROCESSED, &val);


        alSourceUnqueueBuffers(source, val, tempBuffers);
        if ((error = alGetError()) != AL_NO_ERROR) {
            std::cerr << "Error sound source unqueue before delete! " << alGetString(error) << std::endl;
        }

        alDeleteBuffers(NUM_BUFFERS, buffers);
        if ((error = alGetError()) != AL_NO_ERROR) {
            std::cerr << "Error sound source delete buffer ! " << alGetString(error) << std::endl;
        }

        alDeleteSources(1, &source);
        if ((error = alGetError()) != AL_NO_ERROR) {
            std::cerr << "Error deleting the playing source! " << alGetString(error) << std::endl;
        }
    }

}

ALHelper::~ALHelper() {
    this->running = false;
    this->paused = false;
    int threadReturnValue;
    SDL_WaitThread(thread, &threadReturnValue);

    dev = alcGetContextsDevice(ctx);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(ctx);
    alcCloseDevice(dev);

}

bool ALHelper::setLooped(uint32_t soundID, bool looped) {
    if(playingSounds.find(soundID) != playingSounds.end()) {
        playingSounds[soundID]->looped = looped;
        return true;
    } else {
        bool result = false;
        SDL_AtomicLock(&playRequestLock);
        for (auto request = playRequests.begin(); request != playRequests.end(); ++request) {
            if((*request)->soundID == soundID) {
                (*request)->looped = looped;
                result = true;
                break;
            }
        }
        SDL_AtomicUnlock(&playRequestLock);
        return result;
    }
}
