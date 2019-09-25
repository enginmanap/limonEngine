//
// Created by engin on 27.07.2016.
//

#ifndef LIMONENGINE_ASSET_H
#define LIMONENGINE_ASSET_H

#include <vector>
#include <string>


class AssetManager;//avoid cyclic include

class Asset {

protected:
    AssetManager* assetManager;
    uint32_t assetID;
    /**
     * This is an empty constructor, used to indicate what parameters the Asset constructors should have
     * @param graphicsWrapper pointer to render subsystem
     * @param assetID id of the asset, used to check if assets are shared
     * @param fileList Asset files to load
     * @return empty asset
     */
    Asset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList __attribute((unused)))
            : assetManager(assetManager), assetID(assetID) {};

public:

    uint32_t getAssetID() {
        return assetID;
    }

    virtual ~Asset() {};
};


#endif //LIMONENGINE_ASSET_H
