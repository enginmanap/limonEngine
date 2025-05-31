//
// Created by engin on 27.07.2016.
//

#ifndef LIMONENGINE_ASSET_H
#define LIMONENGINE_ASSET_H

#include <vector>
#include <string>
#include <iostream>
#include <thread>
#include <mutex>
#include "Utils/StringUtils.hpp"
#ifdef CEREAL_SUPPORT
#include <cereal/archives/xml.hpp>
#endif


class AssetManager;//avoid cyclic include

class Asset {
public:
    enum class LoadState {INITIATED, CPU_LOAD_STARTED, CPU_LOAD_DONE, DONE};
private:
    friend class AssetManager;
    LoadState loadState = LoadState::INITIATED;
    std::mutex loadStateSetMutex;
    void setLoadState(LoadState state) {
        std::unique_lock<std::mutex> lock(loadStateSetMutex);
        loadState = state;
    }

    virtual void loadCPUPart() = 0;
    virtual void loadGPUPart() = 0;

    /**
 * Override loadCPU and loadGPU method, and don't change this.
 */
    void load() {
        loadCPUPart();
        loadGPUPart();
        this->setLoadState(LoadState::DONE);
    };
protected:
    AssetManager* assetManager;
    uint32_t assetID;
    std::vector<std::string> fileList;
    /**
     * This is an empty constructor, used to indicate what parameters the Asset constructors must have.
     * It should construct the basic object, and allow initialization afterwards, possibly on another thread.
     * @param AssetManager pointer to asset manager used to get render subsystem, sound subsystem etc.
     * @param assetID id of the asset, used to check if assets are shared
     * @param fileList Asset files to load
     * @return empty asset
     */
    Asset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList)
            : assetManager(assetManager), assetID(assetID), fileList(fileList) {};

    /**
 * This is a constructor that is used for cereal loading.
 */
#ifdef CEREAL_SUPPORT
    Asset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList, cereal::BinaryInputArchive& binaryArchive [[gnu::unused]])
            : assetManager(assetManager), assetID(assetID), fileList(fileList) {};
#endif

public:
    uint32_t getAssetID() const {
        return assetID;
    }

    std::string getName() const {
        return StringUtils::join(fileList, ",");
    }

    LoadState getLoadState() {
        std::unique_lock<std::mutex> lock(loadStateSetMutex);
        return loadState;
    }


    virtual ~Asset() = default;
};


#endif //LIMONENGINE_ASSET_H
