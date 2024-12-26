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

#include "Editor/ImGuiRequest.h"
#include "Editor/ImGuiResult.h"


class Material {
private:
    std::string name;

    glm::vec3 ambientColor;
    glm::vec3 diffuseColor;
    glm::vec3 specularColor;

    uint32_t materialIndex;
    uint32_t maps = 0;
    AssetManager *assetManager;
    size_t originalHash = 0;//With this, we can still match assets loaded after the material is changed
    float specularExponent = 0;
    float refractionIndex = 0;
    bool deserialized = false;
    bool isAmbientMap = false;
    bool isDiffuseMap = false;
    bool isSpecularMap = false;
    bool isNormalMap = false;
    bool isOpacityMap = false;

    /**
     * This is a list of texture names.
     * items:
     * 0 -> ambient
     * 1 -> diffuse
     * 2 -> specular
     * 3 -> normal
     * 4 -> opacity
     *
     * vector has vector<string>, because embedded textures are saved with pairs, first element is the index, second element is the model file itself.
     */
    friend struct std::hash<Material>;
    std::shared_ptr<TextureAsset> ambientTexture = nullptr;
    std::shared_ptr<TextureAsset> diffuseTexture = nullptr;
    std::shared_ptr<TextureAsset> specularTexture = nullptr;
    std::shared_ptr<TextureAsset> normalTexture = nullptr;
    std::shared_ptr<TextureAsset> opacityTexture = nullptr;
#ifdef CEREAL_SUPPORT
    friend class cereal::access;
#endif
    friend class AssetManager;
    Material() {};

    friend class WorldLoader;

    void setOriginalHash(size_t originalHash) {
        this->originalHash = originalHash;
    }
public:
    Material(AssetManager *assetManager, const std::string &name, uint32_t materialIndex, float specularExponent, const glm::vec3 &ambientColor,
             const glm::vec3 &diffuseColor, const glm::vec3 &specularColor, float refractionIndex)//FIXME: this should not use raw pointer
            : name(name),
              ambientColor(ambientColor),
              diffuseColor(diffuseColor),
              specularColor(specularColor),
              materialIndex(materialIndex),
              assetManager(assetManager),
              specularExponent(specularExponent),
              refractionIndex(refractionIndex) { }


    Material(AssetManager *assetManager, const std::string &name, uint32_t materialIndex)
            : name(name),
              ambientColor(glm::vec3(0,0,0)),
              diffuseColor(glm::vec3(0,0,0)),
              specularColor(glm::vec3(0,0,0)),
              materialIndex(materialIndex),
              assetManager(assetManager) { }

    Material(const Material &other) {
        this->name = "copy_"+other.name;

        this->ambientColor = other.ambientColor;
        this->diffuseColor = other.diffuseColor;
        this->specularColor = other.specularColor;

        this->assetManager = other.assetManager;

        this->specularExponent = other.specularExponent;
        this->refractionIndex = other.refractionIndex;

        this->isAmbientMap = other.isAmbientMap;
        this->isDiffuseMap = other.isDiffuseMap;
        this->isSpecularMap = other.isSpecularMap;
        this->isNormalMap = other.isNormalMap;
        this->isOpacityMap = other.isOpacityMap;
        this->maps = other.maps;

        this->ambientTexture = other.ambientTexture;
        this->diffuseTexture = other.diffuseTexture;
        this->specularTexture = other.specularTexture;
        this->normalTexture = other.normalTexture;
        this->opacityTexture = other.opacityTexture;

        this->materialIndex = 0;
        this->originalHash = other.originalHash;
    }

    void loadGPUSide(AssetManager *assetManager);

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

    std::shared_ptr<TextureAsset> getAmbientTexture() const {
        return ambientTexture;
    }

    void setAmbientTexture(const std::string &ambientTexture, std::string* sourceAsset = nullptr) {
        std::vector<std::string> textureFiles;
        textureFiles.push_back(ambientTexture);
        if(sourceAsset != nullptr) {
            textureFiles.push_back(*sourceAsset);
        }
        this->ambientTexture = assetManager->partialLoadAssetAsync<TextureAsset>(textureFiles);
        this->isAmbientMap = true;
    }

    std::shared_ptr<TextureAsset> getDiffuseTexture() const {
        return diffuseTexture;
    }

    void setDiffuseTexture(const std::string &diffuseTexture, std::string* sourceAsset = nullptr) {
        std::vector<std::string> textureFiles;
        textureFiles.push_back(diffuseTexture);
        if(sourceAsset != nullptr) {
            textureFiles.push_back(*sourceAsset);
        }
        this->diffuseTexture = assetManager->partialLoadAssetAsync<TextureAsset>(textureFiles);
        this->isDiffuseMap = true;
    }

    std::shared_ptr<TextureAsset>getSpecularTexture() const {
        return specularTexture;
    }

    void setSpecularTexture(const std::string &specularTexture, std::string* sourceAsset = nullptr) {
        std::vector<std::string> textureFiles;
        textureFiles.push_back(specularTexture);
        if(sourceAsset != nullptr) {
            textureFiles.push_back(*sourceAsset);
        }
        this->specularTexture = assetManager->partialLoadAssetAsync<TextureAsset>(textureFiles);
        this->isSpecularMap = true;
    }

