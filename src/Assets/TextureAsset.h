//
// Created by Engin Manap on 1.03.2016.
//

#ifndef UBERGAME_TEXTURE_H
#define UBERGAME_TEXTURE_H

#include <string>
#include <iostream>
#include <SDL2/SDL_image.h>

#include "../GLHelper.h"


class TextureAsset {
protected:
    GLHelper *glHelper;
    std::string name;
    GLuint textureBufferID;

public:
    TextureAsset(GLHelper *, std::string);

    ~TextureAsset();

    GLuint getID() const {
        return textureBufferID;
    }
};

#endif //UBERGAME_TEXTURE_H
