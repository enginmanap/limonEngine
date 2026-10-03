//
// Created by engin on 19.06.2016.
//

#ifndef LIMONENGINE_MATERIAL_H
#define LIMONENGINE_MATERIAL_H

#ifdef CEREAL_SUPPORT
#include <cereal/access.hpp>
#endif

#include <array>
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

    struct TextureSlot {
        const char *displayName;
        const char *imGuiIDSuffix;
        const char *xmlElementName;
        std::shared_ptr<TextureAsset> Material::*texture;
        bool Material::*isMap;
        uint32_t mapBit;
    };
    static constexpr size_t TEXTURE_SLOT_COUNT = 5;
    //function local static for the same shared library reason as nextRegistrationID
    static const std::array<TextureSlot, TEXTURE_SLOT_COUNT> &getTextureSlots() {
        //never reorder: the cereal texture name layout and every saved OriginalHash follow this order
        static const std::array<TextureSlot, TEXTURE_SLOT_COUNT> textureSlots = {{
            {"Ambient",  "ambientTexture",  "AmbientTexture",  &Material::ambientTexture,  &Material::isAmbientMap,  8},
            {"Diffuse",  "diffuseTexture",  "DiffuseTexture",  &Material::diffuseTexture,  &Material::isDiffuseMap,  4},
            {"Specular", "specularTexture", "SpecularTexture", &Material::specularTexture, &Material::isSpecularMap, 2},
            {"Normal",   "normalTexture",   "NormalTexture",   &Material::normalTexture,   &Material::isNormalMap,   16},
            {"Opacity",  "opacityTexture",  "OpacityTexture",  &Material::opacityTexture,  &Material::isOpacityMap,  1},
        }};
        return textureSlots;
    }
    //for loading by name, doesn't touch maps or free what the slot held, same as the public setters
    void loadTextureIntoSlot(const TextureSlot &slot, const std::vector<std::string> &textureFiles) {
        this->*slot.texture = assetManager->partialLoadAssetAsync<TextureAsset>(textureFiles);
        this->*slot.isMap = true;
    }
    //acquiredTexture already holds its reference, nullptr empties the slot
    void assignTexture(const TextureSlot &slot, const std::shared_ptr<TextureAsset> &acquiredTexture);
    bool putRemoveTextureButton(const TextureSlot &slot);
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

        this->maps = other.maps;

        //assetManager is assigned above, acquireCopiedTexture needs it
        for (const TextureSlot &slot : getTextureSlots()) {
            this->*slot.isMap = other.*slot.isMap;
            this->*slot.texture = acquireCopiedTexture(other.*slot.texture);
        }

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

        this->maps = other.maps;

        for (const TextureSlot &slot : getTextureSlots()) {
            this->*slot.isMap = other.*slot.isMap;
            replaceTexture(this->*slot.texture, other.*slot.texture);
        }
    }

    void loadGPUSide(AssetManager *assetManager);

    // 5,6,7,8,9 are used by material, 3 and 4 stay assignable in material stages too
    static constexpr int32_t MATERIAL_SAMPLER_TEXTURE_UNIT_START = GraphicsInterface::FIRST_PIPELINE_TEXTURE_UNIT + 2;

    enum class Sampler : int32_t { DIFFUSE = 0, AMBIENT = 1, SPECULAR = 2, OPACITY = 3, NORMAL = 4 };
    static constexpr int32_t samplerUnit(Sampler sampler) {
        return MATERIAL_SAMPLER_TEXTURE_UNIT_START + static_cast<int32_t>(sampler);
    }
    static constexpr int32_t SAMPLER_COUNT = 5;
    static const char* samplerName(Sampler sampler) {
        switch (sampler) {
            case Sampler::DIFFUSE:  return "diffuseSampler";
            case Sampler::AMBIENT:  return "ambientSampler";
            case Sampler::SPECULAR: return "specularSampler";
            case Sampler::OPACITY:  return "opacitySampler";
            case Sampler::NORMAL:   return "normalSampler";
        }
        return "";
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
        for (const TextureSlot &slot : getTextureSlots()) {
            if (this->*slot.texture != nullptr) {
                assetManager->freeAsset({(this->*slot.texture)->getName()});
                this->*slot.texture = nullptr;
            }
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
        //per slot: texture file, then the model it is embedded in, if it is
        std::string textureNameArray[TEXTURE_SLOT_COUNT * 2];
        for (size_t slotIndex = 0; slotIndex < TEXTURE_SLOT_COUNT; ++slotIndex) {
            const std::shared_ptr<TextureAsset> &texture = this->*getTextureSlots()[slotIndex].texture;
            if (texture == nullptr) {
                continue;
            }
            const std::vector<std::string> &textureName = texture->getName();
            textureNameArray[slotIndex * 2] = textureName[0];
            if (textureName.size() == 2) {
                textureNameArray[slotIndex * 2 + 1] = textureName[1];
            }
        }

        archive(name, specularExponent, maps, ambientColor, diffuseColor, specularColor, isAmbientMap, isDiffuseMap, isSpecularMap, isNormalMap, isOpacityMap, refractionIndex, originalHash,
            textureNameArray);
    }

    template<class Archive>
    void load(Archive & archive)  {

        std::string textureNameArray[TEXTURE_SLOT_COUNT * 2];

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
            this->textureNames = std::make_shared<std::vector<std::vector<std::string>>>(TEXTURE_SLOT_COUNT);
            for (size_t slotIndex = 0; slotIndex < TEXTURE_SLOT_COUNT; ++slotIndex) {
                if (textureNameArray[slotIndex * 2].empty()) {
                    continue;
                }
                (*this->textureNames)[slotIndex].emplace_back(textureNameArray[slotIndex * 2]);
                if (!textureNameArray[slotIndex * 2 + 1].empty()) {
                    (*this->textureNames)[slotIndex].emplace_back(textureNameArray[slotIndex * 2 + 1]);
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
            for (size_t slotIndex = 0; slotIndex < TEXTURE_SLOT_COUNT; ++slotIndex) {
                if (!(*textureNames)[slotIndex].empty()) {
                    loadTextureIntoSlot(getTextureSlots()[slotIndex], (*textureNames)[slotIndex]);
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
            //a deserialized material is registered before afterLoad attaches its textures, so until then the names it
            //will attach stand in. Without them limonmodel materials differing only in textures merged into one
            for (size_t slotIndex = 0; slotIndex < Material::TEXTURE_SLOT_COUNT; ++slotIndex) {
                const std::shared_ptr<TextureAsset> &texture = m.*Material::getTextureSlots()[slotIndex].texture;
                if (texture != nullptr) {
                    hash_combine(hash, texture->getName());
                } else if (m.textureNames != nullptr && !(*m.textureNames)[slotIndex].empty()) {
                    hash_combine(hash, (*m.textureNames)[slotIndex]);
                }
            }
            return hash;
        }
    };
}


#endif //LIMONENGINE_MATERIAL_H
