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

    /**
     * Apparently a condition can wake randomly, and that is allowed. Meaning you can't use Condition
     * as a way to determine if you should work, but as a way to check if you should work, through some other
     * mean. Like a frame indicator etc.
     */
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

    /**
     * Since Semaphore keeps the count within, it does the check you are suppose to do with Condition itself.
     * Therefore, you can use Semaphore as a signal to start working, unlike Condition
     */
    class Semaphore {
        SDL_Semaphore* semaphore;
    public:
        explicit Semaphore(uint32_t initialValue = 0) : semaphore(SDL_CreateSemaphore(initialValue)) {}
        ~Semaphore() { SDL_DestroySemaphore(semaphore); }
        Semaphore(const Semaphore&) = delete;
        Semaphore& operator=(const Semaphore&) = delete;

        //Block until positive, then consume (1)
        inline void wait() { SDL_WaitSemaphore(semaphore); }
        //Increase the counter
        inline void signal() { SDL_SignalSemaphore(semaphore); }
        //Used to get the current value. Used to detect specific use case violations in Latch/Signal pair
        inline uint32_t getValue() const { return SDL_GetSemaphoreValue(semaphore); }
    };

    /**
     * This is a specialization of Semaphore.
     *
     * We know our visibility system is one signaller, one waiter. This pair allows us to catch if that assumption
     * doesn't hold, by logging errors if it doesn't.
     *
     * If you need more than one of either/both, use Semaphore
     */
    class Latch {
        Semaphore semaphore;
        std::string name;
    public:
        explicit Latch(const std::string& latchName = "") : name(latchName) {}
        Latch(const Latch&) = delete;
        Latch& operator=(const Latch&) = delete;

        void setName(const std::string& latchName) { name = latchName; }

        /*
         * Wrapper for signal of the Semaphore, that doesn't allow multiple signals
         *
         * Logs error if you try.
         */
        void signal() {
            if (semaphore.getValue() != 0) {
                std::cerr << "Latch " << name << " signalled again before its previous signal was consumed. "
                          << "Dropping it; something is dispatching a turn nobody asked for." << std::endl;
                return;
            }
            semaphore.signal();
        }

        void wait() { semaphore.wait(); }
    };

    /**
     *
     * Barrier with multiple arrivals.
     *
     * If we have some left over, then we log error, meaning something is wrong
     */
    class Barrier {
        Semaphore semaphore;
        std::string name;
    public:
        explicit Barrier(const std::string& barrierName = "") : name(barrierName) {}
        Barrier(const Barrier&) = delete;
        Barrier& operator=(const Barrier&) = delete;

        //one worker reporting that its turn is finished
        void arrive() { semaphore.signal(); }

        //consumes exactly expectedArrivals, then reports anything left over
        void waitForAll(size_t expectedArrivals) {
            for (size_t i = 0; i < expectedArrivals; ++i) {
                semaphore.wait();
            }
            uint32_t leftOver = semaphore.getValue();
            if (leftOver != 0) {
                std::cerr << "Barrier " << name << " had " << leftOver << " arrival(s) beyond the "
                          << expectedArrivals << " dispatched. A worker ran a turn nobody started." << std::endl;
            }
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
