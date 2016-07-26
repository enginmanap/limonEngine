//
// Created by Engin manap on 1.03.2016.
//

#include "TextureAsset.h"

TextureAsset::TextureAsset(GLHelper *glHelper, std::string name) :
        glHelper(glHelper),
        name(name) {
    SDL_Surface *surface = IMG_Load(name.data());

    if (!surface) {
        std::cerr << "TextureAsset Load failed for " << name << ". Error:" << std::endl << IMG_GetError << std::endl;
        exit(1);
    } else {
        std::cout << "TextureAsset " << name << " loaded succesfully." << std::endl;
    }
    if (surface->format->BytesPerPixel == 4) {
        textureBufferID = glHelper->loadTexture(surface->h, surface->w, GL_RGBA, surface->pixels);
    } else if (surface->format->BytesPerPixel == 3) {
        textureBufferID = glHelper->loadTexture(surface->h, surface->w, GL_RGB, surface->pixels);
    } else {
        std::cerr << "Format has undefined number of pixels:" << surface->format->BytesPerPixel << std::endl;
        exit(1);
    }
    delete surface;
}

TextureAsset::~TextureAsset() {
    glHelper->deleteTexture(textureBufferID);
}