    void setNormalTexture(const std::string &normalTexture, std::string* sourceAsset = nullptr) {
        std::vector<std::string> textureFiles;
        textureFiles.push_back(normalTexture);
        if(sourceAsset != nullptr) {
            textureFiles.push_back(*sourceAsset);
        }
        this->normalTexture = assetManager->partialLoadAssetAsync<TextureAsset>(textureFiles);
        this->isNormalMap = true;

    }

    std::shared_ptr<TextureAsset> getNormalTexture() const {
        return normalTexture;
    }

    void setOpacityTexture(const std::string &opacityTexture, std::string* sourceAsset = nullptr) {
        std::vector<std::string> textureFiles;
        textureFiles.push_back(opacityTexture);
        if(sourceAsset != nullptr) {
            textureFiles.push_back(*sourceAsset);
        }
        this->opacityTexture = assetManager->partialLoadAssetAsync<TextureAsset>(textureFiles);
        this->isOpacityMap = true;
    }

    std::shared_ptr<TextureAsset> getOpacityTexture() const {
        return opacityTexture;
    }

    ~Material() {
        //std::cerr << "Destructor for " << name << std::endl;

        /**
         * Why are we setting the textures to nullptr here?
         * Because otherwise Asset manager detects the texture has a
         * shared_ptr to it and logs an error.
         */
        if (ambientTexture != nullptr) {
            assetManager->freeAsset({ambientTexture->getName()});
            this->ambientTexture = nullptr;
        }
        if (diffuseTexture != nullptr) {
            assetManager->freeAsset({diffuseTexture->getName()});
            this->diffuseTexture = nullptr;
        }
        if (specularTexture != nullptr) {
            assetManager->freeAsset({specularTexture->getName()});
            this->specularTexture = nullptr;
        }
        if (opacityTexture != nullptr) {
            assetManager->freeAsset({opacityTexture->getName()});
            this->opacityTexture = nullptr;
        }
        if (normalTexture != nullptr) {
            assetManager->freeAsset({normalTexture->getName()});
            this->normalTexture = nullptr;
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

    size_t getHash() const;
    size_t getOriginalHash() const;
    void calculateOriginalHash() {
        this->originalHash = getHash();
    }


    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request);

    bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *materialsNode) const;
    static std::shared_ptr<Material> deserialize(AssetManager* assetManager, tinyxml2::XMLElement *materialNode);
#ifdef CEREAL_SUPPORT
    template<class Archive>
    void save(Archive & archive) const {

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
    std::cerr << "LimonModel Material load needs reimplementing" << std::endl;
    getchar();
    /*
     * Old implementation below
     *
        textureNameListList = std::make_unique<std::vector<std::vector<std::string>>>();
        for (int i = 0; i < 5; ++i) {
            textureNameListList->push_back(std::vector<std::string>());
        }

        std::vector<std::string> &ambientTextureNames  = textureNameListList->at(0);
        std::vector<std::string> &diffuseTextureNames  = textureNameListList->at(1);
        std::vector<std::string> &specularTextureNames = textureNameListList->at(2);
        std::vector<std::string> &normalTextureNames   = textureNameListList->at(3);
        std::vector<std::string> &opacityTextureNames  = textureNameListList->at(4);
    */
        archive(name, specularExponent, maps, ambientColor, diffuseColor, specularColor, isAmbientMap, isDiffuseMap, isSpecularMap, isNormalMap, isOpacityMap, refractionIndex
                //,ambientTextureNames, diffuseTextureNames, specularTextureNames, normalTextureNames, opacityTextureNames);
        );
    }
#endif
};

namespace std {

    inline void hash_combine(std::size_t& seed [[gnu::unused]]) { }

    template <typename T, typename... Rest>
    inline void hash_combine(std::size_t& seed, const T& v, Rest... rest) {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        hash_combine(seed, rest...);
    }

    template <>
    struct hash<glm::vec3> {
        size_t operator()(const glm::vec3& v) const {
            size_t hash = 0;
            hash_combine(hash, v.x, v.y, v.z);
            return hash;
        }
    };

    template <>
    struct hash<std::vector<std::string>> {
        size_t operator()(const std::vector<std::string>& vs) const {
            size_t hash = 0;
            for (size_t i = 0; i < vs.size(); ++i) {
                hash_combine(hash, vs.at(i));
            }
            return hash;
        }
    };

    template <>
    struct hash<Material> {
        size_t operator()(const Material& m) const {
            size_t hash = 0;
            hash_combine(hash,
                                m.getSpecularExponent(),
                                m.getMaps(),
                                m.getAmbientColor(),
                                m.getDiffuseColor(),
                                m.getSpecularColor(),
                                m.getRefractionIndex()
                                );
            //std::cout << "for material " << m.getName() << " hash is calculated as " << hash << std::endl;
            //now check the texture info
            if(m.getAmbientTexture() != nullptr) {
                hash_combine(hash, m.getAmbientTexture()->getName());
            }
            if(m.getDiffuseTexture() != nullptr) {
                hash_combine(hash, m.getDiffuseTexture()->getName());
            }
            if(m.getSpecularTexture() != nullptr) {
                hash_combine(hash, m.getSpecularTexture()->getName());
            }
            if(m.getNormalTexture() != nullptr) {
                hash_combine(hash, m.getNormalTexture()->getName());
            }
            if(m.getOpacityTexture() != nullptr) {
                hash_combine(hash, m.getOpacityTexture()->getName());
            }
            return hash;
        }
    };
}


#endif //LIMONENGINE_MATERIAL_H
