//
// Created by engin on 19.06.2016.
//

#include <ImGui/imgui.h>
#include "Assets/AssetTreeManager.h"
#include "Material.h"
#include <sstream>
#include "Utils/HashUtil.h"

#include <ImGuiHelper.h>
#include "XMLHelper.h"
#include "LimonFileSystem.h"

#include "limonAPI/Graphics/GraphicsInterface.h"
#include "limonAPI/Graphics/GraphicsProgram.h"

std::string Material::findTextureByName(const std::string &fileName) const {
    return assetManager->getTree()->findFirstAssetPathByName(AssetTreeManager::Asset_type_TEXTURE, fileName);
}

//model files name textures wherever the artist kept them, so a path that isn't a file is looked up by name. The slot keeps
//what was found, so saving the material writes a path that loads directly from then on
std::string Material::resolveTexturePath(const std::string &requestedPath, bool &repaired) const {
    repaired = false;
    if (LimonFileSystem::fileExists(requestedPath)) {
        return requestedPath;
    }
    std::string fileName = requestedPath.substr(requestedPath.find_last_of("\\/") + 1);
    std::string foundPath = findTextureByName(fileName);
    if (foundPath.empty()) {
        foundPath = findTextureByName(fileName.substr(0, fileName.find_last_of('.')));
    }
    if (foundPath.empty()) {
        return requestedPath;//TextureAsset reports it and loads the not found texture
    }
    repaired = true;
    return foundPath;
}

void Material::setTextureName(const TextureSlot &slot, const std::vector<std::string> &textureFiles) {
    size_t slotIndex = &slot - getTextureSlots().data();
    std::vector<std::string> storedFiles = textureFiles;
    bool repaired = false;
    if (storedFiles.size() == 1) {//two names are an embedded texture and its model, nothing to resolve
        storedFiles[0] = resolveTexturePath(textureFiles[0], repaired);
    }
    if (textureNames == nullptr) {
        textureNames = std::make_shared<std::vector<std::vector<std::string>>>(TEXTURE_SLOT_COUNT);
    }
    (*textureNames)[slotIndex] = storedFiles;
    if (repaired) {
        if (requestedTextureNames == nullptr) {
            requestedTextureNames = std::make_unique<std::array<std::string, TEXTURE_SLOT_COUNT>>();
        }
        (*requestedTextureNames)[slotIndex] = textureFiles[0];
    } else if (requestedTextureNames != nullptr) {
        (*requestedTextureNames)[slotIndex].clear();
    }
    this->*slot.isMap = true;
}

void Material::collectTextureFiles(std::vector<size_t> &slotIndices, std::vector<std::vector<std::string>> &textureFiles) const {
    if (textureNames == nullptr) {
        return;
    }
    for (size_t slotIndex = 0; slotIndex < TEXTURE_SLOT_COUNT; ++slotIndex) {
        if (!(*textureNames)[slotIndex].empty() && this->*getTextureSlots()[slotIndex].texture == nullptr) {
            slotIndices.push_back(slotIndex);
            textureFiles.push_back((*textureNames)[slotIndex]);
        }
    }
}

std::shared_ptr<TextureAsset> Material::loadSlotTexture(const TextureSlot &slot, const std::vector<std::string> &textureFiles) const {
    std::shared_ptr<TextureAsset> loadedTexture;
    if (slot.isColor) {
        loadedTexture = assetManager->loadAsset<SRGBTextureAsset>(textureFiles);
    } else {
        loadedTexture = assetManager->loadAsset<TextureAsset>(textureFiles);
    }
    if (loadedTexture == nullptr) {
        failTextureType(slot, textureFiles[0]);
    }
    return loadedTexture;
}

void Material::failTextureType(const TextureSlot &slot, const std::string &textureFile) const {
    //a file is one asset whichever type asked first, nothing deduplicates per type yet
    std::cerr << "Texture " << textureFile << " for the " << slot.displayName << " slot of material " << getName()
              << " is already loaded as another asset type. Exiting..." << std::endl;
    exit(-1);
}

