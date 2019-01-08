//
// Created by engin on 19.06.2016.
//

#ifndef LIMONENGINE_MATERIAL_H
#define LIMONENGINE_MATERIAL_H

#ifdef CEREAL_SUPPORT
#include <cereal/access.hpp>
#endif

#include "glm/glm.hpp"
#include "Assets/TextureAsset.h"
#include "Assets/AssetManager.h"


class Material {
private:
    AssetManager *assetManager;
    std::string name;
    float specularExponent = 0;
    uint32_t materialIndex;
    uint32_t maps = 0;

    glm::vec3 ambientColor;
    glm::vec3 diffuseColor;
    glm::vec3 specularColor;
    bool isAmbientMap = false;
    bool isDiffuseMap = false;
    bool isSpecularMap = false;
    bool isNormalMap = false;
    bool isOpacityMap = false;
    float refractionIndex;

    std::unique_ptr<std::vector<std::vector<std::string>>> textureNameListList = nullptr;

    TextureAsset *ambientTexture = nullptr, *diffuseTexture = nullptr, *specularTexture = nullptr, *normalTexture = nullptr, *opacityTexture = nullptr;
#ifdef CEREAL_SUPPORT
    friend class cereal::access;
#endif
    friend class AssetManager;
    Material() {};

public:
    Material(AssetManager *assetManager, const std::string &name, uint32_t materialIndex, float specularExponent, const glm::vec3 &ambientColor,
             const glm::vec3 &diffuseColor, const glm::vec3 &specularColor, float refractionIndex)
            : assetManager(assetManager),
              name(name),
              specularExponent(specularExponent),
              materialIndex(materialIndex),
              ambientColor(ambientColor),
              diffuseColor(diffuseColor),
              specularColor(specularColor),
              refractionIndex(refractionIndex) { }


    Material(AssetManager *assetManager, const std::string &name, uint32_t materialIndex)
            : assetManager(assetManager),
              name(name),
              materialIndex(materialIndex),
              ambientColor(glm::vec3(0,0,0)),
              diffuseColor(glm::vec3(0,0,0)),
              specularColor(glm::vec3(0,0,0)) { }

    void afterDeserialize(AssetManager *assetManager, std::string modelAssetFileName);

    const std::string &getName() const {
        return name;
    }

