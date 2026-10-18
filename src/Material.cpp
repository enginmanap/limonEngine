//
// Created by engin on 19.06.2016.
//

#include <ImGui/imgui.h>
#include "Material.h"
#include <sstream>
#include "Utils/HashUtil.h"

#include <ImGuiHelper.h>
#include "XMLHelper.h"
#include "LimonFileSystem.h"

#include "limonAPI/Graphics/GraphicsInterface.h"
#include "limonAPI/Graphics/GraphicsProgram.h"

static std::string firstTextureIn(const AssetManager::AvailableAssetsNode *assetsNode) {
    if (assetsNode->assetType != AssetManager::Asset_type_DIRECTORY) {
        return assetsNode->fullPath;
    }
    for (const AssetManager::AvailableAssetsNode *child : assetsNode->children) {
        std::string found = firstTextureIn(child);
        if (!found.empty()) {
            return found;
        }
    }
    return "";
}

std::string Material::findTextureByName(const std::string &fileName) const {
    const AssetManager::AvailableAssetsNode *matches = assetManager->getAvailableAssetsTreeFiltered(AssetManager::Asset_type_TEXTURE, fileName);
    return matches == nullptr ? "" : firstTextureIn(matches);
}

//model files name textures wherever the artist kept them, so a path that isn't a file is looked up by name. The slot keeps
//what was found, so saving the material writes a path that loads directly from then on
std::string Material::resolveTexturePath(const std::string &requestedPath) const {
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
    std::cerr << "Material " << getName() << " names texture " << requestedPath << ", using " << foundPath << " found by file name." << std::endl;
    return foundPath;
}

void Material::loadGPUSide(AssetManager *assetManager) {
    if (!assetManager->getMaterialRegistry().isRegistered(*this)) {
        // If someone tries to upload without registering, we won't allow it.
        std::cerr << "Material " << getName() << " got loadGPUSide before registerMaterial, nothing loaded." << std::endl;
        return;
    }
    this->assetManager = assetManager;
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
        if (this->*slot.texture == nullptr) {
            ImGui::Text("%s Texture: not set", slot.displayName);
        } else {
            ImGui::Text("%s Texture: %s", slot.displayName, (this->*slot.texture)->getName().at(0).c_str());
        }
        if (request.imgGuiHelper != nullptr) {
            ImGui::SameLine();
            std::string pickedTexture = request.imgGuiHelper->pickTexture(std::string("MaterialTexture") + slot.imGuiIDSuffix);
            if (!pickedTexture.empty()) {
                assignTexture(slot, assetManager->loadAsset<TextureAsset>({pickedTexture}));
                dirty = true;//without it the edit stays on the asset's material, never splits into a world override
            }
        }
        ImGui::SameLine();
        dirty = putRemoveTextureButton(slot) || dirty;
    }
    if(dirty) {
        assetManager->getGraphicsWrapper()->setMaterial(*this);
        result.materialDirty = true;//the pane needs to know before this edit can reach anything shared
    }
    return result;
}

void Material::assignTexture(const TextureSlot &slot, const std::shared_ptr<TextureAsset> &acquiredTexture) {
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
}

