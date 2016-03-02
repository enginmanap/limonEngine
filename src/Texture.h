//
// Created by Engin Manap on 1.03.2016.
//

#ifndef UBERGAME_TEXTURE_H
#define UBERGAME_TEXTURE_H

#include <string>
#include <iostream>
#include <SDL2/SDL_image.h>

#include "GLHelper.h"



class Texture{
protected:
    GLHelper* glHelper;
    std::string name;
    GLuint textureBufferID;

public:
    Texture(GLHelper*, std::string);

    ~Texture();

    GLuint getID() const {
        return textureBufferID;
    }
};

#endif //UBERGAME_TEXTURE_H
