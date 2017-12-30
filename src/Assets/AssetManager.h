//
// Created by engin on 27.07.2016.
//

#ifndef LIMONENGINE_ASSETMANAGER_H
#define LIMONENGINE_ASSETMANAGER_H


#include <string>
#include <map>

#include "Asset.h"


class AssetManager {
    std::map<const std::vector<std::string>, Asset *> assets;
    GLHelper *glHelper;
public:

    GLHelper *getGlHelper() const {
        return glHelper;
    }

public:

    AssetManager(GLHelper *glHelper) : glHelper(glHelper) {}

    template<class T>
    T *loadAsset(const std::vector<std::string> files) {
        if (assets.count(files) == 0) {
            assets[files] = new T(this, files);
        }
        return (T *) assets[files];
    };


};


#endif //LIMONENGINE_ASSETMANAGER_H
