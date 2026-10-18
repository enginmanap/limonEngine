//
// Created by engin on 18.10.2026.
//

#ifndef LIMONENGINE_LOADREQUESTER_H
#define LIMONENGINE_LOADREQUESTER_H

#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

class Asset;
class AssetManager;

//one owner's loads. While a requester is open, other threads' requesters wait, closing it is how the owner says its results are processed
class LoadRequester {
    friend class AssetManager;
public:
    enum class GPUMode {
        SKIP,//handed over after the CPU part, nobody runs the GPU part
        SELF,//handed over after the CPU part, the owner runs the GPU part on the main thread
        UPLOADED//handed over DONE, the world runs the GPU part, one asset per frame
    };

    struct LoadResult {
        std::vector<std::string> files;
        std::shared_ptr<Asset> asset;
        std::string failureReason;//only report mode gets here with one. A failed asset is out of the map already, don't free it
    };

    LoadRequester(const LoadRequester &) = delete;
    LoadRequester &operator=(const LoadRequester &) = delete;

    //releases what was not handed over and lets the next requester start. Only the thread that opened it may close it
    void close();

    ~LoadRequester();
private:
    struct PendingRequest {
        std::vector<std::string> files;
        std::shared_ptr<Asset> (*createAsset)(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &files);//function pointer to create an asset
    };

    struct TrackedRequest {
        std::vector<std::string> files;
        std::shared_ptr<Asset> asset;
        std::string failureReason;
    };

    AssetManager *assetManager;
    const bool reportFailures;//a failed load is handed back with its reason instead of stopping the engine
    const GPUMode gpuMode;
    const uint32_t maxInFlight;//0 is no limit
    const std::thread::id ownerThread;

    //below are guarded by AssetManager::requestersMutex
    std::vector<PendingRequest> pending;//not started yet, the requester isn't running or has no free slot
    std::vector<TrackedRequest> inFlight;//started, holds a reference until it is handed over
    std::vector<TrackedRequest> finished;//handed over by whoever finished it, waiting for the owner to take it
    bool running = false;
    bool closed = false;

    LoadRequester(AssetManager *assetManager, bool reportFailures, GPUMode gpuMode, uint32_t maxInFlight)
            : assetManager(assetManager), reportFailures(reportFailures), gpuMode(gpuMode), maxInFlight(maxInFlight),
              ownerThread(std::this_thread::get_id()) {}
};

#endif //LIMONENGINE_LOADREQUESTER_H
