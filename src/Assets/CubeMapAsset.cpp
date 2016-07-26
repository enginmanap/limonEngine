//
// Created by Engin Manap on 1.03.2016.
//

#include "CubeMapAsset.h"


CubeMapAsset::CubeMapAsset(GLHelper *glHelper, std::string path,
                           std::string right, std::string left,
                           std::string top, std::string bottom,
                           std::string back, std::string front) :
        glHelper(glHelper),
        path(path) {
    names[0] = right;
    names[1] = left;
    names[2] = top;
    names[3] = bottom;
    names[4] = back;
    names[5] = front;
    SDL_Surface *surfaces[6] = {0};

    for (int i = 0; i < 6; i++) {
        surfaces[i] = IMG_Load((path + "/" + names[i]).data());
        if (!surfaces[i]) {
            std::cerr << "TextureAsset Load failed for " << path + "/" + names[i] << ". Error:" << std::endl <<
                      IMG_GetError << std::endl;
            exit(-1);
        } else {
            std::cout << "TextureAsset " << path + "/" + names[i] << " loaded succesfully." << std::endl;
        }

    }
    //check if all the maps has same height/width
    cubeMapBufferID = glHelper->loadCubeMap(surfaces[0]->h, surfaces[0]->w,
                                            surfaces[0]->pixels, surfaces[1]->pixels,
                                            surfaces[2]->pixels, surfaces[3]->pixels,
                                            surfaces[4]->pixels, surfaces[5]->pixels);
    for (int i = 0; i < 6; i++) {
        delete surfaces[i];
    }

}