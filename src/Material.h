//
// Created by engin on 19.06.2016.
//

#ifndef LIMONENGINE_MATERIAL_H
#define LIMONENGINE_MATERIAL_H

#ifdef CEREAL_SUPPORT
#include <cereal/access.hpp>
#endif

#include <atomic>

#include "glm/glm.hpp"
#include "Assets/TextureAsset.h"
#include "Assets/AssetManager.h"

#include "Editor/ImGuiRequest.h"
#include "Editor/ImGuiResult.h"
#include "Editor/EditorRenderable.h"
#include "limonAPI/Graphics/GraphicsInterface.h"

class GraphicsProgram;

/*
 * Function local static, not a class level one: the graphics backends include this header and are built as
 * separate shared libraries, where a header static does not reliably resolve to a single instance.
 */
inline uint32_t nextRegistrationID() {
    static std::atomic<uint32_t> counter{1};//0 means unset
    return counter++;
}

class Material : public EditorRenderable {
private:
    std::string name;

    glm::vec3 ambientColor;
    glm::vec3 diffuseColor;
    glm::vec3 specularColor;

    /*
     * Identity. Assigned once at construction, never reused, never recomputed, so it is what the registry
     * files this material under. Editing changes the content hash, it can not change this.
     *
     * Not materialIndex: that is a slot in a fixed size GPU buffer, recycled, and only handed out once the
     * registry has decided this material is new.
     */
    uint32_t registrationID = nextRegistrationID();
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


    std::shared_ptr<std::vector<std::vector<std::string>>> textureNames = nullptr;

    friend struct std::hash<Material>;
    std::shared_ptr<TextureAsset> ambientTexture = nullptr;
    std::shared_ptr<TextureAsset> diffuseTexture = nullptr;
    std::shared_ptr<TextureAsset> specularTexture = nullptr;
    std::shared_ptr<TextureAsset> normalTexture = nullptr;
    std::shared_ptr<TextureAsset> opacityTexture = nullptr;
#ifdef CEREAL_SUPPORT
    friend class cereal::access;
#endif
    //registration is the only thing that assigns materialIndex, sets which base we override, or renames
    friend class MaterialRegistry;

    //name is not part of the content hash, so this never invalidates a registry key
    void setName(const std::string &name) {
        this->name = name;
    }
    Material() {};

    friend class WorldLoader;

    void setOriginalHash(size_t originalHash) {
        this->originalHash = originalHash;
    }

    //~Material frees every texture it holds, so a copy has to take its own reference for each one or it
    //releases what it never took. getName() is the full asset key, same one the destructor frees
    std::shared_ptr<TextureAsset> acquireCopiedTexture(const std::shared_ptr<TextureAsset> &sourceTexture) {
        if (sourceTexture == nullptr) {
            return nullptr;
        }
        return assetManager->partialLoadAssetAsync<TextureAsset>(sourceTexture->getName());
    }

    //acquire before free, or swapping between two materials that share a texture drops it to zero in between
    void replaceTexture(std::shared_ptr<TextureAsset> &currentTexture, const std::shared_ptr<TextureAsset> &newTexture) {
        if (currentTexture == newTexture) {
            return;
        }
        std::shared_ptr<TextureAsset> previousTexture = currentTexture;
        currentTexture = acquireCopiedTexture(newTexture);
        if (previousTexture != nullptr) {
            assetManager->freeAsset(previousTexture->getName());
        }
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

        //assetManager is assigned above, acquireCopiedTexture needs it
        this->ambientTexture = acquireCopiedTexture(other.ambientTexture);
        this->diffuseTexture = acquireCopiedTexture(other.diffuseTexture);
        this->specularTexture = acquireCopiedTexture(other.specularTexture);
        this->normalTexture = acquireCopiedTexture(other.normalTexture);
        this->opacityTexture = acquireCopiedTexture(other.opacityTexture);

        this->materialIndex = 0;
        this->originalHash = other.originalHash;
    }

