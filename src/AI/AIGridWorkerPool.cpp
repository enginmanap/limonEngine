#include "AIGridWorkerPool.h"

AIGridWorkerPool::AIGridWorkerPool(size_t workerCount) {
    for (size_t workerIndex = 1; workerIndex < workerCount; ++workerIndex) {
        threads.emplace_back(&AIGridWorkerPool::workerLoop, this, workerIndex);
    }
}

AIGridWorkerPool::~AIGridWorkerPool() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        isStopping = true;
    }
    startCondition.notify_all();
    for (std::thread &thread : threads) {
        thread.join();
    }
}

void AIGridWorkerPool::workerLoop(size_t workerIndex) {
    uint64_t seenGeneration = 0;
    while (true) {
        std::function<void(size_t)> job;
        {
            std::unique_lock<std::mutex> lock(mutex);
            while (!isStopping && generation == seenGeneration) {
                startCondition.wait(lock);
            }
            if (isStopping) {
                return;
            }
            seenGeneration = generation;
            job = currentJob;
        }
        job(workerIndex);
        {
            std::lock_guard<std::mutex> lock(mutex);
            pendingWorkers--;
            if (pendingWorkers == 0) {
                doneCondition.notify_one();
            }
        }
    }
}

void AIGridWorkerPool::run(const std::function<void(size_t)> &job) {
    {
        std::lock_guard<std::mutex> lock(mutex);
        currentJob = job;
        pendingWorkers = threads.size();
        generation++;
    }
    startCondition.notify_all();
    job(0);
    std::unique_lock<std::mutex> lock(mutex);
    while (pendingWorkers != 0) {
        doneCondition.wait(lock);
    }
}