template<class TextureType>
void Material::loadSlotBatch(AssetManager *assetManager, const std::vector<std::pair<Material *, size_t>> &slotsToLoad,
                             const std::vector<std::vector<std::string>> &filesToLoad) {
    if (filesToLoad.empty()) {
        return;
    }
    std::vector<std::shared_ptr<TextureType>> loadedTextures = assetManager->parallelLoadAssetList<TextureType>(filesToLoad);
    for (size_t entryIndex = 0; entryIndex < slotsToLoad.size(); ++entryIndex) {
        Material *material = slotsToLoad[entryIndex].first;
        size_t slotIndex = slotsToLoad[entryIndex].second;
        if (loadedTextures[entryIndex] == nullptr) {
            material->failTextureType(getTextureSlots()[slotIndex], filesToLoad[entryIndex][0]);
        }
        material->attachTexture(slotIndex, loadedTextures[entryIndex]);
    }
}

void Material::loadTextures(const std::vector<std::shared_ptr<Material>> &materials) {
    if (materials.empty()) {
        return;
    }
    AssetManager *assetManager = materials[0]->assetManager;
    //a requester waits behind other threads' loads, so only what nobody loaded yet goes through one
    std::vector<std::pair<Material *, size_t>> colorSlotsToLoad;
    std::vector<std::vector<std::string>> colorFilesToLoad;
    std::vector<std::pair<Material *, size_t>> linearSlotsToLoad;
    std::vector<std::vector<std::string>> linearFilesToLoad;
    for (const std::shared_ptr<Material> &material : materials) {
        std::vector<size_t> slotIndices;
        std::vector<std::vector<std::string>> textureFiles;
        material->collectTextureFiles(slotIndices, textureFiles);
        for (size_t entryIndex = 0; entryIndex < slotIndices.size(); ++entryIndex) {
            bool isColor = getTextureSlots()[slotIndices[entryIndex]].isColor;
            std::shared_ptr<TextureAsset> loadedTexture;
            if (isColor) {
                loadedTexture = assetManager->acquireLoadedAsset<SRGBTextureAsset>(textureFiles[entryIndex]);
            } else {
                loadedTexture = assetManager->acquireLoadedAsset<TextureAsset>(textureFiles[entryIndex]);
            }
            if (loadedTexture != nullptr) {
                material->attachTexture(slotIndices[entryIndex], loadedTexture);
            } else if (isColor) {
                colorSlotsToLoad.emplace_back(material.get(), slotIndices[entryIndex]);
                colorFilesToLoad.push_back(textureFiles[entryIndex]);
            } else {
                linearSlotsToLoad.emplace_back(material.get(), slotIndices[entryIndex]);
                linearFilesToLoad.push_back(textureFiles[entryIndex]);
            }
        }
    }
    loadSlotBatch<SRGBTextureAsset>(assetManager, colorSlotsToLoad, colorFilesToLoad);
    loadSlotBatch<TextureAsset>(assetManager, linearSlotsToLoad, linearFilesToLoad);
}

void Material::attachTexture(size_t slotIndex, const std::shared_ptr<TextureAsset> &acquiredTexture) {
    this->*getTextureSlots()[slotIndex].texture = acquiredTexture;
    if (requestedTextureNames != nullptr && !(*requestedTextureNames)[slotIndex].empty()) {
        std::cerr << "Material " << getName() << " names texture " << (*requestedTextureNames)[slotIndex] << ", using "
                  << (*textureNames)[slotIndex][0] << " found by file name." << std::endl;
    }
}

void Material::detachTextures() {
    //null before freeing, AssetManager reports an asset that still has a shared_ptr to it
    for (const TextureSlot &slot : getTextureSlots()) {
        if (this->*slot.texture != nullptr) {
            std::vector<std::string> textureFiles = (this->*slot.texture)->getName();
            this->*slot.texture = nullptr;
            assetManager->freeAsset(textureFiles);
        }
    }
    deserialized = false;
}

