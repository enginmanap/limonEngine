//
// Created by Engin manap on 1.03.2016.
//

#include "TextureAsset.h"

TextureAsset::TextureAsset(AssetManager* assetManager, const std::vector<std::string> &files) :
        Asset(assetManager, files) {
    if (files.size() < 1) {
        std::cerr << "Texture load failed because file name vector is empty." << std::endl;
        exit(-1);
    }
    name = files[0];
    if (files.size() > 1) {
        std::cerr << "multiple files are sent to Texture constructor, extra elements ignored." << std::endl;
    }
    SDL_Surface *surface = IMG_Load(name.data());

    if (!surface) {
        std::cerr << "TextureAsset Load failed for " << name << ". Error:" << std::endl << IMG_GetError << std::endl;
        exit(1);
    } else {
        std::cout << "TextureAsset " << name << " loaded succesfully." << std::endl;
    }
    if (surface->format->BytesPerPixel == 4) {
        textureBufferID = assetManager->getGlHelper()->loadTexture(surface->h, surface->w, GL_RGBA, surface->pixels);
    } else if (surface->format->BytesPerPixel == 3) {
        textureBufferID = assetManager->getGlHelper()->loadTexture(surface->h, surface->w, GL_RGB, surface->pixels);
    } else {
        std::cerr << "Format has undefined number of pixels:" << surface->format->BytesPerPixel << std::endl;
        exit(1);
    }
    delete surface;
}

TextureAsset::~TextureAsset() {
    assetManager->getGlHelper()->deleteTexture(textureBufferID);
}