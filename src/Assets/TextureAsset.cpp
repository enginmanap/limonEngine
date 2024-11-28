//
// Created by Engin manap on 1.03.2016.
//

#include "TextureAsset.h"

#include <GameObjects/Players/FreeCursorPlayer.h>

TextureAsset::TextureAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &files) :
        Asset(assetManager, assetID, files) {
    if (files.empty()) {
        std::cerr << "Texture load failed because file name vector is empty." << std::endl;
        exit(-1);
    }
    name = files;
    if (files.size() == 1) {
        //in case of texture only (No model asset) we duplicate the file name
        this->name.emplace_back(files[0]);
    }
    if (files.size() > 3) {
        std::cerr << "multiple files are sent to Texture constructor, extra elements ignored." << std::endl;
    }
}


/**
 * Loads the CPU part of the texture asset. This function handles both embedded textures
 * and textures loaded from disk. If the texture is embedded, it extracts the texture data
 * from the asset manager using the texture ID and owner asset file. If the texture is not
 * embedded, it loads the texture from the disk using the provided file path. The function
 * also ensures that the loaded texture has the correct pixel format and updates the texture
 * metadata accordingly. In case of errors, appropriate error messages are printed and the
 * program exits.
 */

void TextureAsset::loadCPUPart() {

    if (name.size() == 3) {//If embedded texture is needed, first element is the index, second is the owner asset file
        //index is a string, first char is * second char is the index
        std::cout << "Texture request has 3 elements. Attempting to extract embedded texture. " << std::endl;
        int textureID = std::atoi(&name[1][1]);
        std::shared_ptr<const AssetManager::EmbeddedTexture> embeddedTexture = assetManager->getEmbeddedTextures(name[2], textureID);
        if(embeddedTexture != nullptr) {
            SDL_RWops* rwop = nullptr;
            if(embeddedTexture->height == 0) {
                rwop = SDL_RWFromMem( (void*)embeddedTexture->texelData.data(), embeddedTexture->width);
            } else {
                rwop = SDL_RWFromMem((void*)embeddedTexture->texelData.data(), embeddedTexture->width * embeddedTexture->height);
            }
            cpuSurface = IMG_Load_RW(rwop, 0);
        } else {
            std::cerr << "Embedded texture can't be found with following information: " << name[2] << ":" << textureID << std::endl;
        }
    } else {
        /**
         * We have a path at hand, but we don't know if thats where the texture is. We will try alternatives
         * 1) path itself
         * 2) get the filename from the full path, get the path from the loader asset, maybe the texture is side by side with the model?
         * 3) as with 2, but maybe texture is in the same folder as the model but under textures folder?
         * 4) Texture should be under ./data/texture/+ same as model
         *
         * if all fails then we fail ungracefully
         */

        //Data\Models\Polygon\AncientEmpire
        cpuSurface = IMG_Load(name[1].data());
        if(!cpuSurface) {
            const std::string textureFullFileName = name[1].substr(name[1].find_last_of("\\/") + 1);
            const AssetManager::AvailableAssetsNode *fullMatchAssets = assetManager->getAvailableAssetsTreeFiltered(AssetManager::Asset_type_TEXTURE, textureFullFileName);
            if (fullMatchAssets != nullptr) {
                std::vector<std::string> possibleAssets;
                getPossibleTexturesList(fullMatchAssets, possibleAssets);

                if (!possibleAssets.empty()) {
                    for (size_t i = 0; i < possibleAssets.size(); ++i) {
                        cpuSurface = IMG_Load(possibleAssets[i].c_str());
                        if (cpuSurface) {
                            break;
                        }
                    }
                }
            }
            if (!cpuSurface) {
                std::cerr << "Couldn't find any asset with same name and same extension for "<< textureFullFileName <<". Will search for same name different extension." << std::endl;
                const std::string textureNameOnly = textureFullFileName.substr(0, textureFullFileName.find_last_of("."));
                const AssetManager::AvailableAssetsNode *nameOnlyMatchAssets = assetManager->getAvailableAssetsTreeFiltered(AssetManager::Asset_type_TEXTURE, textureNameOnly);
                if (nameOnlyMatchAssets != nullptr) {
                    std::vector<std::string> possibleAssets;
                    getPossibleTexturesList(nameOnlyMatchAssets, possibleAssets);
                    if (!possibleAssets.empty()) {
                        for (size_t i = 0; i < possibleAssets.size(); ++i) {
                            cpuSurface = IMG_Load(possibleAssets[i].c_str());
                            if (cpuSurface) {
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    if (!cpuSurface) {
        std::cerr << "TextureAsset Load from disk failed for " << name[0] << " | " << name[1] << ". Error:" << std::endl << IMG_GetError()
                  << std::endl;
        exit(1);
    } else {
        //std::cout << "TextureAsset " << name[0] << " loaded from disk successfully." << std::endl;
    }
    if (cpuSurface->format->BytesPerPixel == 4) {
        if(cpuSurface->format->format != SDL_PIXELFORMAT_ABGR8888) {
            //if the internal format is not rgba32, convert to it.
            SDL_Surface* surfaceTemp = SDL_ConvertSurfaceFormat(cpuSurface,
                                                                SDL_PIXELFORMAT_ABGR8888,
                                                                0);
            SDL_FreeSurface(cpuSurface);
            cpuSurface = surfaceTemp;
        }
        textureMetaData.textureType = GraphicsInterface::TextureTypes::T2D;
        textureMetaData.internalFormatType = GraphicsInterface::InternalFormatTypes::RGBA;
        textureMetaData.formatType = GraphicsInterface::FormatTypes::RGBA;
        textureMetaData.dataType = GraphicsInterface::DataTypes::UNSIGNED_BYTE;
        textureMetaData.width = cpuSurface->w;
        textureMetaData.height = cpuSurface->h;

    } else if (cpuSurface->format->BytesPerPixel == 3) {
        if(cpuSurface->format->format != SDL_PIXELFORMAT_RGB24) {
            //if the internal format is not rgb24, convert to it.
            SDL_Surface* surfaceTemp = SDL_ConvertSurfaceFormat(cpuSurface,
                                                                SDL_PIXELFORMAT_RGB24,
                                                                0);
            SDL_FreeSurface(cpuSurface);
            cpuSurface = surfaceTemp;
        }
        textureMetaData.textureType = GraphicsInterface::TextureTypes::T2D;
        textureMetaData.internalFormatType = GraphicsInterface::InternalFormatTypes::RGB;
        textureMetaData.formatType = GraphicsInterface::FormatTypes::RGB;
        textureMetaData.dataType = GraphicsInterface::DataTypes::UNSIGNED_BYTE;
        textureMetaData.width = cpuSurface->w;
        textureMetaData.height = cpuSurface->h;
    } else if (cpuSurface->format->BytesPerPixel == 1) {
        SDL_Surface* surfaceTemp = SDL_ConvertSurfaceFormat(cpuSurface,
                                                            SDL_PIXELFORMAT_ABGR8888,
                                                            0);
        SDL_FreeSurface(cpuSurface);
        cpuSurface = surfaceTemp;

        textureMetaData.textureType = GraphicsInterface::TextureTypes::T2D;
        textureMetaData.internalFormatType = GraphicsInterface::InternalFormatTypes::RGBA;
        textureMetaData.formatType = GraphicsInterface::FormatTypes::RGBA;
        textureMetaData.dataType = GraphicsInterface::DataTypes::UNSIGNED_BYTE;
        textureMetaData.width = cpuSurface->w;
        textureMetaData.height = cpuSurface->h;
    } else {
        std::cerr << "Format has undefined number of pixels:" << std::to_string(cpuSurface->format->BytesPerPixel) << std::endl;
        exit(1);
    }
}

void TextureAsset::loadGPUPart() {
    texture = std::make_unique<Texture>(assetManager->getGraphicsWrapper(), textureMetaData.textureType,
                                        textureMetaData.internalFormatType, textureMetaData.formatType, textureMetaData.dataType,
                                        textureMetaData.width, textureMetaData.height);
    texture->loadData(cpuSurface->pixels);
    texture->setName(name[1]);
    SDL_FreeSurface(cpuSurface);
    cpuSurface = nullptr;
}

void TextureAsset::getPossibleTexturesList(const AssetManager::AvailableAssetsNode *assetsNode, std::vector<std::string> &possibleTexturesList) {
    if (assetsNode->assetType != AssetManager::Asset_type_DIRECTORY) {
        possibleTexturesList.emplace_back(assetsNode->fullPath);
    } else {
        for (size_t i = 0; i < assetsNode->children.size(); ++i) {
            getPossibleTexturesList(assetsNode->children[i], possibleTexturesList);
        }
    }
}

TextureAsset::~TextureAsset() {
    if (cpuSurface)
        SDL_FreeSurface(cpuSurface);
    //std::cout << "Texture asset deleted: " << name[0] << std::endl;
}
