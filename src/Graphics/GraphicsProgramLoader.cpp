//
// Created by engin on 27.04.2020.
//

#include "GraphicsProgramLoader.h"

std::shared_ptr<GraphicsProgram> GraphicsProgramLoader::deserialize(tinyxml2::XMLElement *programNode, std::shared_ptr<AssetManager> assetManager) {
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
            std::cout << "Graphics Program geometry shader has no text." << std::endl;
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

    bool materialRequired = false;
    programNodeAttribute = programNode->FirstChildElement("MaterialRequired");
    if (programNodeAttribute != nullptr) {
        if(programNodeAttribute->GetText() == nullptr) {
            std::cerr << "Graphics Program material required flag couldn't be read, assuming no!" << std::endl;
        } else {
            std::string materialRequiredString = programNodeAttribute->GetText();
            if(materialRequiredString == "True") {
                materialRequired = true;
            } else if(materialRequiredString == "False") {
                materialRequired = false;
            } else {
                std::cerr << "Graphics Program material required flag is unknown, assuming no!" << std::endl;
            }
        }
    } else {
        std::cerr << "Graphics Program material required flag not found, assuming no!" << std::endl;
    }

    std::shared_ptr<GraphicsProgram> newProgram;
    if(geometryShader.length() > 0 ) {
        newProgram = std::make_shared<GraphicsProgram>(assetManager.get(), vertexShader, geometryShader, fragmentShader, materialRequired);
    } else {
        newProgram = std::make_shared<GraphicsProgram>(assetManager.get(), vertexShader, fragmentShader, materialRequired);
    }
    if(materialRequired) {
        newProgram->setSamplersAndUBOs();
    }

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
                if (newProgram->graphicsProgramAsset->getUniformMap().find(uniformName) != newProgram->graphicsProgramAsset->getUniformMap().end()) {
                    newProgram->addPresetValue(uniformName, value);
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
    currentElement->SetText(graphicsProgram->graphicsProgramAsset->getVertexShader().c_str());
    programNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("GeometryShader");
    currentElement->SetText(graphicsProgram->graphicsProgramAsset->getGeometryShader().c_str());
    programNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("FragmentShader");
    currentElement->SetText(graphicsProgram->graphicsProgramAsset->getFragmentShader().c_str());
    programNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("MaterialRequired");
    if(graphicsProgram->materialRequired) {
        currentElement->SetText("True");
    } else {
        currentElement->SetText("False");
    }
    programNode->InsertEndChild(currentElement);

    if(!graphicsProgram->presetUniformValues.empty()) {
        currentElement = document.NewElement("PresetValues");
        for(auto uniformEntry: graphicsProgram->presetUniformValues){
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