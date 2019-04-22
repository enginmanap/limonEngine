//
// Created by engin on 19.06.2016.
//

#include "Material.h"
#include "Graphics/GLHelper.h"

void Material::afterDeserialize(AssetManager *assetManager, std::string modelAssetFileName) {
    if(textureNameListList == nullptr || textureNameListList->size() != 5) {
        std::cerr << "Texture deserialize didn't set proper values. Please check for material " << name << ":" << materialIndex << std::endl;
    } else {
        this->assetManager = assetManager;
        if(!textureNameListList->at(0).empty()) {
            //file rename handling
            if(textureNameListList->at(0).size() == 2) {
                textureNameListList->at(0)[1] = modelAssetFileName;
            }
            this->ambientTexture = assetManager->loadAsset<TextureAsset>(textureNameListList->at(0));
        }
        if(!textureNameListList->at(1).empty()) {
            //file rename handling
            if(textureNameListList->at(1).size() == 2) {
                textureNameListList->at(1)[1] = modelAssetFileName;
            }
            this->diffuseTexture = assetManager->loadAsset<TextureAsset>(textureNameListList->at(1));
        }
        if(!textureNameListList->at(2).empty()) {
            //file rename handling
            if(textureNameListList->at(2).size() == 2) {
                textureNameListList->at(2)[1] = modelAssetFileName;
            }
            this->specularTexture = assetManager->loadAsset<TextureAsset>(textureNameListList->at(2));
        }
        if(!textureNameListList->at(3).empty()) {
            //file rename handling
            if(textureNameListList->at(3).size() == 2) {
                textureNameListList->at(3)[1] = modelAssetFileName;
            }
            this->normalTexture = assetManager->loadAsset<TextureAsset>(textureNameListList->at(3));
        }
        if(!textureNameListList->at(4).empty()) {
            //file rename handling
            if(textureNameListList->at(4).size() == 2) {
                textureNameListList->at(4)[1] = modelAssetFileName;
            }
            this->opacityTexture = assetManager->loadAsset<TextureAsset>(textureNameListList->at(4));
        }
    }
    textureNameListList.reset();

    this->materialIndex = assetManager->getGlHelper()->getNextMaterialIndex();
}