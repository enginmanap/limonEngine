//
// Created by Engin manap on 1.03.2016.
//

#include "TextureAsset.h"

#include <SDL_image.h>
#include <GameObjects/Players/FreeCursorPlayer.h>

TextureAsset::TextureAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &files) :
        Asset(assetManager, assetID, files) {
    if (files.empty()) {
        std::cerr << "Texture load failed because file name vector is empty." << std::endl;
        exit(-1);
    }
    name = files;
    if (files.size() > 2) {
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
 * program loads a fallback texture.
 */

void TextureAsset::loadCPUPart() {
    std::string originalErrorMessage;

    if (name.size() == 2) {//If embedded texture is needed, first element is the index, second is the owner asset file
        //index is a string, first char is * second char is the index
        std::cout << "Texture request has 2 elements. Attempting to extract embedded texture. " << std::endl;
        int textureID = std::atoi(&name[0][1]);
        std::shared_ptr<const AssetManager::EmbeddedTexture> embeddedTexture = assetManager->getEmbeddedTextures(name[1], textureID);
        if(embeddedTexture != nullptr) {
            SDL_RWops* rwop = nullptr;
            if(embeddedTexture->height == 0) {
                rwop = SDL_RWFromMem( (void*)embeddedTexture->texelData.data(), embeddedTexture->width);
            } else {
                rwop = SDL_RWFromMem((void*)embeddedTexture->texelData.data(), embeddedTexture->width * embeddedTexture->height);
            }
            cpuSurface = IMG_Load_RW(rwop, 0);
        } else {
            std::cerr << "Embedded texture can't be found with following information: " << name[1] << ":" << textureID << std::endl;
        }
    } else {
        cpuSurface = IMG_Load(name[0].data());
        if(!cpuSurface) {
            originalErrorMessage = IMG_GetError();
            const std::string textureFullFileName = name[0].substr(name[0].find_last_of("\\/") + 1);
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
                const std::string textureNameOnly = textureFullFileName.substr(0, textureFullFileName.find_last_of('.'));
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
        std::cerr << "TextureAsset Load from disk failed for " << name[0] <<  ". Error:" << std::endl << originalErrorMessage << std::endl;
        std::cerr << "Loading \"not found fallback\" texture" << std::endl;
        cpuSurface = IMG_Load(FALLBACK_TEXTURE_NAME.c_str());
        if (!cpuSurface) {
            std::cerr << "Loading  fallback texture failed. Please make sure " << FALLBACK_TEXTURE_NAME << " exists and valid. Exiting." << std::endl;
            exit(1);
        }
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
    if (name.size() > 1) {
        texture->setName(name[0] + "|" + name[1]);
    } else {
        texture->setName(name[0]);
    }
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
    std::cout << "Texture asset deleted: " << name[0] << std::endl;
}
