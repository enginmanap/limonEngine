//
// Created by Engin Manap on 1.03.2016.
//

#include "CubeMapAsset.h"


CubeMapAsset::CubeMapAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList) :
        Asset(assetManager, assetID, fileList) {
    if (fileList.size() < 7) {
        std::cerr << "CubeMap load failed because file name vector does not have 7 elements." << std::endl;
        exit(-1);
    }
    path = fileList[0];
    names[0] = fileList[1];
    names[1] = fileList[2];
    names[2] = fileList[3];
    names[3] = fileList[4];
    names[4] = fileList[5];
    names[5] = fileList[6];
    if (fileList.size() > 7) {
        std::cerr << "more than 7 files are sent to CubeMap constructor, extra elements ignored." << std::endl;
    }

    SDL_Surface *surfaces[6] = {0};

    for (int i = 0; i < 6; i++) {
        surfaces[i] = IMG_Load((path + "/" + names[i]).data());
        if (!surfaces[i]) {
            std::cerr << "TextureAsset Load failed for " << path + "/" + names[i] << ". Error:" << std::endl <<
                      IMG_GetError << std::endl;
            exit(-1);
        } else {
            //std::cout << "TextureAsset " << path + "/" + names[i] << " loaded succesfully." << std::endl;
        }

    }

    texture = std::make_unique<Texture>(assetManager->getGlHelper(), OpenGLGraphics::TextureTypes::TCUBE_MAP,
                                        OpenGLGraphics::InternalFormatTypes::RGB, OpenGLGraphics::FormatTypes::RGB, OpenGLGraphics::DataTypes::UNSIGNED_BYTE,
                                        surfaces[0]->w, surfaces[0]->h);
    //FIXME check if all the maps has same height/width, if they are RGB.
    texture->loadData(surfaces[0]->pixels, surfaces[1]->pixels,
                      surfaces[2]->pixels, surfaces[3]->pixels,
                      surfaces[4]->pixels, surfaces[5]->pixels);

    for (int i = 0; i < 6; i++) {
        delete surfaces[i];
    }

}