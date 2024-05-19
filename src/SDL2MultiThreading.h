//
// Created by engin on 12/05/2024.
//

#ifndef LIMONENGINE_SDL2MULTITHREADING_H
#define LIMONENGINE_SDL2MULTITHREADING_H

#include <SDL_atomic.h>
#include <SDL_thread.h>
#include <functional>
#include <string>
#include <vector>
#include <iostream>
#include "API/LimonTypes.h"

class SDL2MultiThreading {
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

    class Condition {
        SDL_cond* condition = SDL_CreateCond();
    public:
        void waitCondition(SDL_mutex* blockMutex) {
            SDL_LockMutex(blockMutex);
            SDL_CondWait(condition, blockMutex);
            SDL_UnlockMutex(blockMutex);
        }

        void signalWaiting() {
            SDL_CondBroadcast(condition);
        }

    };


    class InternalThread {
    protected:
        SDL_Thread* thread = nullptr;
        std::function<void*(const void*)> functionToRun;
        const void* parameters;
        void* result;
        SpinLock lock;
        std::string name;
        bool finished = false;

        static int threadRunner(void* ptr) {
            InternalThread* runnable = static_cast<InternalThread*>(ptr);
            runnable->result = runnable->functionToRun(runnable->parameters);
            runnable->finished = true;//since this is written in the lock, we don't need to sync it.
            runnable->lock.unlock();
            return 0;
        }
    public:
        InternalThread(const std::string &threadName, std::function<void*(const void*)> functionToRun, const void* parameters) {
            this->parameters = parameters;
            this->functionToRun = functionToRun;
            this->name = threadName;
        }

        void run() {
            this->lock.lock();
            thread = SDL_CreateThread(&threadRunner,name.c_str(),this);
            if(thread == nullptr) {
                std::string error = SDL_GetError();
                std::cerr << "Thread launch failure, with error" << error << std::endl;
            }
        }

        bool isThreadDone() const {
            return finished;
        }

        void* getResult() {
            if(!isThreadDone()) {
                return nullptr;
            }
            return &result;
        }

        void waitUntilDone() {
            lock.lock();
            lock.unlock();
        }

        ~InternalThread() {
            int threadReturnValue;
            SDL_WaitThread(thread, &threadReturnValue);
        }
    };

    class Thread {
    private:
        SDL_Thread* thread = nullptr;

        std::function<std::vector<LimonTypes::GenericParameter>(std::vector<LimonTypes::GenericParameter>)> functionToRun;
        std::vector<LimonTypes::GenericParameter> parameters;
        std::vector<LimonTypes::GenericParameter> result;
        SpinLock lock;
        std::string name;
        bool finished = false;

        static int threadRunner2(void* ptr) {
            Thread* runnable = static_cast<Thread*>(ptr);

            runnable->result = runnable->functionToRun(runnable->parameters);
            runnable->finished = true;//since this is written in the lock, we don't need to sync it.
            runnable->lock.unlock();
            return 0;
        }

    public:
        Thread(const std::string &threadName, std::function<std::vector<LimonTypes::GenericParameter>(std::vector<LimonTypes::GenericParameter>)> functionToRun, const std::vector<LimonTypes::GenericParameter> &parameters) {

            this->parameters = parameters;
            this->functionToRun = functionToRun;
            this->name = threadName;
        }

        void run() {
            this->lock.lock();
            thread = SDL_CreateThread(&threadRunner2,name.c_str(),this);

        }

        bool isThreadDone() const {
            return finished;
        }

        const std::vector<LimonTypes::GenericParameter>* getResult() {
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
};


#endif //LIMONENGINE_SDL2MULTITHREADING_H
