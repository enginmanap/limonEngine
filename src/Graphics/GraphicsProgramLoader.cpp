//
// Created by engin on 27.04.2020.
//

#include "GraphicsProgramLoader.h"

std::shared_ptr<GraphicsProgram> GraphicsProgramLoader::deserialize(tinyxml2::XMLElement *programNode, GraphicsInterface* graphicsWrapper) {
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
        newProgram = graphicsWrapper->createGraphicsProgram(vertexShader, geometryShader, fragmentShader, materialRequired);
    } else {
        newProgram = graphicsWrapper->createGraphicsProgram(vertexShader, fragmentShader, materialRequired);
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
                if (newProgram->uniformMap.find(uniformName) != newProgram->uniformMap.end()) {
                    newProgram->presetUniformValues[newProgram->uniformMap[uniformName]] = value;
                    switch (newProgram->uniformMap[uniformName]->type) {
                        case GraphicsInterface::VariableTypes::INT:
                        case GraphicsInterface::VariableTypes::CUBEMAP:
                        case GraphicsInterface::VariableTypes::CUBEMAP_ARRAY:
                        case GraphicsInterface::VariableTypes::TEXTURE_2D:
                        case GraphicsInterface::VariableTypes::TEXTURE_2D_ARRAY:
                            newProgram->setUniform(uniformName, std::stoi(value));
                            std::cout << "found preset uniform " << uniformName << " and set value to " << std::stoi(value) << std::endl;
                            break;
                        case GraphicsInterface::VariableTypes::FLOAT:
                            newProgram->setUniform(uniformName, std::stof(value));
                            std::cout << "found preset uniform " << uniformName << " and set value to " << std::stof(value) << std::endl;
                            break;
                        case GraphicsInterface::VariableTypes::FLOAT_VEC2:
                        case GraphicsInterface::VariableTypes::FLOAT_VEC3:
                        case GraphicsInterface::VariableTypes::FLOAT_VEC4:
                        case GraphicsInterface::VariableTypes::FLOAT_MAT4:
                        case GraphicsInterface::VariableTypes::UNDEFINED:
                            std::cerr << "Deserializing the given type is not implemented! name: " << uniformName << ", value: " << value << std::endl;
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
    currentElement->SetText(graphicsProgram->vertexShader.c_str());
    programNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("GeometryShader");
    currentElement->SetText(graphicsProgram->geometryShader.c_str());
    programNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("FragmentShader");
    currentElement->SetText(graphicsProgram->fragmentShader.c_str());
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