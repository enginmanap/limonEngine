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
#include <cmath>
#include <mutex>
#include <set>
#include <unordered_map>

#include "glm/glm.hpp"
#include "Assets/TextureAsset.h"
#include "Assets/SRGBTextureAsset.h"
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
    mutable std::mutex nameMutex;// When a material tries to do a register, our dedup renames the material, based on what is alphabetically the first. For rename, we need a mutex

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
    uint32_t materialIndex = 0;//cereal builds through Material(), isLoaded reads garbage otherwise
    uint32_t maps = 0;
    AssetManager *assetManager;
    uint64_t originalHash = 0;//With this, we can still match assets loaded after the material is changed
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
     *
     * Never cleared, the packer thread reads these while the main thread sets and clears the texture pointers below.
     */


    std::shared_ptr<std::vector<std::vector<std::string>>> textureNames = nullptr;

    //null unless loaded
    std::shared_ptr<TextureAsset> ambientTexture = nullptr;
    std::shared_ptr<TextureAsset> diffuseTexture = nullptr;
    std::shared_ptr<TextureAsset> specularTexture = nullptr;
    std::shared_ptr<TextureAsset> normalTexture = nullptr;
    std::shared_ptr<TextureAsset> opacityTexture = nullptr;

    //only the slots load repaired, printed when the texture is used
    std::unique_ptr<std::array<std::string, 5>> requestedTextureNames;
#ifdef CEREAL_SUPPORT
    friend class cereal::access;
