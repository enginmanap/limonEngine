//
// Created by engin on 19.06.2016.
//

#ifndef UBERGAME_MATERIAL_H
#define UBERGAME_MATERIAL_H

#include "glm/glm.hpp"
#include "Assets/TextureAsset.h"
#include "Assets/AssetManager.h"


class Material {
private:
    AssetManager *assetManager;
    std::string name;
    float specularExponent;

    glm::vec3 ambientColor, diffuseColor, specularColor;
    float refractionIndex;

    TextureAsset *ambientTexture = NULL, *diffuseTexture = NULL, *specularTexture = NULL;

public:
    Material(AssetManager *assetManager, const std::string &name, float specularExponent, const glm::vec3 &ambientColor,
             const glm::vec3 &diffuseColor, const glm::vec3 &specularColor, float refractionIndex)
            : assetManager(assetManager),
              name(name),
              specularExponent(specularExponent),
              ambientColor(ambientColor),
              diffuseColor(diffuseColor),
              specularColor(specularColor),
              refractionIndex(refractionIndex) { }


    Material(AssetManager *assetManager, const std::string &name)
            : assetManager(assetManager),
              name(name) { }

    const std::string &getName() const {
        return name;
    }

    float getSpecularExponent() const {
        return specularExponent;
    }

    void setSpecularExponent(float specularExponent) {
        this->specularExponent = specularExponent;
    }

    const glm::vec3 &getAmbientColor() const {
        return ambientColor;
    }

    void setAmbientColor(const glm::vec3 &ambientColor) {
        this->ambientColor = ambientColor;
    }

    const glm::vec3 &getDiffuseColor() const {
        return diffuseColor;
    }

    void setDiffuseColor(const glm::vec3 &diffuseColor) {
        this->diffuseColor = diffuseColor;
    }

    const glm::vec3 &getSpecularColor() const {
        return specularColor;
    }

    void setSpecularColor(const glm::vec3 &specularColor) {
        this->specularColor = specularColor;
    }

    float getRefractionIndex() const {
        return refractionIndex;
    }

    void setRefractionIndex(float refractionIndex) {
        this->refractionIndex = refractionIndex;
    }

    TextureAsset *getAmbientTexture() const {
        return ambientTexture;
    }

    void setAmbientTexture(std::string ambientTexture) {

        this->ambientTexture = assetManager->loadAsset<TextureAsset>({ambientTexture});
    }

    TextureAsset *getDiffuseTexture() const {
//        if (diffuseTexture == NULL) {
//            std::cerr << "access to null element" << std::endl;
//        }
        return diffuseTexture;
    }

    void setDiffuseTexture(std::string diffuseTexture) {
        this->diffuseTexture = assetManager->loadAsset<TextureAsset>({diffuseTexture});
    }

    TextureAsset *getSpecularTexture() const {
        return specularTexture;
    }

    void setSpecularTexture(std::string specularTexture) {
        this->specularTexture = assetManager->loadAsset<TextureAsset>({specularTexture});
    }

    ~Material() {
        if (ambientTexture != NULL) {
            delete ambientTexture;
        }
        if (diffuseTexture != NULL) {
            delete diffuseTexture;
        }
        if (specularTexture != NULL) {
            delete specularTexture;
        }
    }
};


#endif //UBERGAME_MATERIAL_H