    /*
     * No assignment. It would copy registrationID, giving two live materials the same identity, and it would
     * copy the texture shared_ptrs without acquiring AssetManager references while ~Material still frees
     * them - the same imbalance the copy constructor had to be fixed for. Copy construction is fine and used;
     * to overwrite an existing material's appearance use restoreValuesFrom, which leaves identity alone.
     */
    Material& operator=(const Material &other) = delete;

    //appearance only, name/materialIndex/originalHash stay put so we keep our registration and UBO slot
    void restoreValuesFrom(const Material &other) {
        this->ambientColor = other.ambientColor;
        this->diffuseColor = other.diffuseColor;
        this->specularColor = other.specularColor;
        this->specularExponent = other.specularExponent;
        this->refractionIndex = other.refractionIndex;

        this->isAmbientMap = other.isAmbientMap;
        this->isDiffuseMap = other.isDiffuseMap;
        this->isSpecularMap = other.isSpecularMap;
        this->isNormalMap = other.isNormalMap;
        this->isOpacityMap = other.isOpacityMap;
        this->maps = other.maps;

        replaceTexture(this->ambientTexture, other.ambientTexture);
        replaceTexture(this->diffuseTexture, other.diffuseTexture);
        replaceTexture(this->specularTexture, other.specularTexture);
        replaceTexture(this->normalTexture, other.normalTexture);
        replaceTexture(this->opacityTexture, other.opacityTexture);
    }

    void loadGPUSide(AssetManager *assetManager);

    // 5,6,7,8,9 would be used by material, so first assignable would be 10
    static constexpr int32_t MATERIAL_SAMPLER_TEXTURE_UNIT_START = GraphicsInterface::SHADOW_MAP_TEXTURE_UNIT_START + 2; // +2: directional, point
    static constexpr int32_t FIRST_ASSIGNABLE_TEXTURE_UNIT      = MATERIAL_SAMPLER_TEXTURE_UNIT_START + 5;              // +5: diffuse, ambient, specular, opacity, normal

    enum class Sampler : int32_t { DIFFUSE = 0, AMBIENT = 1, SPECULAR = 2, OPACITY = 3, NORMAL = 4 };
    static constexpr int32_t samplerUnit(Sampler sampler) {
        return MATERIAL_SAMPLER_TEXTURE_UNIT_START + static_cast<int32_t>(sampler);
    }

    static void configureProgram(const std::shared_ptr<GraphicsProgram>& program);

    void activateTextures(GraphicsInterface* graphicsWrapper) const;

    const std::string &getName() const {
        return name;
    }

    uint32_t getMaterialIndex() const {
        return materialIndex;
    }

    uint32_t getRegistrationID() const {
        return registrationID;
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

    //the base we globally override, 0 if none. Set once when a world overrides that base, or read
    //from XML, never recomputed from current content
    size_t getOriginalHash() const;


    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request) override;

    bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *materialsNode) const;
    static std::shared_ptr<Material> deserialize(AssetManager* assetManager, tinyxml2::XMLElement *materialNode);
