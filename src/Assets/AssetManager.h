//
// Created by engin on 27.07.2016.
//

#ifndef LIMONENGINE_ASSETMANAGER_H
#define LIMONENGINE_ASSETMANAGER_H


#include <string>
#include <map>
#include <utility>

#include "Asset.h"


class AssetManager {
    //second of the pair is how many times load requested. prework for unload
    std::map<const std::vector<std::string>, std::pair<Asset *, uint32_t>> assets;
    GLHelper *glHelper;
public:

    GLHelper *getGlHelper() const {
        return glHelper;
    }

public:

    explicit AssetManager(GLHelper *glHelper) : glHelper(glHelper) {}

    template<class T>
    T *loadAsset(const std::vector<std::string> files) {
        if (assets.count(files) == 0) {

            assets[files] = std::make_pair(new T(this, files), 0);
        }

        assets[files].second++;
        return (T *) assets[files].first;
    }

    void freeAsset(const std::vector<std::string> files) {
        if (assets.count(files) == 0) {
            std::cerr << "Unloading an asset that was not loaded. skipping" << std::endl;
            return;
        }
        assets[files].second--;
        if (assets[files].second == 0) {
            //last element that requested the load freed, delete the object
            Asset *assetToRemove = assets[files].first;
            delete assetToRemove;
            assets.erase(files);
        }
    }

    ~AssetManager() {
        //free all the assets
        for (std::map<const std::vector<std::string>, std::pair<Asset *, uint32_t>>::iterator it = assets.begin();
             it != assets.end(); it++) {
            delete it->second.first;
        }
    }

};


#endif //LIMONENGINE_ASSETMANAGER_H