#endif
    //registration is the only thing that assigns materialIndex, sets which base we override, or renames
    friend class MaterialRegistry;

    //name is not part of the content hash, so this never invalidates a registry key
    void setName(const std::string &name) {
        std::lock_guard<std::mutex> lock(nameMutex);
        this->name = name;
    }
    Material() {};

    friend class WorldLoader;

    void setOriginalHash(uint64_t originalHash) {
        this->originalHash = originalHash;
    }

    struct TextureSlot {
        const char *displayName;
        const char *imGuiIDSuffix;
        const char *xmlElementName;
        std::shared_ptr<TextureAsset> Material::*texture;
        bool Material::*isMap;
        uint32_t mapBit;
        bool isColor;//color slots load as SRGBTextureAsset, the rest as TextureAsset
    };
    static constexpr size_t TEXTURE_SLOT_COUNT = 5;
    //function local static for the same shared library reason as nextRegistrationID
    static const std::array<TextureSlot, TEXTURE_SLOT_COUNT> &getTextureSlots() {
        //never reorder: the cereal texture name layout and every saved OriginalHash follow this order
        static const std::array<TextureSlot, TEXTURE_SLOT_COUNT> textureSlots = {{
            {"Ambient",  "ambientTexture",  "AmbientTexture",  &Material::ambientTexture,  &Material::isAmbientMap,  8,  true},
            {"Diffuse",  "diffuseTexture",  "DiffuseTexture",  &Material::diffuseTexture,  &Material::isDiffuseMap,  4,  true},
            {"Specular", "specularTexture", "SpecularTexture", &Material::specularTexture, &Material::isSpecularMap, 2,  true},
            {"Normal",   "normalTexture",   "NormalTexture",   &Material::normalTexture,   &Material::isNormalMap,   16, false},
            {"Opacity",  "opacityTexture",  "OpacityTexture",  &Material::opacityTexture,  &Material::isOpacityMap,  1,  false},
        }};
        return textureSlots;
    }
    //records the file only, doesn't touch maps, same as the public setters
    void setTextureName(const TextureSlot &slot, const std::vector<std::string> &textureFiles);

    //prints nothing, attachTexture says it if the texture is ever used
    std::string resolveTexturePath(const std::string &requestedPath, bool &repaired) const;

    std::string findTextureByName(const std::string &fileName) const;
    //takes a reference, as the asset type of the slot. A file is one asset whichever type asked first, so every request for a slot's file goes through here
    std::shared_ptr<TextureAsset> loadSlotTexture(const TextureSlot &slot, const std::vector<std::string> &textureFiles) const;

    [[noreturn]] void failTextureType(const TextureSlot &slot, const std::string &textureFile) const;

    //the slots of one batch all load as TextureType, color slots and linear slots are separate batches
    template<class TextureType>
    static void loadSlotBatch(AssetManager *assetManager, const std::vector<std::pair<Material *, size_t>> &slotsToLoad,
                              const std::vector<std::vector<std::string>> &filesToLoad);

    //loads every texture these materials are missing in one parallel batch per asset type, only MaterialRegistry calls it, on the GL context thread
    static void loadTextures(const std::vector<std::shared_ptr<Material>> &materials);

    //acquiredTexture already holds its reference, nullptr empties the slot
    void assignTexture(const TextureSlot &slot, const std::shared_ptr<TextureAsset> &acquiredTexture);
    bool putRemoveTextureButton(const TextureSlot &slot);

    //only MaterialRegistry calls these, they make GL calls
    void collectTextureFiles(std::vector<size_t> &slotIndices, std::vector<std::vector<std::string>> &textureFiles) const;
    void attachTexture(size_t slotIndex, const std::shared_ptr<TextureAsset> &acquiredTexture);
    void detachTextures();
    void loadGPUSide();
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
        this->name = "copy_" + other.getName();

        this->ambientColor = other.ambientColor;
        this->diffuseColor = other.diffuseColor;
        this->specularColor = other.specularColor;

        this->assetManager = other.assetManager;

        this->specularExponent = other.specularExponent;
        this->refractionIndex = other.refractionIndex;

        this->maps = other.maps;

        //names only, the copy has no slot. Its own vector, assignTexture edits it in place
        for (const TextureSlot &slot : getTextureSlots()) {
            this->*slot.isMap = other.*slot.isMap;
        }
        if (other.textureNames != nullptr) {
            this->textureNames = std::make_shared<std::vector<std::vector<std::string>>>(*other.textureNames);
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
    void restoreValuesFrom(const Material &other);

    //material colors come from model files, which keep them sRGB encoded like their textures, and the shaders light in linear.
    //The curve is the one the hardware uses to decode sRGB textures
    static float srgbToLinear(float encodedValue) {
        if (encodedValue <= 0.04045f) {
            return encodedValue / 12.92f;
        }
        return std::pow((encodedValue + 0.055f) / 1.055f, 2.4f);
    }

    static glm::vec3 srgbToLinear(const glm::vec3 &encodedColor) {
        return glm::vec3(srgbToLinear(encodedColor.x), srgbToLinear(encodedColor.y), srgbToLinear(encodedColor.z));
    }

    // 5,6,7,8,9 are used by material, 3 and 4 stay assignable in material stages too
    static constexpr int32_t MATERIAL_SAMPLER_TEXTURE_UNIT_START = GraphicsInterface::FIRST_PIPELINE_TEXTURE_UNIT + 2;

    enum class Sampler : int32_t { DIFFUSE = 0, AMBIENT = 1, SPECULAR = 2, OPACITY = 3, NORMAL = 4 };
    static constexpr int32_t samplerUnit(Sampler sampler) {
        return MATERIAL_SAMPLER_TEXTURE_UNIT_START + static_cast<int32_t>(sampler);
    }
    static constexpr int32_t SAMPLER_COUNT = 5;
    static const char* samplerName(Sampler sampler) {
        //not a switch: GCC 15.2.0 (MSYS2 Rev9, mingw64 and ucrt64) at -O2 jump threads GraphicsPipeline::isEngineOwnedSampler's loop over it
        //past OPACITY, so opacitySampler got a pipeline unit and opacity maps did nothing. -O0/-O1/-O3 and clang are fine, no report filed yet
        //switch (sampler) {
        //    case Sampler::DIFFUSE:  return "diffuseSampler";
        //    case Sampler::AMBIENT:  return "ambientSampler";
        //    case Sampler::SPECULAR: return "specularSampler";
        //    case Sampler::OPACITY:  return "opacitySampler";
        //    case Sampler::NORMAL:   return "normalSampler";
        //}
        //return "";
        static const char *const SAMPLER_NAMES[SAMPLER_COUNT] = {"diffuseSampler", "ambientSampler", "specularSampler", "opacitySampler", "normalSampler"};
        return SAMPLER_NAMES[static_cast<int32_t>(sampler)];
    }

    static void configureProgram(const std::shared_ptr<GraphicsProgram>& program);

    void activateTextures(GraphicsInterface* graphicsWrapper) const;

    std::string getName() const {
        std::lock_guard<std::mutex> lock(nameMutex);
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

    //names only, the textures load when a mesh shows the material
    void setAmbientTextureName(const std::string &ambientTexture, std::string* sourceAsset = nullptr) {
        setTextureName(getTextureSlots()[0], sourceAsset == nullptr ? std::vector<std::string>{ambientTexture} : std::vector<std::string>{ambientTexture, *sourceAsset});
    }

    std::shared_ptr<TextureAsset> getDiffuseTexture() const {
        return diffuseTexture;
    }

    void setDiffuseTextureName(const std::string &diffuseTexture, std::string* sourceAsset = nullptr) {
        setTextureName(getTextureSlots()[1], sourceAsset == nullptr ? std::vector<std::string>{diffuseTexture} : std::vector<std::string>{diffuseTexture, *sourceAsset});
    }

    std::shared_ptr<TextureAsset>getSpecularTexture() const {
        return specularTexture;
    }

    void setSpecularTextureName(const std::string &specularTexture, std::string* sourceAsset = nullptr) {
        setTextureName(getTextureSlots()[2], sourceAsset == nullptr ? std::vector<std::string>{specularTexture} : std::vector<std::string>{specularTexture, *sourceAsset});
    }

    void setNormalTextureName(const std::string &normalTexture, std::string* sourceAsset = nullptr) {
        setTextureName(getTextureSlots()[3], sourceAsset == nullptr ? std::vector<std::string>{normalTexture} : std::vector<std::string>{normalTexture, *sourceAsset});
    }

    std::shared_ptr<TextureAsset> getNormalTexture() const {
        return normalTexture;
    }

    void setOpacityTextureName(const std::string &opacityTexture, std::string* sourceAsset = nullptr) {
        setTextureName(getTextureSlots()[4], sourceAsset == nullptr ? std::vector<std::string>{opacityTexture} : std::vector<std::string>{opacityTexture, *sourceAsset});
    }

    std::shared_ptr<TextureAsset> getOpacityTexture() const {
        return opacityTexture;
    }

    ~Material() {
        //std::cerr << "Destructor for " << name << std::endl;

        detachTextures();
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

    uint64_t getHash() const;

    //the base we globally override, 0 if none. Set once when a world overrides that base, or read
    //from XML, never recomputed from current content
    uint64_t getOriginalHash() const;


    ImGuiResult addImGuiEditorElements(const ImGuiRequest &request) override;

    //key is a label the world file refers to this material by, never recomputed on load. worldOverride marks the entries that
    //become forwarding rules, the rest are only used by the objects naming them
    bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *materialsNode, const std::string &key, bool worldOverride) const;
    std::string getSaveKey() const;
    static std::shared_ptr<Material> deserialize(AssetManager* assetManager, tinyxml2::XMLElement *materialNode);

    //the same rename on a <Material> written by serialize, for references to a converted asset in a saved world
    static bool updateSavedReferences(tinyxml2::XMLElement *materialNode, const std::string &fromOwner, const std::string &toOwner,
                                      const std::unordered_map<uint64_t, uint64_t> &materialHashes);

    //a .limonmodel owns its embedded textures under its own name, so writing one renames their owner, and every hash
    //of the materials changes with it. Set only while ModelAsset::convertToLimon hashes and writes, on its own thread
    struct EmbeddedOwnerRename {
        std::string from;
        std::string to;
        const std::unordered_map<uint64_t, uint64_t> *hashes = nullptr;
    };

    static const EmbeddedOwnerRename *&activeOwnerRename();

    // This is a method, that wraps all active materials to their names that were in the model that loaded them (if such a thing exists)
    // It is static, because we don't wanna have each model to have its own copy, and in it, there is a thread local, so it doesn't collide
    // with multiple savers trying to save at the same time.
    static const std::map<const Material *, std::string> *&activeSaveNames();

    static std::vector<std::string> renamedTextureName(const std::vector<std::string> &textureName);

    //never the texture pointer, the packer thread calls this
    std::vector<std::string> getTextureName(size_t slotIndex) const;

    bool isLoaded() const {
        return materialIndex != 0;
    }

    std::vector<std::pair<std::string, std::string>> getTextureRepairs() const {
        std::vector<std::pair<std::string, std::string>> repairs;
        for (size_t slotIndex = 0; requestedTextureNames != nullptr && slotIndex < TEXTURE_SLOT_COUNT; ++slotIndex) {
            if (!(*requestedTextureNames)[slotIndex].empty()) {
                repairs.emplace_back((*requestedTextureNames)[slotIndex], (*textureNames)[slotIndex][0]);
            }
        }
        return repairs;
    }

    std::vector<std::vector<std::string>> getTextureFiles() const {
        std::vector<std::vector<std::string>> textureFiles;
        for (size_t slotIndex = 0; slotIndex < TEXTURE_SLOT_COUNT; ++slotIndex) {
            std::vector<std::string> textureName = getTextureName(slotIndex);
            if (!textureName.empty()) {
                textureFiles.push_back(textureName);
            }
        }
        return textureFiles;
    }

    //the same files as getTextureFiles, split by the asset type their slot loads them as
    void getTextureFilesByType(std::vector<std::vector<std::string>> &colorFiles, std::vector<std::vector<std::string>> &linearFiles) const {
        for (size_t slotIndex = 0; slotIndex < TEXTURE_SLOT_COUNT; ++slotIndex) {
            std::vector<std::string> textureName = getTextureName(slotIndex);
            if (textureName.empty()) {
                continue;
            }
            if (getTextureSlots()[slotIndex].isColor) {
                colorFiles.push_back(textureName);
            } else {
                linearFiles.push_back(textureName);
            }
        }
    }

    //"*N" of each texture embedded in owner, the images a file written for this material has to carry
    void addEmbeddedTextureIndices(const std::string &owner, std::set<uint32_t> &indices) const;
#ifdef CEREAL_SUPPORT
    template<class Archive>
    void save(Archive & archive) const {
        //per slot: texture file, then the model it is embedded in, if it is
        std::string textureNameArray[TEXTURE_SLOT_COUNT * 2];
        for (size_t slotIndex = 0; slotIndex < TEXTURE_SLOT_COUNT; ++slotIndex) {
            std::vector<std::string> textureName = renamedTextureName(getTextureName(slotIndex));
            if (textureName.empty()) {
                continue;
            }
            textureNameArray[slotIndex * 2] = textureName[0];
            if (textureName.size() == 2) {
                textureNameArray[slotIndex * 2 + 1] = textureName[1];
            }
        }
        uint64_t originalHashToWrite = originalHash;
        const EmbeddedOwnerRename *rename = activeOwnerRename();
        if (rename != nullptr && rename->hashes != nullptr) {
            std::unordered_map<uint64_t, uint64_t>::const_iterator renamedHash = rename->hashes->find(originalHash);
            if (renamedHash != rename->hashes->end()) {
                originalHashToWrite = renamedHash->second;
            }
        }

        std::string nameToWrite = getName();
        const std::map<const Material *, std::string> *saveNames = activeSaveNames();
        if (saveNames != nullptr) {
            std::map<const Material *, std::string>::const_iterator ownName = saveNames->find(this);
            if (ownName != saveNames->end()) {
                nameToWrite = ownName->second;
            }
        }
        archive(nameToWrite, specularExponent, maps, ambientColor, diffuseColor, specularColor, isAmbientMap, isDiffuseMap, isSpecularMap, isNormalMap, isOpacityMap, refractionIndex, originalHashToWrite,
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

    //before registering, the hash needs the repaired names
    void afterLoad(AssetManager* assetManager) {
        this->assetManager = assetManager;
        if (textureNames == nullptr) {
            return;
        }
        std::vector<std::vector<std::string>> storedNames = *textureNames;
        for (size_t slotIndex = 0; slotIndex < TEXTURE_SLOT_COUNT; ++slotIndex) {
            if (!storedNames[slotIndex].empty()) {
                setTextureName(getTextureSlots()[slotIndex], storedNames[slotIndex]);
            }
        }
    }
#endif
};



#endif //LIMONENGINE_MATERIAL_H