#ifdef CEREAL_SUPPORT
    template<class Archive>
    void save(Archive & archive) const {
        std::string textureNameArray[10];

        std::vector<std::string> tempName;
        if(ambientTexture != nullptr) {
            tempName = ambientTexture->getName();

            textureNameArray[0] = tempName[0];
            if(tempName.size() == 2) {
                textureNameArray[1] = tempName[1];
            }
        }
        if(diffuseTexture != nullptr) {
            tempName = diffuseTexture->getName();

            textureNameArray[2] = tempName[0];
            if(tempName.size() == 2) {
                textureNameArray[3] = tempName[1];
            }
        }
        if(specularTexture != nullptr) {
            tempName = specularTexture->getName();

            textureNameArray[4] = tempName[0];
            if(tempName.size() == 2) {
                textureNameArray[5] = tempName[1];
            }
        };
        if(normalTexture != nullptr) {
            tempName = normalTexture->getName();

            textureNameArray[6] = tempName[0];
            if(tempName.size() == 2) {
                textureNameArray[7] = tempName[1];
            }
        };
        if(opacityTexture != nullptr) {
            tempName = opacityTexture->getName();

            textureNameArray[8] = tempName[0];
            if(tempName.size() == 2) {
                textureNameArray[9] = tempName[1];
            }
        };

        archive(name, specularExponent, maps, ambientColor, diffuseColor, specularColor, isAmbientMap, isDiffuseMap, isSpecularMap, isNormalMap, isOpacityMap, refractionIndex, originalHash,
            textureNameArray);
    }

    template<class Archive>
    void load(Archive & archive)  {

        std::string textureNameArray[10];

        archive(name, specularExponent, maps, ambientColor, diffuseColor, specularColor, isAmbientMap, isDiffuseMap, isSpecularMap, isNormalMap, isOpacityMap, refractionIndex, originalHash,
            textureNameArray);
        //now verify that we have any texture names we wanna process
        bool isTextureNamePresent = false;
        for (const std::string& textureName:textureNameArray) {
            if(!textureName.empty()) {
                isTextureNamePresent = true;
                break;
            }
        }
        if(isTextureNamePresent) {
            this->textureNames = std::make_shared<std::vector<std::vector<std::string>>>(5);
            for (int i = 0; i < 5; ++i) {
                (*this->textureNames).emplace_back();
            }
            if(!textureNameArray[0].empty()) {
                (*this->textureNames)[0].emplace_back(textureNameArray[0]);
                if(!textureNameArray[1].empty()) {
                    (*this->textureNames)[0].emplace_back(textureNameArray[1]);
                }
            }
            if(!textureNameArray[2].empty()) {
                (*this->textureNames)[1].emplace_back(textureNameArray[2]);
                if(!textureNameArray[3].empty()) {
                    (*this->textureNames)[1].emplace_back(textureNameArray[3]);
                }
            }
            if(!textureNameArray[4].empty()) {
                (*this->textureNames)[2].emplace_back(textureNameArray[4]);
                if(!textureNameArray[5].empty()) {
                    (*this->textureNames)[2].emplace_back(textureNameArray[5]);
                }
            }
            if(!textureNameArray[6].empty()) {
                (*this->textureNames)[3].emplace_back(textureNameArray[6]);
                if(!textureNameArray[7].empty()) {
                    (*this->textureNames)[3].emplace_back(textureNameArray[7]);
                }
            }
            if(!textureNameArray[8].empty()) {
                (*this->textureNames)[4].emplace_back(textureNameArray[8]);
                if(!textureNameArray[9].empty()) {
                    (*this->textureNames)[4].emplace_back(textureNameArray[9]);
                }
            }
        }

        if(this->name == "Mat_PolygonWestern_01_A") {
            this->ambientColor.x+=0.0001f;
        }
    }

    void afterLoad(AssetManager* assetManager) {
        this->assetManager = assetManager;
        if (textureNames != nullptr) {
            if (!(*textureNames)[0].empty()) {
                if ((*textureNames)[0].size() > 1) {
                    this->setAmbientTexture((*textureNames)[0][0], &(*textureNames)[0][1]);
                } else {
                    this->setAmbientTexture((*textureNames)[0][0]);
                }
            }
            if (!(*textureNames)[1].empty()) {
                if ((*textureNames)[1].size() > 1) {
                    this->setDiffuseTexture((*textureNames)[1][0], &(*textureNames)[1][1]);
                } else {
                    this->setDiffuseTexture((*textureNames)[1][0]);
                }
            }
            if (!(*textureNames)[2].empty()) {
                if ((*textureNames)[2].size() > 1) {
                    this->setSpecularTexture((*textureNames)[2][0], &(*textureNames)[2][1]);
                } else {
                    this->setSpecularTexture((*textureNames)[2][0]);
                }
            }
            if (!(*textureNames)[3].empty()) {
                if ((*textureNames)[3].size() > 1) {
                    this->setNormalTexture((*textureNames)[3][0], &(*textureNames)[3][1]);
                } else {
                    this->setNormalTexture((*textureNames)[3][0]);
                }
            }
            if (!(*textureNames)[4].empty()) {
                if ((*textureNames)[4].size() > 1) {
                    this->setOpacityTexture((*textureNames)[4][0], &(*textureNames)[4][1]);
                } else {
                    this->setOpacityTexture((*textureNames)[4][0]);
                }
            }
        }
        this->textureNames = nullptr;
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