void Material::restoreValuesFrom(const Material &other) {
    this->ambientColor = other.ambientColor;
    this->diffuseColor = other.diffuseColor;
    this->specularColor = other.specularColor;
    this->specularExponent = other.specularExponent;
    this->refractionIndex = other.refractionIndex;

    this->maps = other.maps;

    std::vector<std::vector<std::string>> otherNames(TEXTURE_SLOT_COUNT);
    for (size_t slotIndex = 0; slotIndex < TEXTURE_SLOT_COUNT; ++slotIndex) {
        otherNames[slotIndex] = other.getTextureName(slotIndex);
    }
    for (size_t slotIndex = 0; slotIndex < TEXTURE_SLOT_COUNT; ++slotIndex) {
        const TextureSlot &slot = getTextureSlots()[slotIndex];
        this->*slot.isMap = other.*slot.isMap;
        if (getTextureName(slotIndex) == otherNames[slotIndex]) {
            continue;
        }
        //acquire before free, swapping between two materials that share a texture would drop it to zero in between
        std::shared_ptr<TextureAsset> newTexture = nullptr;
        if (isLoaded() && !otherNames[slotIndex].empty()) {
            newTexture = loadSlotTexture(slot, otherNames[slotIndex]);
        }
        std::shared_ptr<TextureAsset> previousTexture = this->*slot.texture;
        this->*slot.texture = newTexture;
        if (previousTexture != nullptr) {
            assetManager->freeAsset(previousTexture->getName());
        }
        if (textureNames == nullptr) {
            textureNames = std::make_shared<std::vector<std::vector<std::string>>>(TEXTURE_SLOT_COUNT);
        }
        (*textureNames)[slotIndex] = otherNames[slotIndex];
        if (requestedTextureNames != nullptr) {
            (*requestedTextureNames)[slotIndex].clear();
        }
    }
    if (isLoaded()) {
        deserialized = false;
        loadGPUSide();
    }
}

void Material::loadGPUSide() {
    if (!isLoaded()) {
        std::cerr << "Material " << getName() << " got loadGPUSide without a slot, nothing loaded." << std::endl;
        return;
    }
    if(!deserialized) {
        for (const TextureSlot &slot : getTextureSlots()) {
            if (this->*slot.texture != nullptr) {
                assetManager->partialLoadGPUSide(this->*slot.texture);
            }
        }
        assetManager->getGraphicsWrapper()->setMaterial(*this);
        deserialized = true;
    }
}

ImGuiResult Material::addImGuiEditorElements(const ImGuiRequest &request) {
    bool dirty = false;
    ImGuiResult result;

    //let s dump everything for now:
    dirty = ImGui::SliderFloat("Specular Exponent", &this->specularExponent, 0, 99999);
    dirty = ImGui::SliderFloat3("Ambient Color", &this->ambientColor.x, 0, 1) || dirty;
    dirty = ImGui::SliderFloat3("Diffuse Color", &this->diffuseColor.x, 0, 1) || dirty;
    dirty = ImGui::SliderFloat3("Specular Color", &this->specularColor.x, 0, 1) || dirty;
    dirty = ImGui::SliderFloat("Refraction Index", &this->refractionIndex, 0, 99999) || dirty;
    ImGui::Text("%s", (std::string("Maps is ") +  std::to_string(this->maps)).c_str());
    const std::array<TextureSlot, TEXTURE_SLOT_COUNT> &textureSlots = getTextureSlots();
    for (size_t slotIndex = 0; slotIndex < textureSlots.size(); ++slotIndex) {
        const TextureSlot &slot = textureSlots[slotIndex];
        std::vector<std::string> textureName = getTextureName(slotIndex);
        if (textureName.empty()) {
            ImGui::Text("%s Texture: not set", slot.displayName);
        } else {
            ImGui::Text("%s Texture: %s", slot.displayName, textureName.at(0).c_str());
        }
        if (request.imgGuiHelper != nullptr) {
            ImGui::SameLine();
            std::string pickedTexture = request.imgGuiHelper->pickTexture(std::string("MaterialTexture") + slot.imGuiIDSuffix);
            if (!pickedTexture.empty()) {
                assignTexture(slot, loadSlotTexture(slot, {pickedTexture}));
                dirty = true;//without it the edit stays on the asset's material, never splits into a world override
            }
        }
        ImGui::SameLine();
        dirty = putRemoveTextureButton(slot) || dirty;
    }
    if(dirty) {
        if (isLoaded()) {//slot 0 belongs to nobody
            assetManager->getGraphicsWrapper()->setMaterial(*this);
        }
        result.materialDirty = true;//the pane needs to know before this edit can reach anything shared
    }
    return result;
}

