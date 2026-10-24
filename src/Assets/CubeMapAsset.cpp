//
// Created by Engin Manap on 1.03.2016.
//

#include "CubeMapAsset.h"
#include <SDL3/SDL_surface.h>
#include "TextureAsset.h"


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
}

void CubeMapAsset::loadGPUPart() {
    SDL_Surface *surfaces[6] = {0};

    for (int i = 0; i < 6; i++) {
        surfaces[i] = TextureAsset::loadSurface(path + "/" + names[i]);
        if (!surfaces[i]) {
            std::cerr << "TextureAsset Load failed for " << path + "/" + names[i] << ". Error:" << std::endl <<
                      SDL_GetError() << std::endl;
            exit(-1);
        } else {
            //std::cout << "TextureAsset " << path + "/" + names[i] << " loaded succesfully." << std::endl;
        }

    }

    //color images, always RGBA for the same reason SRGBTextureAsset is, ES can't mipmap SRGB8
    for (int i = 0; i < 6; i++) {
        if (surfaces[i]->format != SDL_PIXELFORMAT_ABGR8888) {
            SDL_Surface *convertedSurface = SDL_ConvertSurface(surfaces[i], SDL_PIXELFORMAT_ABGR8888);
            if (convertedSurface == nullptr) {
                std::cerr << "CubeMap face " << path + "/" + names[i] << " can't be converted to RGBA. Error:" << std::endl << SDL_GetError() << std::endl;
                exit(-1);
            }
            SDL_DestroySurface(surfaces[i]);
            surfaces[i] = convertedSurface;
        }
    }

    texture = std::make_unique<Texture>(assetManager->getGraphicsWrapper(), GraphicsInterface::TextureTypes::TCUBE_MAP,
                                        GraphicsInterface::InternalFormatTypes::SRGB8_ALPHA8, GraphicsInterface::FormatTypes::RGBA, GraphicsInterface::DataTypes::UNSIGNED_BYTE,
                                        surfaces[0]->w, surfaces[0]->h);
    //FIXME check if all the maps has same height/width.
    texture->loadData(surfaces[0]->pixels, surfaces[1]->pixels,
                      surfaces[2]->pixels, surfaces[3]->pixels,
                      surfaces[4]->pixels, surfaces[5]->pixels);

    for (int i = 0; i < 6; i++) {
        SDL_DestroySurface(surfaces[i]);
    }
}