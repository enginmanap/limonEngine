//
// Created by engin on 19.06.2016.
//

#ifndef LIMONENGINE_MATERIAL_H
#define LIMONENGINE_MATERIAL_H

#include "glm/glm.hpp"
#include "Assets/TextureAsset.h"
#include "Assets/AssetManager.h"


class Material {
private:
    AssetManager *assetManager;
    std::string name;
    float specularExponent;

    glm::vec3 ambientColor, diffuseColor, specularColor;
    bool isAmbientMap = false;
    bool isDiffuseMap = false;
    bool isSpecularMap = false;
    bool isOpacityMap = false;
    float refractionIndex;

    TextureAsset *ambientTexture = nullptr, *diffuseTexture = nullptr, *specularTexture = nullptr, *opacityTexture = nullptr;

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
        this->isAmbientMap = true;
    }

    TextureAsset *getDiffuseTexture() const {
        if (!isDiffuseMap) {
            std::cerr << "access to nullptr element for " << this->name << std::endl;
            return nullptr;
        }
        return diffuseTexture;
    }

    void setDiffuseTexture(std::string diffuseTexture) {
        this->diffuseTexture = assetManager->loadAsset<TextureAsset>({diffuseTexture});
        this->isDiffuseMap = true;
    }

    TextureAsset *getSpecularTexture() const {
        return specularTexture;
    }

    void setSpecularTexture(std::string specularTexture) {
        this->specularTexture = assetManager->loadAsset<TextureAsset>({specularTexture});
        this->isSpecularMap = true;
    }


    void setOpacityTexture(std::string opacityTexture) {
        this->opacityTexture = assetManager->loadAsset<TextureAsset>({opacityTexture});
        this->isOpacityMap = true;
    }

    TextureAsset *getOpacityTexture() const {
        return opacityTexture;
    }

    ~Material() {
        if (ambientTexture != nullptr) {
            assetManager->freeAsset({ambientTexture->getName()});
        }
        if (diffuseTexture != nullptr) {
            assetManager->freeAsset({diffuseTexture->getName()});
        }
        if (specularTexture != nullptr) {
            assetManager->freeAsset({specularTexture->getName()});
        }
        if (opacityTexture != nullptr) {
            assetManager->freeAsset({opacityTexture->getName()});
        }
    }

    bool hasAmbientMap() const {
        return isAmbientMap;
    }

    bool hasDiffuseMap() const {
        return isDiffuseMap;
    }

    bool hasSpecularMap() const {
        return isSpecularMap;
    }

    bool hasOpacityMap() const {
        return isOpacityMap;
    }

};


#endif //LIMONENGINE_MATERIAL_H
