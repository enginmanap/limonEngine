//
// Created by engin on 27.04.2020.
//

#include "GraphicsProgramLoader.h"
#include "Material.h"

bool GraphicsProgramLoader::isSamplerType(Uniform::VariableTypes type) {
    return type == Uniform::VariableTypes::TEXTURE_2D || type == Uniform::VariableTypes::TEXTURE_2D_ARRAY ||
           type == Uniform::VariableTypes::CUBEMAP || type == Uniform::VariableTypes::CUBEMAP_ARRAY;
}

std::shared_ptr<GraphicsProgram> GraphicsProgramLoader::deserialize(tinyxml2::XMLElement *programNode, std::shared_ptr<AssetManager> assetManager, std::map<std::string, uint32_t>& legacySamplerUnits) {
    std::string vertexShader;
    std::string geometryShader;
    std::string fragmentShader;

    tinyxml2::XMLElement* programNodeAttribute = programNode->FirstChildElement("VertexShader");
    if (programNodeAttribute != nullptr) {
        if(programNodeAttribute->GetText() == nullptr) {
            std::cerr << "Graphics Program vertex shader has no text, this case is not handled!" << std::endl;
            return nullptr;
        } else {
            vertexShader = programNodeAttribute->GetText();
        }
    }

    programNodeAttribute = programNode->FirstChildElement("GeometryShader");
    if (programNodeAttribute != nullptr) {
        if(programNodeAttribute->GetText() == nullptr) {
            //std::cout << "Graphics Program geometry shader has no text." << std::endl;
        } else {
            geometryShader = programNodeAttribute->GetText();
        }
    }

    programNodeAttribute = programNode->FirstChildElement("FragmentShader");
    if (programNodeAttribute != nullptr) {
        if(programNodeAttribute->GetText() == nullptr) {
            std::cerr << "Graphics Program vertex shader has no text, this case is not handled!" << std::endl;
            return nullptr;
        } else {
            fragmentShader = programNodeAttribute->GetText();
        }
    }

    std::shared_ptr<GraphicsProgram> newProgram;
    if(geometryShader.length() > 0 ) {
        newProgram = std::make_shared<GraphicsProgram>(assetManager.get(), vertexShader, geometryShader, fragmentShader);
    } else {
        newProgram = std::make_shared<GraphicsProgram>(assetManager.get(), vertexShader, fragmentShader);
    }
    Material::configureProgram(newProgram);

    tinyxml2::XMLElement *presetValuesNode = programNode->FirstChildElement("PresetValues");
    if(presetValuesNode != nullptr) {
        tinyxml2::XMLElement *uniformNode = presetValuesNode->FirstChildElement("Uniform");
        while (uniformNode != nullptr) {
            const char *nameRaw = uniformNode->Attribute("Name");
            if (nameRaw == nullptr) {
                std::cerr << "uniform name of a preset value can't be read, skipping" << std::endl;
            } else {
                std::string uniformName = nameRaw;
                std::string value = uniformNode->GetText();
                std::unordered_map<std::string, std::shared_ptr<Uniform>>::const_iterator uniformIterator = newProgram->graphicsProgramAsset->getUniformMap().find(uniformName);
                if (uniformIterator != newProgram->graphicsProgramAsset->getUniformMap().end()) {
                    if (isSamplerType(uniformIterator->second->type)) {
                        legacySamplerUnits[uniformName] = (uint32_t)std::stoul(value);
                    } else {
                        newProgram->addPresetValue(uniformName, value);
                    }
                }
            }
            uniformNode = uniformNode->NextSiblingElement("Uniform");
        }
    }
    return newProgram;
}

bool GraphicsProgramLoader::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, const std::shared_ptr<GraphicsProgram> graphicsProgram) {
    tinyxml2::XMLElement *programNode = document.NewElement("GraphicsProgram");
    parentNode->InsertEndChild(programNode);
    tinyxml2::XMLElement *currentElement = nullptr;

    currentElement = document.NewElement("VertexShader");
    currentElement->SetText(graphicsProgram->graphicsProgramAsset->getVertexShaderFile().c_str());
    programNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("GeometryShader");
    currentElement->SetText(graphicsProgram->graphicsProgramAsset->getGeometryShaderFile().c_str());
    programNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("FragmentShader");
    currentElement->SetText(graphicsProgram->graphicsProgramAsset->getFragmentShaderFile().c_str());
    programNode->InsertEndChild(currentElement);

    if(!graphicsProgram->presetUniformValues.empty()) {
        currentElement = document.NewElement("PresetValues");
        for(auto uniformEntry: graphicsProgram->presetUniformValues){
            if (isSamplerType(uniformEntry.first->type)) {
                continue;//units are assigned on the machine that loads the pipeline
            }
            tinyxml2::XMLElement *uniformNode = document.NewElement("Uniform");
            uniformNode->SetAttribute("Name", uniformEntry.first->name.c_str());
            uniformNode->SetText(uniformEntry.second.c_str());
            currentElement->InsertEndChild(uniformNode);
        }
        programNode->InsertEndChild(currentElement);
    }
    programNode->InsertEndChild(currentElement);
    return true;
}
