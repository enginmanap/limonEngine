//
// Created by Engin manap on 1.03.2016.
//

#include "TextureAsset.h"
#include "../GLHelper.h"

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

    SDL_Surface *surface = nullptr;
    if (files.size() == 2) {//If embedded texture is needed, first element is the index, second is the owner asset file
        //index is a string, first char is * second char is the index
        std::cout << "Texture request has 2 elements. Attempting to extract embedded texture. " << std::endl;
        int textureID = std::atoi(&files[0][1]);
        const AssetManager::EmbeddedTexture* embeddedTexture = assetManager->getEmbeddedTextures(files[1], textureID);
        if(embeddedTexture != nullptr) {
            SDL_RWops* rwop = nullptr;
            if(embeddedTexture->height == 0) {
                rwop = SDL_RWFromMem( embeddedTexture->texelData, embeddedTexture->width);
            } else {
                rwop = SDL_RWFromMem(embeddedTexture->texelData, embeddedTexture->width * embeddedTexture->height);
            }
            surface = IMG_Load_RW(rwop, 0);
        }
    } else {
        /**
         * FIXME: This takes full path, which is not acceptable,
         * we need to work with relative path to model for textures
         */
        surface = IMG_Load(name[0].data());
    }





    if (!surface) {
        std::cerr << "TextureAsset Load from disk failed for " << name[0] << ". Error:" << std::endl << IMG_GetError()
                  << std::endl;
        exit(1);
    } else {
        //std::cout << "TextureAsset " << name[0] << " loaded from disk successfully." << std::endl;
    }
    if (surface->format->BytesPerPixel == 4) {
        if(surface->format->format != SDL_PIXELFORMAT_ABGR8888) {
            //if the internal format is not rgba32, convert to it.
            SDL_Surface* surfaceTemp = SDL_ConvertSurfaceFormat(surface,
                                                                SDL_PIXELFORMAT_ABGR8888,
                                                                0);
            delete surface;
            surface = surfaceTemp;
        }
        textureBufferID = assetManager->getGlHelper()->loadTexture(surface->h, surface->w, GL_RGBA, surface->pixels);
    } else if (surface->format->BytesPerPixel == 3) {
        if(surface->format->format != SDL_PIXELFORMAT_RGB24) {
            //if the internal format is not rgb24, convert to it.
            SDL_Surface* surfaceTemp = SDL_ConvertSurfaceFormat(surface,
                                                                SDL_PIXELFORMAT_RGB24,
                                                                0);
            delete surface;
            surface = surfaceTemp;
        }
        textureBufferID = assetManager->getGlHelper()->loadTexture(surface->h, surface->w, GL_RGB, surface->pixels);
    } else {
        std::cerr << "Format has undefined number of pixels:" << surface->format->BytesPerPixel << std::endl;
        exit(1);
    }

    this->height = surface->h;
    this->width = surface->w;

    delete surface;
}

TextureAsset::~TextureAsset() {
    assetManager->getGlHelper()->deleteTexture(textureBufferID);
    //std::cout << "Texture asset deleted: " << name[0] << std::endl;
}