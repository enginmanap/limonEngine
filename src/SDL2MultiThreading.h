//
// Created by engin on 12/05/2024.
//

#ifndef LIMONENGINE_SDL2MULTITHREADING_H
#define LIMONENGINE_SDL2MULTITHREADING_H

#include <SDL3/SDL.h>
#include <functional>
#include <string>
#include <vector>
#include <iostream>
#include "limonAPI/LimonTypes.h"

class SDL2MultiThreading {
public:
    static inline void sleep(uint32_t milliseconds) {
        SDL_Delay(milliseconds);
    }

    class SpinLock {
        SDL_SpinLock sdlLock;
    public:
        SpinLock() {
            SDL_UnlockSpinlock(&sdlLock);
        }
        inline void lock() {
            SDL_LockSpinlock(&sdlLock);
        }
        inline void unlock() {
            SDL_UnlockSpinlock(&sdlLock);
        }
        inline bool tryLock() {
            return SDL_TryLockSpinlock(&sdlLock);
        }
        ~SpinLock() {
            unlock();
        }
    };

    class Mutex {
        SDL_Mutex* mutex;
    public:
        Mutex() : mutex(SDL_CreateMutex()) {}
        ~Mutex() { SDL_DestroyMutex(mutex); }
        Mutex(const Mutex&) = delete;
        Mutex& operator=(const Mutex&) = delete;
        inline void lock()   { SDL_LockMutex(mutex); }
        inline void unlock() { SDL_UnlockMutex(mutex); }
        inline SDL_Mutex* get() { return mutex; }
    };

    class Condition {
        SDL_Condition* condition;
    public:
        Condition() : condition(SDL_CreateCondition()) {}
        ~Condition() { SDL_DestroyCondition(condition); }
        Condition(const Condition&) = delete;
        Condition& operator=(const Condition&) = delete;

        inline void waitCondition(Mutex& blockMutex) {
            SDL_LockMutex(blockMutex.get());
            SDL_WaitCondition(condition, blockMutex.get());
            SDL_UnlockMutex(blockMutex.get());
        }
        inline void signalWaiting() {
            SDL_BroadcastCondition(condition);
        }
    };

    // General-purpose thread wrapper. Caller-supplied function owns all logic —
    // loop or one-shot. Results are returned via the userData context. Completion can
    // be polled with isThreadDone() or waited on with waitUntilDone()/join().
    class InternalThread {
        SDL_Thread* thread = nullptr;
        std::string name;
        void(*function)(void*);
        void* userData;
        void(*cleanup)(void*) = nullptr;
        SpinLock lock;
        bool finished = false;

        static int threadRunner(void* ptr) {
            InternalThread* self = static_cast<InternalThread*>(ptr);
            self->function(self->userData);
            self->finished = true;
            self->lock.unlock();
            return 0;
        }
    public:
        // Primary constructor: C-compatible, no overhead beyond the thread itself.
        InternalThread(const std::string& threadName, void(*functionToRun)(void*), void* userDataArg)
            : name(threadName), function(functionToRun), userData(userDataArg) {}

        // Convenience constructor: for engine-internal callers where a capturing lambda
        // is more readable than a static trampoline + void* context pair.
        InternalThread(const std::string& threadName, std::function<void()> functionToRun)
            : name(threadName) {
            auto* wrapper = new std::function<void()>(std::move(functionToRun));
            userData = wrapper;
            function = [](void* context) {
                (*static_cast<std::function<void()>*>(context))();
            };
            cleanup = [](void* context) {
                delete static_cast<std::function<void()>*>(context);
            };
        }
        ~InternalThread() {
            join();
            if (cleanup != nullptr) {
                cleanup(userData);
            }
        }
        InternalThread(const InternalThread&) = delete;
        InternalThread& operator=(const InternalThread&) = delete;

        void run() {
            lock.lock();
            thread = SDL_CreateThread(&threadRunner, name.c_str(), this);
            if (thread == nullptr) {
                std::cerr << "InternalThread launch failure: " << SDL_GetError() << std::endl;
            }
        }

        inline bool isThreadDone() const { return finished; }

        // Spin-waits until the thread's function returns.
        void waitUntilDone() {
            lock.lock();
            lock.unlock();
        }

        // OS-level blocking wait. Safe to call more than once.
        void join() {
            if (thread != nullptr) {
                int ret;
                SDL_WaitThread(thread, &ret);
                thread = nullptr;
            }
        }
    };

    // One-shot task: GenericParameter in/out — for use across the plugin API boundary.
    class Thread {
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
            runnable->finished = true;
            runnable->lock.unlock();
            return 0;
        }

    public:
        Thread(const std::string &threadName,
               std::function<std::vector<LimonTypes::GenericParameter>(std::vector<LimonTypes::GenericParameter>)> functionToRun,
               const std::vector<LimonTypes::GenericParameter> &parameters) {
            this->parameters = parameters;
            this->functionToRun = functionToRun;
            this->name = threadName;
        }

        void run() {
            lock.lock();
            thread = SDL_CreateThread(&threadRunner2, name.c_str(), this);
        }

        inline bool isThreadDone() const { return finished; }

        const std::vector<LimonTypes::GenericParameter>* getResult() {
            if (!isThreadDone()) return nullptr;
            return &result;
        }

        void waitUntilDone() {
            lock.lock();
            lock.unlock();
        }

        ~Thread() {
            if (thread != nullptr) {
                int ret;
                SDL_WaitThread(thread, &ret);
            }
        }
    };
};


#endif //LIMONENGINE_SDL2MULTITHREADING_H
