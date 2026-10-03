//
// Created by engin on 19.06.2016.
//

#include <ImGui/imgui.h>
#include "Material.h"

#include <ImGuiHelper.h>
#include "XMLHelper.h"

#include "limonAPI/Graphics/GraphicsInterface.h"
#include "limonAPI/Graphics/GraphicsProgram.h"

void Material::loadGPUSide(AssetManager *assetManager) {
    if(deserialized) {
        return;
    }
    this->assetManager = assetManager;
    for (const TextureSlot &slot : getTextureSlots()) {
        if (this->*slot.texture != nullptr) {
            assetManager->partialLoadGPUSide(this->*slot.texture);
        }
    }
    deserialized = true;
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
    static const AssetManager::AvailableAssetsNode* selectedAsset = nullptr;
    static int32_t selectedSlotIndex = -1;
    const std::array<TextureSlot, TEXTURE_SLOT_COUNT> &textureSlots = getTextureSlots();
    for (size_t slotIndex = 0; slotIndex < textureSlots.size(); ++slotIndex) {
        const TextureSlot &slot = textureSlots[slotIndex];
        if (this->*slot.texture == nullptr) {
            ImGui::Text("%s Texture: not set", slot.displayName);
        } else {
            ImGui::Text("%s Texture: %s", slot.displayName, (this->*slot.texture)->getName().at(0).c_str());
        }
        ImGui::SameLine();
        if (ImGui::Button((std::string("Change##") + slot.imGuiIDSuffix).c_str())) {
            selectedSlotIndex = static_cast<int32_t>(slotIndex);
            ImGui::OpenPopup("Select Texture##TextureSelectorPopup");
        }
        ImGui::SameLine();
        dirty = putRemoveTextureButton(slot) || dirty;
    }
    ImGui::SetNextWindowSize(ImVec2(400.0f, 400.0f));
    if (ImGui::BeginPopup("Select Texture##TextureSelectorPopup", ImGuiWindowFlags_AlwaysAutoResize)) {
        const AssetManager::AvailableAssetsNode* filteredAssets = assetManager->getAvailableAssetsTreeFiltered(AssetManager::Asset_type_TEXTURE, "");
        if(request.imgGuiHelper != nullptr) {
            request.imgGuiHelper->buildTreeFromAssets(filteredAssets, AssetManager::Asset_type_TEXTURE,
                                                      "Texture Selector",
                                                      &selectedAsset,
                                                      ImGuiHelper::PreviewMode::Preview);
        }
        if(selectedAsset != nullptr) {
            if (selectedSlotIndex >= 0) {
                assignTexture(textureSlots[selectedSlotIndex], assetManager->loadAsset<TextureAsset>({selectedAsset->name}));
            }
            dirty = true;//without it the edit stays on the asset's material, never splits into a world override
            selectedAsset = nullptr;
            selectedSlotIndex = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
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
    std::hash<Material> hashGenerator;
    return hashGenerator(*this);
}

size_t Material::getOriginalHash() const {
    return originalHash;
}
