//
// Created by engin on 27.07.2016.
//

#ifndef LIMONENGINE_ASSET_H
#define LIMONENGINE_ASSET_H

#include <vector>
#include <string>
#include "../GLHelper.h"

class AssetManager;//avoid cyclic include

class Asset {

protected:
    AssetManager* assetManager;
    /**
     * This is an empty constructor, used to indicate what parameters the Asset constructors should have
     * @param glHelper pointer to render subsystem
     * @param fileList Asset files to load
     * @return empty asset
     */
    Asset(AssetManager* assetManager, const std::vector<std::string> &fileList): assetManager(assetManager) {};
};


#endif //LIMONENGINE_ASSET_H