void Material::assignTexture(const TextureSlot &slot, const std::shared_ptr<TextureAsset> &acquiredTexture) {
    size_t slotIndex = &slot - getTextureSlots().data();
    if (this->*slot.texture != nullptr) {
        assetManager->freeAsset((this->*slot.texture)->getName());
    }
    this->*slot.texture = acquiredTexture;
    //activateTextures binds by the flag, a set flag on an empty slot dereferences null on the next draw
    this->*slot.isMap = acquiredTexture != nullptr;
    if (acquiredTexture != nullptr) {
        maps |= slot.mapBit;
    } else {
        maps &= ~slot.mapBit;
    }
    //the hash and the writers read the name, not the texture
    if (textureNames == nullptr) {
        textureNames = std::make_shared<std::vector<std::vector<std::string>>>(TEXTURE_SLOT_COUNT);
    }
    (*textureNames)[slotIndex] = acquiredTexture != nullptr ? acquiredTexture->getName() : std::vector<std::string>();
    if (requestedTextureNames != nullptr) {
        (*requestedTextureNames)[slotIndex].clear();
    }
}

bool Material::putRemoveTextureButton(const TextureSlot &slot) {
    bool isEmpty = getTextureName(&slot - getTextureSlots().data()).empty();//captured, the click empties it before EndDisabled
    bool removed = false;
    if (isEmpty) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button((std::string("Remove##") + slot.imGuiIDSuffix).c_str())) {
        assignTexture(slot, nullptr);
        removed = true;
    }
    if (isEmpty) {
        ImGui::EndDisabled();
    }
    return removed;
}

void Material::configureProgram(const std::shared_ptr<GraphicsProgram>& program) {
    if (!program->isMaterialRequired()) {
        return;
    }
    //A sampler the shader doesn't declare simply fails to set (harmless) - e.g. a program that pulls in
    //the material UBO but samples the G-buffer directly, not diffuseSampler.
    for (int32_t samplerIndex = 0; samplerIndex < SAMPLER_COUNT; ++samplerIndex) {
        Sampler sampler = static_cast<Sampler>(samplerIndex);
        program->setUniform(samplerName(sampler), samplerUnit(sampler));
    }
}

void Material::activateTextures(GraphicsInterface* graphicsWrapper) const {
    //Binds each present map to its material-sampler unit. configureProgram (above) sets the matching
    //sampler uniforms to the same units, both via Material::samplerUnit, so the bound texture and the
    //shader's sampler always agree.
    if (!isLoaded()) {
        std::cerr << "Material " << getName() << " is drawn without a slot, a model rendered before its materials were loaded. This is a bug. Exiting..." << std::endl;
        exit(-1);
    }
    if (hasDiffuseMap()) {
        graphicsWrapper->attachTexture(getDiffuseTexture()->getID(), samplerUnit(Sampler::DIFFUSE));
    }
    if (hasAmbientMap()) {
        graphicsWrapper->attachTexture(getAmbientTexture()->getID(), samplerUnit(Sampler::AMBIENT));
    }
    if (hasSpecularMap()) {
        graphicsWrapper->attachTexture(getSpecularTexture()->getID(), samplerUnit(Sampler::SPECULAR));
    }
    if (hasOpacityMap()) {
        graphicsWrapper->attachTexture(getOpacityTexture()->getID(), samplerUnit(Sampler::OPACITY));
    }
    if (hasNormalMap()) {
        graphicsWrapper->attachTexture(getNormalTexture()->getID(), samplerUnit(Sampler::NORMAL));
    }
}

std::string Material::getSaveKey() const {
    return std::to_string(getHash());
}

