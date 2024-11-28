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
            const std::string textureFileName = name[1].substr(name[1].find_last_of("\\/") + 1);
            const std::string assetPath = name[0].substr(0, name[0].find_last_of("\\/"));

            cpuSurface = IMG_Load((assetPath + "/" + textureFileName).c_str());
            if (!cpuSurface) {
                cpuSurface = IMG_Load((assetPath + "/Textures/" + textureFileName).c_str());
            }
            if (!cpuSurface) {
                OptionsUtil::Options::Option<std::string> dataPath = assetManager->getGraphicsWrapper()->getOptions()->getOption<std::string>(HASH("dataDirectory"));
                std::string dataPathStr = dataPath.getOrDefault("./Data");
                if (dataPathStr.find_last_of("'/\\") == dataPathStr.size() - 1) {
                    dataPathStr = dataPathStr.substr(0, dataPathStr.size() - 1);//remove the last slash
                }

                size_t position = name[0].find("/", 0);
                position = name[0].find("/", position+1);
                position = name[0].find("/", position+1);
                const std::string assetInnerPath = name[0].substr(position+1, name[0].find_last_of("/\\") - (position + 1));

                std::string assetPathUnderData = assetPath.substr(assetPath.find_last_of("'/\\") + 1);//get the name of the asset folder under data folder
                cpuSurface = IMG_Load((dataPathStr + "/texture/" + assetInnerPath + "/" + textureFileName).c_str());
                if (!cpuSurface) {
                    std::cerr << "Texture asset load from file failed, tried the following paths:" << std::endl;
                    std::cerr << name[1] << std::endl;
                    std::cerr << assetPath + "/" + textureFileName << std::endl;
                    std::cerr << assetPath + "/Textures/" + textureFileName << std::endl;
                    std::cerr << assetPath + "/Textures/" + textureFileName << std::endl;
                    std::cerr << dataPathStr + "/textures/" + textureFileName << std::endl;
                }
            }
        }

    }

    if (!cpuSurface) {
        std::cerr << "TextureAsset Load from disk failed for " << name[1] << ". Error:" << std::endl << IMG_GetError()
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

TextureAsset::~TextureAsset() {
    //std::cout << "Texture asset deleted: " << name[0] << std::endl;
}
