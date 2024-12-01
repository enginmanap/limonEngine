//
// Created by engin on 19.06.2016.
//

#include <ImGui/imgui.h>
#include "Material.h"
#include "API/Graphics/GraphicsInterface.h"

void Material::loadGPUSide(AssetManager *assetManager) {
    if(deserialized) {
        return;
    }
    this->assetManager = assetManager;
    if(this->ambientTexture != nullptr) {
        assetManager->partialLoadGPUSide(ambientTexture);
    }
    if(this->diffuseTexture != nullptr) {
        assetManager->partialLoadGPUSide(diffuseTexture);
    }
    if(this->specularTexture != nullptr) {
        assetManager->partialLoadGPUSide(specularTexture);
    }
    if(this->normalTexture != nullptr) {
        assetManager->partialLoadGPUSide(normalTexture);
    }
    if(this->opacityTexture != nullptr) {
        assetManager->partialLoadGPUSide(opacityTexture);
    }
    deserialized = true;
    this->materialIndex = assetManager->getGraphicsWrapper()->getNextMaterialIndex();
}

ImGuiResult Material::addImGuiEditorElements(const ImGuiRequest &request [[gnu::unused]]) {
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

size_t Material::getOriginalHash() const {
    return originalHash;
}
