//
// Created by Engin manap on 1.03.2016.
//

#include "TextureAsset.h"

TextureAsset::TextureAsset(AssetManager* assetManager, const std::vector<std::string> &files) :
        Asset(assetManager, files) {
    if (files.empty()) {
        std::cerr << "Texture load failed because file name vector is empty." << std::endl;
        exit(-1);
    }
    name = files[0];
    if (files.size() > 1) {
        std::cerr << "multiple files are sent to Texture constructor, extra elements ignored." << std::endl;
    }
    /**
     * FIXME: This takes full path, which is not acceptable,
     * we need to work with relative path to model for textures
     */
    SDL_Surface *surface = IMG_Load(name.data());

    if (!surface) {
        std::cerr << "TextureAsset Load from disk failed for " << name << ". Error:" << std::endl << IMG_GetError()
                  << std::endl;
        exit(1);
    } else {
        std::cout << "TextureAsset " << name << " loaded from disk successfully." << std::endl;
    }
    if (surface->format->BytesPerPixel == 4) {
        if(surface->format->format != SDL_PIXELFORMAT_RGBA32) {
            //if the internal format is not rgba32, convert to it.
            SDL_Surface* surfaceTemp = SDL_ConvertSurfaceFormat(surface,
                                                            SDL_PIXELFORMAT_RGBA32,
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
    delete surface;
}

TextureAsset::~TextureAsset() {
    assetManager->getGlHelper()->deleteTexture(textureBufferID);
    std::cout << "Texture asset deleted: " << name << std::endl;
}