#ifndef LIMONENGINE_AIGRIDWORKERPOOL_H
#define LIMONENGINE_AIGRIDWORKERPOOL_H

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

//Runs one job on every worker and waits for all of them. Worker 0 is the calling thread, but can be non-main
class AIGridWorkerPool {
    std::vector<std::thread> threads;
    std::mutex mutex;
    std::condition_variable startCondition;
    std::condition_variable doneCondition;
    std::function<void(size_t)> currentJob;
    uint64_t generation = 0;
    size_t pendingWorkers = 0;
    bool isStopping = false;

    void workerLoop(size_t workerIndex);

public:
    explicit AIGridWorkerPool(size_t workerCount);
    ~AIGridWorkerPool();

    AIGridWorkerPool(const AIGridWorkerPool &) = delete;
    AIGridWorkerPool &operator=(const AIGridWorkerPool &) = delete;

    size_t getWorkerCount() const {
        return threads.size() + 1;
    }

    void run(const std::function<void(size_t)> &job);
};

#endif //LIMONENGINE_AIGRIDWORKERPOOL_H