    uint32_t getMaterialIndex() const {
        return materialIndex;
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

    void setAmbientTexture(const std::string &ambientTexture, std::string* sourceAsset = nullptr) {
        std::vector<std::string> textureFiles;
        textureFiles.push_back(ambientTexture);
        if(sourceAsset != nullptr) {
            textureFiles.push_back(*sourceAsset);
        }
        this->ambientTexture = assetManager->loadAsset<TextureAsset>(textureFiles);
        this->isAmbientMap = true;
    }

    TextureAsset *getDiffuseTexture() const {
        if (!isDiffuseMap) {
            std::cerr << "access to nullptr element for " << this->name << std::endl;
            return nullptr;
        }
        return diffuseTexture;
    }

    void setDiffuseTexture(const std::string &diffuseTexture, std::string* sourceAsset = nullptr) {
        std::vector<std::string> textureFiles;
        textureFiles.push_back(diffuseTexture);
        if(sourceAsset != nullptr) {
            textureFiles.push_back(*sourceAsset);
        }
        this->diffuseTexture = assetManager->loadAsset<TextureAsset>(textureFiles);
        this->isDiffuseMap = true;
    }

    TextureAsset *getSpecularTexture() const {
        return specularTexture;
    }

    void setSpecularTexture(const std::string &specularTexture, std::string* sourceAsset = nullptr) {
        std::vector<std::string> textureFiles;
        textureFiles.push_back(specularTexture);
        if(sourceAsset != nullptr) {
            textureFiles.push_back(*sourceAsset);
        }
        this->specularTexture = assetManager->loadAsset<TextureAsset>(textureFiles);
        this->isSpecularMap = true;
    }

    void setNormalTexture(const std::string &normalTexture, std::string* sourceAsset = nullptr) {
        std::vector<std::string> textureFiles;
        textureFiles.push_back(normalTexture);
        if(sourceAsset != nullptr) {
            textureFiles.push_back(*sourceAsset);
        }

        this->normalTexture = assetManager->loadAsset<TextureAsset>(textureFiles);
        this->isNormalMap = true;

    }

    TextureAsset *getNormalTexture() const {
        return normalTexture;
    }

    void setOpacityTexture(const std::string &opacityTexture, std::string* sourceAsset = nullptr) {
        std::vector<std::string> textureFiles;
        textureFiles.push_back(opacityTexture);
        if(sourceAsset != nullptr) {
            textureFiles.push_back(*sourceAsset);
        }

        this->opacityTexture = assetManager->loadAsset<TextureAsset>(textureFiles);
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
        if (normalTexture != nullptr) {
            assetManager->freeAsset({normalTexture->getName()});
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

    bool hasNormalMap() const {
        return isNormalMap;
    }

    bool hasOpacityMap() const {
        return isOpacityMap;
    }

    void setMaps(uint32_t maps) {
        this->maps = maps;
    }

    uint32_t getMaps() const {
        return maps;
    }
#ifdef CEREAL_SUPPORT
    template<class Archive>
    void save(Archive & archive) const {
        //AssetManager *assetManager;
        //TextureAsset *ambientTexture = nullptr, *diffuseTexture = nullptr, *specularTexture = nullptr, *normalTexture = nullptr, *opacityTexture = nullptr;

        std::vector<std::string> ambientTextureNames, diffuseTextureNames, specularTextureNames, normalTextureNames, opacityTextureNames;

        if(ambientTexture != nullptr) {
            ambientTextureNames = ambientTexture->getName();
        };
        if(diffuseTexture != nullptr) {
            diffuseTextureNames = diffuseTexture->getName();
        };
        if(specularTexture != nullptr) {
            specularTextureNames = specularTexture->getName();
        };
        if(normalTexture != nullptr) {
            normalTextureNames = normalTexture->getName();
        };
        if(opacityTexture != nullptr) {
            opacityTextureNames = opacityTexture->getName();
        };

        archive(name, specularExponent, maps, ambientColor, diffuseColor, specularColor, isAmbientMap, isDiffuseMap, isSpecularMap, isNormalMap, isOpacityMap, refractionIndex,
                ambientTextureNames, diffuseTextureNames, specularTextureNames, normalTextureNames, opacityTextureNames);
    }

    template<class Archive>
    void load(Archive & archive)  {
        //AssetManager *assetManager;
        //TextureAsset *ambientTexture = nullptr, *diffuseTexture = nullptr, *specularTexture = nullptr, *normalTexture = nullptr, *opacityTexture = nullptr;

        textureNameListList = std::make_unique<std::vector<std::vector<std::string>>>();
        for (int i = 0; i < 5; ++i) {
            textureNameListList->push_back(std::vector<std::string>());
        }

        std::vector<std::string> &ambientTextureNames  = textureNameListList->at(0);
        std::vector<std::string> &diffuseTextureNames  = textureNameListList->at(1);
        std::vector<std::string> &specularTextureNames = textureNameListList->at(2);
        std::vector<std::string> &normalTextureNames   = textureNameListList->at(3);
        std::vector<std::string> &opacityTextureNames  = textureNameListList->at(4);
        archive(name, specularExponent, maps, ambientColor, diffuseColor, specularColor, isAmbientMap, isDiffuseMap, isSpecularMap, isNormalMap, isOpacityMap, refractionIndex,
                ambientTextureNames, diffuseTextureNames, specularTextureNames, normalTextureNames, opacityTextureNames);
    }
#endif
};


#endif //LIMONENGINE_MATERIAL_H
