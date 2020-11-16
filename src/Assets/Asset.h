//
// Created by engin on 27.07.2016.
//

#ifndef LIMONENGINE_ASSET_H
#define LIMONENGINE_ASSET_H

#include <vector>
#include <string>
#ifdef CEREAL_SUPPORT
#include <cereal/archives/xml.hpp>
#endif


class AssetManager;//avoid cyclic include

class Asset {

protected:
    AssetManager* assetManager;
    uint32_t assetID;
    /**
     * This is an empty constructor, used to indicate what parameters the Asset constructors must have.
     * @param graphicsWrapper pointer to render subsystem
     * @param assetID id of the asset, used to check if assets are shared
     * @param fileList Asset files to load
     * @return empty asset
     */
    Asset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList [[gnu::unused]])
            : assetManager(assetManager), assetID(assetID) {};

    /**
 * This is a constructor that is used for cereal loading.
 */
#ifdef CEREAL_SUPPORT
    Asset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList [[gnu::unused]], cereal::BinaryInputArchive& binaryArchive [[gnu::unused]])
            : assetManager(assetManager), assetID(assetID) {};
#endif

public:

    uint32_t getAssetID() const {
        return assetID;
    }

    virtual ~Asset() = default;
};


#endif //LIMONENGINE_ASSET_H