bool Material::putRemoveTextureButton(const TextureSlot &slot) {
    bool isEmpty = this->*slot.texture == nullptr;//captured, the click empties it before EndDisabled
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

bool Material::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *materialsNode) const {
    tinyxml2::XMLElement *materialNode = document.NewElement("Material");
        materialsNode->InsertEndChild(materialNode);
        tinyxml2::XMLElement *materialNameNode = document.NewElement("Name");
        materialNameNode->SetText(this->getName().c_str());
        materialNode->InsertEndChild(materialNameNode);
        tinyxml2::XMLElement *materialAmbientColorNode = document.NewElement("AmbientColor");
        XMLHelper::writeVec3(document, materialAmbientColorNode, this->getAmbientColor());
        materialNode->InsertEndChild(materialAmbientColorNode);
        tinyxml2::XMLElement *materialDiffuseColorNode = document.NewElement("DiffuseColor");
        XMLHelper::writeVec3(document, materialDiffuseColorNode, this->getDiffuseColor());
        materialNode->InsertEndChild(materialDiffuseColorNode);
        tinyxml2::XMLElement *materialSpecularColorNode = document.NewElement("SpecularColor");
        XMLHelper::writeVec3(document, materialSpecularColorNode, this->getSpecularColor());
        materialNode->InsertEndChild(materialSpecularColorNode);
        tinyxml2::XMLElement *materialIndexNode = document.NewElement("MaterialIndex");
        materialIndexNode->SetText(this->getMaterialIndex());
        materialNode->InsertEndChild(materialIndexNode);
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
        for (const TextureSlot &slot : getTextureSlots()) {
            if (this->*slot.texture == nullptr) {
                continue;
            }
            tinyxml2::XMLElement *materialTextureNode = document.NewElement(slot.xmlElementName);
            materialTextureNode->SetText(StringUtils::join((this->*slot.texture)->getName(), ",").c_str());
            materialNode->InsertEndChild(materialTextureNode);
        }
    return true;
}

bool Material::updateSavedReferences(tinyxml2::XMLElement *materialNode, const std::string &fromOwner, const std::string &toOwner,
                                           const std::unordered_map<size_t, size_t> &materialHashes) {
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
        size_t originalHash = 0;
        std::stringstream stream(originalHashNode->GetText());
        stream >> originalHash;
        std::unordered_map<size_t, size_t>::const_iterator renamedHash = materialHashes.find(originalHash);
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
    const std::shared_ptr<TextureAsset> &texture = this->*getTextureSlots()[slotIndex].texture;
    if (texture != nullptr) {
        return texture->getName();
    }
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

        size_t originalHash = 0;
        uint32_t materialIndex = std::stoi(materialNode->FirstChildElement("MaterialIndex")->GetText());
        float specularExponent = std::stof(materialNode->FirstChildElement("SpecularExponent")->GetText());
        float refractionIndex = std::stof(materialNode->FirstChildElement("RefractionIndex")->GetText());
        if (materialNode->FirstChildElement("OriginalHash") != nullptr && materialNode->FirstChildElement("OriginalHash")->GetText() != nullptr) {
            std::string originalHashStr = materialNode->FirstChildElement("OriginalHash")->GetText();
            std::stringstream stream(originalHashStr);
            stream >> originalHash;
        }

        std::shared_ptr<Material> material = std::make_shared<Material>(assetManager, name, materialIndex, specularExponent, ambientColor, diffuseColor, specularColor,
                                                                        refractionIndex);

        uint32_t maps = 0;
        for (const TextureSlot &slot : getTextureSlots()) {
            tinyxml2::XMLElement *textureNode = materialNode->FirstChildElement(slot.xmlElementName);
            if (textureNode) {
                //texture file, then the model it is embedded in, if it is
                std::vector<std::string> textureNames = StringUtils::split(textureNode->GetText(), ",");
                if (textureNames.size() == 1 || textureNames.size() == 2) {
                    material->loadTextureIntoSlot(slot, textureNames);
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

size_t Material::getHash() const {
    std::string bytes;
    HashUtil::appendFloat(bytes, specularExponent);
    HashUtil::appendUint32(bytes, maps);
    HashUtil::appendVec3(bytes, ambientColor);
    HashUtil::appendVec3(bytes, diffuseColor);
    HashUtil::appendVec3(bytes, specularColor);
    HashUtil::appendFloat(bytes, refractionIndex);
    //a deserialized material is registered before afterLoad attaches its textures, so until then the names it
    //will attach stand in. Without them limonmodel materials differing only in textures merged into one
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

size_t Material::getOriginalHash() const {
    return originalHash;
}
