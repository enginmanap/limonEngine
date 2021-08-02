//
// Created by engin on 27.07.2016.
//

#ifndef LIMONENGINE_ASSET_H
#define LIMONENGINE_ASSET_H

#include <vector>
#include <string>
#include <iostream>
#include <thread>
#ifdef CEREAL_SUPPORT
#include <cereal/archives/xml.hpp>
#endif


class AssetManager;//avoid cyclic include

class Asset {

protected:
    AssetManager* assetManager;
    uint32_t assetID;
    bool loadStarted = false;
    bool loadFinished = false;
    //SDL2Helper::SpinLock loadingLock;// should lock on load start, and unlock at load end.
    /**
     * This is an empty constructor, used to indicate what parameters the Asset constructors must have.
     * It should construct the basic object, and allow initialization afterwards, possibly on another thread.
     * @param AssetManager pointer to asset manager used to get render subsystem, sound subsystem etc.
     * @param assetID id of the asset, used to check if assets are shared
     * @param fileList Asset files to load
     * @return empty asset
     */
    Asset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList [[gnu::unused]])
            : assetManager(assetManager), assetID(assetID) {};

    virtual void loadInternal() = 0;

    /**
 * This is a constructor that is used for cereal loading.
 */
#ifdef CEREAL_SUPPORT
    Asset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList [[gnu::unused]], cereal::BinaryInputArchive& binaryArchive [[gnu::unused]])
            : assetManager(assetManager), assetID(assetID) {};
#endif

public:

    /**
     * Override loadInternal method, and don't change this.
     */
    void load() {
        //loadingLock.lock();
        loadStarted = true;
        loadInternal();
        loadFinished = true;
        //loadingLock.unlock();
    };

    uint32_t getAssetID() const {
        return assetID;
    }

    bool isLoadFinished() const {
        return loadFinished;
    }

    void waitUntilLoadFinish() {
        if(loadFinished) {
            return;
        }
        if(!loadStarted) {
            std::cerr << "waiting before starting the load, sleeping to get valid state" << std::endl;
            while (!loadStarted) {
                std::this_thread::sleep_for(std::chrono::milliseconds (10));
            }
        }
        //now we started, wait until we finish
        while (!loadFinished) {
            std::this_thread::sleep_for(std::chrono::milliseconds (10));
        }

        //loadingLock.lock();
        //loadingLock.unlock();

    }

    virtual ~Asset() = default;
};


#endif //LIMONENGINE_ASSET_H
