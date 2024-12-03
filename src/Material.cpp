//
// Created by engin on 19.06.2016.
//

#include <ImGui/imgui.h>
#include "Material.h"

#include <WorldLoader.h>
#include <WorldSaver.h>

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

bool Material::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *materialsNode) const {
    tinyxml2::XMLElement *materialNode = document.NewElement("Material");
        materialsNode->InsertEndChild(materialNode);
        tinyxml2::XMLElement *materialNameNode = document.NewElement("Name");
        materialNameNode->SetText(this->getName().c_str());
        materialNode->InsertEndChild(materialNameNode);
        tinyxml2::XMLElement *materialAmbientColorNode = document.NewElement("AmbientColor");
        WorldSaver::serializeVec3(document, materialAmbientColorNode, this->getAmbientColor());
        materialNode->InsertEndChild(materialAmbientColorNode);
        tinyxml2::XMLElement *materialDiffuseColorNode = document.NewElement("DiffuseColor");
        WorldSaver::serializeVec3(document, materialDiffuseColorNode, this->getDiffuseColor());
        materialNode->InsertEndChild(materialDiffuseColorNode);
        tinyxml2::XMLElement *materialSpecularColorNode = document.NewElement("SpecularColor");
        WorldSaver::serializeVec3(document, materialSpecularColorNode, this->getSpecularColor());
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
        materialOriginalHashNode->SetText(this->getOriginalHash());
        materialNode->InsertEndChild(materialOriginalHashNode);

        //now the textures. They might or might not exists, we need to check
        if (this->getAmbientTexture() != nullptr) {
            tinyxml2::XMLElement *materialAmbientTextureNode = document.NewElement("AmbientTexture");
            materialAmbientTextureNode->SetText(StringUtils::join(this->getAmbientTexture()->getName(), ",").c_str());
            materialNode->InsertEndChild(materialAmbientTextureNode);
        }

        if (this->getDiffuseTexture() != nullptr) {
            tinyxml2::XMLElement *materialDiffuseTextureNode = document.NewElement("DiffuseTexture");
            materialDiffuseTextureNode->SetText(StringUtils::join(this->getDiffuseTexture()->getName(), ",").c_str());
            materialNode->InsertEndChild(materialDiffuseTextureNode);
        }

        if (this->getSpecularTexture() != nullptr) {
            tinyxml2::XMLElement *materialSpecularTextureNode = document.NewElement("SpecularTexture");
            materialSpecularTextureNode->SetText(StringUtils::join(this->getSpecularTexture()->getName(), ",").c_str());
            materialNode->InsertEndChild(materialSpecularTextureNode);
        }

        if (this->getNormalTexture() != nullptr) {
            tinyxml2::XMLElement *materialNormalTextureNode = document.NewElement("NormalTexture");
            materialNormalTextureNode->SetText(StringUtils::join(this->getNormalTexture()->getName(), ",").c_str());
            materialNode->InsertEndChild(materialNormalTextureNode);
        }

        if (this->getOpacityTexture() != nullptr) {
            tinyxml2::XMLElement *materialOpacityTextureNode = document.NewElement("OpacityTexture");
            materialOpacityTextureNode->SetText(StringUtils::join(this->getOpacityTexture()->getName(), ",").c_str());
            materialNode->InsertEndChild(materialOpacityTextureNode);
        }
    return true;
}

std::shared_ptr<Material> Material::deserialize(AssetManager* assetManager, tinyxml2::XMLElement *materialNode) {

        std::string name = materialNode->FirstChildElement("Name")->GetText();

        glm::vec3 ambientColor;
        WorldLoader::loadVec3(materialNode->FirstChildElement("AmbientColor"), ambientColor);

        glm::vec3 diffuseColor;
        WorldLoader::loadVec3(materialNode->FirstChildElement("DiffuseColor"), diffuseColor);

        glm::vec3 specularColor;
        WorldLoader::loadVec3(materialNode->FirstChildElement("SpecularColor"), specularColor);

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

        tinyxml2::XMLElement *textureNode = materialNode->FirstChildElement("AmbientTexture");
        if (textureNode) {
            std::vector<std::string> textureNames = StringUtils::split(textureNode->GetText(), ",");
            if(textureNames.size() == 1) {
                material->setAmbientTexture(textureNames[0]);
            } else if (textureNames.size() == 2) {
                material->setAmbientTexture(textureNames[0], &textureNames[1]);
            }
        }
        textureNode = materialNode->FirstChildElement("DiffuseTexture");
        if (textureNode) {
            std::vector<std::string> textureNames = StringUtils::split(textureNode->GetText(), ",");
            if(textureNames.size() == 1) {
                material->setDiffuseTexture(textureNames[0]);
            } else if (textureNames.size() == 2) {
                material->setDiffuseTexture(textureNames[0], &textureNames[1]);
            }
        }

        textureNode = materialNode->FirstChildElement("SpecularTexture");
        if (textureNode) {
            std::vector<std::string> textureNames = StringUtils::split(textureNode->GetText(), ",");
            if(textureNames.size() == 1) {
                material->setSpecularTexture(textureNames[0]);
            } else if (textureNames.size() == 2) {
                material->setSpecularTexture(textureNames[0], &textureNames[1]);
            }
        }

        textureNode = materialNode->FirstChildElement("NormalTexture");
        if (textureNode) {
            std::vector<std::string> textureNames = StringUtils::split(textureNode->GetText(), ",");
            if(textureNames.size() == 1) {
                material->setNormalTexture(textureNames[0]);
            } else if (textureNames.size() == 2) {
                material->setNormalTexture(textureNames[0], &textureNames[1]);
            }
        }

        textureNode = materialNode->FirstChildElement("OpacityTexture");
        if (textureNode) {
            std::vector<std::string> textureNames = StringUtils::split(textureNode->GetText(), ",");
            if(textureNames.size() == 1) {
                material->setOpacityTexture(textureNames[0]);
            } else if (textureNames.size() == 2) {
                material->setOpacityTexture(textureNames[0], &textureNames[1]);
            }
        }
        uint32_t maps = 0;

        if(material->hasNormalMap()) {
            maps +=16;
        }
        if(material->hasAmbientMap()) {
            maps +=8;
        }
        if(material->hasDiffuseMap()) {
            maps +=4;
        }
        if(material->hasSpecularMap()) {
            maps +=2;
        }
        if(material->hasOpacityMap()) {
            maps +=1;
        }
        material->originalHash = originalHash;
        material->setMaps(maps);
        assetManager->registerOverriddenMaterial(material);
        return material;


}

size_t Material::getHash() const {
    std::hash<Material> hashGenerator;
    return hashGenerator(*this);
}

size_t Material::getOriginalHash() const {
    return originalHash;
}
