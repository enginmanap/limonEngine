//
// Created by Engin manap on 1.03.2016.
//

#include "Texture.h"

Texture::Texture(GLHelper* glHelper, std::string name):
    glHelper(glHelper),
    name(name){
    SDL_Surface* surface = IMG_Load(name.data());

    if(!surface){
        std::cerr << "Texture Load failed for " << name << ". Error:" << std::endl << IMG_GetError << std::endl;
    } else {
        std::cout << "Texture " << name << " loaded succesfully."<< std::endl;
    }
    if(surface->format->BytesPerPixel == 4){
        textureBufferID = glHelper->loadTexture(surface->h, surface->w, GL_RGBA, surface->pixels);
        std::cout << " Buffer id " << textureBufferID << std::endl;
    } else if(surface->format->BytesPerPixel == 3){
        textureBufferID = glHelper->loadTexture(surface->h, surface->w, GL_RGB, surface->pixels);
        std::cout << " Buffer id " << textureBufferID << std::endl;
    } else {
        std::cerr << "Format has undefined number of pixels:" << surface->format->BytesPerPixel << std::endl;
        exit(1);
    }
    delete surface;
}

Texture::~Texture() {
    glHelper->deleteTexture(textureBufferID);
}