//
// Created by Engin Manap on 10.02.2016.
//

#ifndef LIMONENGINE_SDL2HELPER_CPP_H
#define LIMONENGINE_SDL2HELPER_CPP_H

#include <iostream>

#include <SDL2/SDL.h>
#include <SDL_atomic.h>
#include <SDL_thread.h>
#include <functional>
#include "Options.h"
#include "API/LimonAPI.h"


class SDL2Helper {
public:
    class SpinLock {
    private:
        SDL_SpinLock sdlLock;
    public:
        SpinLock() {
            SDL_AtomicUnlock(&sdlLock);
        }
        void lock() {
            SDL_AtomicLock(&sdlLock);
        }
        void unlock() {
            SDL_AtomicUnlock(&sdlLock);
        };

        bool tryLock() {
            return SDL_AtomicTryLock(&sdlLock);
        }

        ~SpinLock() {
            this->unlock();
        }
    };
private:
    SDL_Window *window;
    SDL_GLContext context;
    Options* options;

public:
    class Thread {
    private:
        SDL_Thread* thread = nullptr;
        std::function<std::vector<LimonAPI::ParameterRequest>(std::vector<LimonAPI::ParameterRequest>)> functionToRun;
        std::vector<LimonAPI::ParameterRequest> parameters;
        std::vector<LimonAPI::ParameterRequest> result;
        SpinLock lock;
        std::string name;
        static int threadRunner(void* ptr) {
            Thread* runable = static_cast<Thread*>(ptr);

            runable->lock.lock();
            runable->result = runable->functionToRun(runable->parameters);
            runable->lock.unlock();
            return 0;
        }
    public:
        Thread(const std::string &threadName, std::function<std::vector<LimonAPI::ParameterRequest>(std::vector<LimonAPI::ParameterRequest>)> functionToRun, const std::vector<LimonAPI::ParameterRequest> &parameters) {
            this->parameters = parameters;
            this->functionToRun = functionToRun;
            this->name = threadName;
        }

        void run() {
            thread = SDL_CreateThread(&threadRunner,name.c_str(),this);
        }

        bool isThreadDone() {
            bool isDone = lock.tryLock();
            if(isDone) {
                lock.unlock();
            }
            return isDone;
        }

        const std::vector<LimonAPI::ParameterRequest>* getResult() {
            if(!isThreadDone()) {
                return nullptr;
            }
            return &result;
        }

        void waitUntilDone() {
            lock.lock();
            lock.unlock();
        }

        ~Thread() {
            int threadReturnValue;
            SDL_WaitThread(thread, &threadReturnValue);
        }
    };

    void setFullScreen(bool isFullScreen);

    SDL2Helper(const char *, Options* options);

    ~SDL2Helper();

    void swap() {
        SDL_GL_SwapWindow(window);
        options->setIsWindowInFocus(SDL_GetWindowFlags(window) & SDL_WINDOW_MOUSE_FOCUS);
    };

    bool loadSharedLibrary(const std::string& fileName);

    SDL_Window *getWindow();

    static uint32_t getLogicalCPUCount() {
        return SDL_GetCPUCount();
    }

    bool loadTriggers(void *objectHandle) const;
    bool loadPlayerExtensions(void *objectHandle) const;
    bool loadActors(void *objectHandle) const;
    bool loadRenderMethods(void* objectHandle) const;

};

#endif //LIMONENGINE_SDL2HELPER_CPP_H