bool Material::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *materialsNode, const std::string &key, bool worldOverride) const {
    tinyxml2::XMLElement *materialNode = document.NewElement("Material");
        materialNode->SetAttribute("Key", key.c_str());
        if (worldOverride) {
            materialNode->SetAttribute("WorldOverride", 1);
        }
        materialsNode->InsertEndChild(materialNode);
        tinyxml2::XMLElement *materialNameNode = document.NewElement("Name");
        materialNameNode->SetText(this->getName().c_str());
        materialNode->InsertEndChild(materialNameNode);
        tinyxml2::XMLElement *materialAmbientColorNode = document.NewElement("AmbientColor");
        XMLHelper::writeVec3(document, materialAmbientColorNode, this->getRawAmbientColor());
        materialNode->InsertEndChild(materialAmbientColorNode);
        tinyxml2::XMLElement *materialDiffuseColorNode = document.NewElement("DiffuseColor");
        XMLHelper::writeVec3(document, materialDiffuseColorNode, this->getRawDiffuseColor());
        materialNode->InsertEndChild(materialDiffuseColorNode);
        tinyxml2::XMLElement *materialSpecularColorNode = document.NewElement("SpecularColor");
        XMLHelper::writeVec3(document, materialSpecularColorNode, this->getRawSpecularColor());
        materialNode->InsertEndChild(materialSpecularColorNode);
        tinyxml2::XMLElement *materialSpecularExponentNode = document.NewElement("SpecularExponent");
        materialSpecularExponentNode->SetText(this->getSpecularExponent());
        materialNode->InsertEndChild(materialSpecularExponentNode);
        tinyxml2::XMLElement *materialRefractionIndexNode = document.NewElement("RefractionIndex");
        materialRefractionIndexNode->SetText(this->getRefractionIndex());
        materialNode->InsertEndChild(materialRefractionIndexNode);
        tinyxml2::XMLElement *materialOriginalHashNode = document.NewElement("OriginalHash");
        materialOriginalHashNode->SetText((uint64_t)(this->getOriginalHash()));
        materialNode->InsertEndChild(materialOriginalHashNode);

        //now the textures. They might or might not exist, we need to check
        for (size_t slotIndex = 0; slotIndex < TEXTURE_SLOT_COUNT; ++slotIndex) {
            std::vector<std::string> textureName = getTextureName(slotIndex);
            if (textureName.empty()) {
                continue;
            }
            tinyxml2::XMLElement *materialTextureNode = document.NewElement(getTextureSlots()[slotIndex].xmlElementName);
            materialTextureNode->SetText(StringUtils::join(textureName, ",").c_str());
            materialNode->InsertEndChild(materialTextureNode);
        }
    return true;
}

bool Material::updateSavedReferences(tinyxml2::XMLElement *materialNode, const std::string &fromOwner, const std::string &toOwner,
                                           const std::unordered_map<uint64_t, uint64_t> &materialHashes) {
    bool changed = false;
    for (const TextureSlot &slot : getTextureSlots()) {
        tinyxml2::XMLElement *textureNode = materialNode->FirstChildElement(slot.xmlElementName);
        if (textureNode == nullptr || textureNode->GetText() == nullptr) {
            continue;
        }
        std::string textureName = textureNode->GetText();//serialize joins an embedded texture as "*N,owner"
        size_t separator = textureName.find(',');
        if (separator != std::string::npos && textureName[0] == '*' && textureName.substr(separator + 1) == fromOwner) {
            textureNode->SetText((textureName.substr(0, separator + 1) + toOwner).c_str());
            changed = true;
        }
    }
    tinyxml2::XMLElement *originalHashNode = materialNode->FirstChildElement("OriginalHash");
    if (originalHashNode != nullptr && originalHashNode->GetText() != nullptr) {
        uint64_t originalHash = 0;
        std::stringstream stream(originalHashNode->GetText());
        stream >> originalHash;
        std::unordered_map<uint64_t, uint64_t>::const_iterator renamedHash = materialHashes.find(originalHash);
        if (renamedHash != materialHashes.end() && renamedHash->second != originalHash) {
            originalHashNode->SetText(static_cast<uint64_t>(renamedHash->second));
            changed = true;
        }
    }
    return changed;
}

const Material::EmbeddedOwnerRename *&Material::activeOwnerRename() {
    thread_local const EmbeddedOwnerRename *active = nullptr;
    return active;
}

const std::map<const Material *, std::string> *&Material::activeSaveNames() {
    thread_local const std::map<const Material *, std::string> *active = nullptr;
    return active;
}

