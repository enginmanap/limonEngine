//
// Created by engin on 19.06.2016.
//

#include <ImGui/imgui.h>
#include "Material.h"
#include "API/Graphics/GraphicsInterface.h"

void Material::afterDeserialize(AssetManager *assetManager, std::string modelAssetFileName) {
    if(textureNameListList == nullptr || textureNameListList->size() != 5) {
        std::cerr << "Texture deserialize didn't set proper values. Please check for material " << name << ":" << materialIndex << std::endl;
    } else {
        this->assetManager = assetManager;
        if(!textureNameListList->at(0).empty()) {
            //file rename handling
            if(textureNameListList->at(0).size() == 2) {
                textureNameListList->at(0)[1] = modelAssetFileName;
            }
            this->ambientTexture = assetManager->loadAsset<TextureAsset>(textureNameListList->at(0));
        }
        if(!textureNameListList->at(1).empty()) {
            //file rename handling
            if(textureNameListList->at(1).size() == 2) {
                textureNameListList->at(1)[1] = modelAssetFileName;
            }
            this->diffuseTexture = assetManager->loadAsset<TextureAsset>(textureNameListList->at(1));
        }
        if(!textureNameListList->at(2).empty()) {
            //file rename handling
            if(textureNameListList->at(2).size() == 2) {
                textureNameListList->at(2)[1] = modelAssetFileName;
            }
            this->specularTexture = assetManager->loadAsset<TextureAsset>(textureNameListList->at(2));
        }
        if(!textureNameListList->at(3).empty()) {
            //file rename handling
            if(textureNameListList->at(3).size() == 2) {
                textureNameListList->at(3)[1] = modelAssetFileName;
            }
            this->normalTexture = assetManager->loadAsset<TextureAsset>(textureNameListList->at(3));
        }
        if(!textureNameListList->at(4).empty()) {
            //file rename handling
            if(textureNameListList->at(4).size() == 2) {
                textureNameListList->at(4)[1] = modelAssetFileName;
            }
            this->opacityTexture = assetManager->loadAsset<TextureAsset>(textureNameListList->at(4));
        }
    }
    textureNameListList.reset();

    this->materialIndex = assetManager->getGraphicsWrapper()->getNextMaterialIndex();
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

    if(this->ambientTexture == nullptr) {
        ImGui::Text("Ambient Texture: not set");
    } else {
        ImGui::Text("%s", (std::string("Ambient Texture: ") + this->ambientTexture->getName().at(0)).c_str());
    }
    if(this->diffuseTexture == nullptr) {
        ImGui::Text("Diffuse Texture: not set");
    } else {
        ImGui::Text("%s", (std::string("Diffuse Texture: ") + this->diffuseTexture->getName().at(0)).c_str());
    }
    if(this->specularTexture == nullptr) {
        ImGui::Text("Specular Texture: not set");
    } else {
        ImGui::Text("%s", (std::string("Specular Texture: ") + this->specularTexture->getName().at(0)).c_str());
    }
    if(this->normalTexture == nullptr) {
        ImGui::Text("Normal Texture: not set");
    } else {
        ImGui::Text("%s", (std::string("Normal Texture: ") + this->normalTexture->getName().at(0)).c_str());
    }
    if(this->opacityTexture == nullptr) {
        ImGui::Text("Opacity Texture: not set");
    } else {
        ImGui::Text("%s", (std::string("Opacity Texture: ") + this->opacityTexture->getName().at(0)).c_str());
    }

    if(dirty) {
        assetManager->getGraphicsWrapper()->setMaterial(*this);
    }
    return result;
}

size_t Material::getHash() const {
    std::hash<Material> hashGenerator;
    return hashGenerator(*this);
}