std::vector<std::string> Material::renamedTextureName(const std::vector<std::string> &textureName) {
    const EmbeddedOwnerRename *rename = activeOwnerRename();
    if (rename != nullptr && textureName.size() == 2 && textureName[1] == rename->from) {
        return {textureName[0], rename->to};
    }
    return textureName;
}

std::vector<std::string> Material::getTextureName(size_t slotIndex) const {
    if (textureNames != nullptr) {
        return (*textureNames)[slotIndex];
    }
    return {};
}

void Material::addEmbeddedTextureIndices(const std::string &owner, std::set<uint32_t> &indices) const {
    for (size_t slotIndex = 0; slotIndex < TEXTURE_SLOT_COUNT; ++slotIndex) {
        std::vector<std::string> textureName = getTextureName(slotIndex);
        if (textureName.size() == 2 && textureName[1] == owner && textureName[0].size() > 1 && textureName[0][0] == '*') {
            indices.insert(static_cast<uint32_t>(std::atoi(textureName[0].c_str() + 1)));
        }
    }
}

std::shared_ptr<Material> Material::deserialize(AssetManager* assetManager, tinyxml2::XMLElement *materialNode) {

        std::string name = materialNode->FirstChildElement("Name")->GetText();

        glm::vec3 ambientColor;
        XMLHelper::readVec3(materialNode->FirstChildElement("AmbientColor"), ambientColor);

        glm::vec3 diffuseColor;
        XMLHelper::readVec3(materialNode->FirstChildElement("DiffuseColor"), diffuseColor);

        glm::vec3 specularColor;
        XMLHelper::readVec3(materialNode->FirstChildElement("SpecularColor"), specularColor);

        uint64_t originalHash = 0;
        float specularExponent = std::stof(materialNode->FirstChildElement("SpecularExponent")->GetText());
        float refractionIndex = std::stof(materialNode->FirstChildElement("RefractionIndex")->GetText());
        if (materialNode->FirstChildElement("OriginalHash") != nullptr && materialNode->FirstChildElement("OriginalHash")->GetText() != nullptr) {
            std::string originalHashStr = materialNode->FirstChildElement("OriginalHash")->GetText();
            std::stringstream stream(originalHashStr);
            stream >> originalHash;
        }

        std::shared_ptr<Material> material = std::make_shared<Material>(assetManager, name, 0, specularExponent, ambientColor, diffuseColor, specularColor,
                                                                        refractionIndex);

        uint32_t maps = 0;
        for (const TextureSlot &slot : getTextureSlots()) {
            tinyxml2::XMLElement *textureNode = materialNode->FirstChildElement(slot.xmlElementName);
            if (textureNode) {
                //texture file, then the model it is embedded in, if it is
                std::vector<std::string> textureNames = StringUtils::split(textureNode->GetText(), ",");
                if (textureNames.size() == 1 || textureNames.size() == 2) {
                    material->setTextureName(slot, textureNames);
                }
            }
            if ((*material).*slot.isMap) {
                maps |= slot.mapBit;
            }
        }
        material->originalHash = originalHash;
        material->setMaps(maps);
        return material;
}

uint64_t Material::getHash() const {
    std::string bytes;
    HashUtil::appendFloat(bytes, specularExponent);
    HashUtil::appendUint32(bytes, maps);
    HashUtil::appendVec3(bytes, ambientColor);
    HashUtil::appendVec3(bytes, diffuseColor);
    HashUtil::appendVec3(bytes, specularColor);
    HashUtil::appendFloat(bytes, refractionIndex);
    //names, never the texture pointers, so it hashes the same with or without its textures
    for (size_t slotIndex = 0; slotIndex < TEXTURE_SLOT_COUNT; ++slotIndex) {
        std::vector<std::string> textureName = renamedTextureName(getTextureName(slotIndex));
        HashUtil::appendUint32(bytes, static_cast<uint32_t>(textureName.size()));
        for (const std::string &namePart : textureName) {
            HashUtil::appendUint32(bytes, static_cast<uint32_t>(namePart.size()));
            bytes += namePart;
        }
    }
    return hash(bytes);
}

uint64_t Material::getOriginalHash() const {
    return originalHash;
}
