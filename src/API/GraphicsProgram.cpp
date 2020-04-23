//
// Created by Engin Manap on 2.03.2016.
//

#include "GraphicsProgram.h"

GraphicsProgram::GraphicsProgram(GraphicsInterface* graphicsWrapper, std::string vertexShader, std::string fragmentShader, bool isMaterialUsed) :
        graphicsWrapper(graphicsWrapper), vertexShader(vertexShader), fragmentShader(fragmentShader), materialRequired(isMaterialUsed) {
    programName = vertexShader +"|"+ fragmentShader;
    //FIXME is passing empty string acceptable?
    programID = graphicsWrapper->initializeProgram(vertexShader, "", fragmentShader, uniformMap, outputMap);
}

GraphicsProgram::GraphicsProgram(GraphicsInterface* graphicsWrapper, std::string vertexShader, std::string geometryShader, std::string fragmentShader, bool isMaterialUsed) :
        graphicsWrapper(graphicsWrapper), vertexShader(vertexShader), geometryShader(geometryShader), fragmentShader(fragmentShader), materialRequired(isMaterialUsed) {
    programName = vertexShader +"|"+ geometryShader +"|"+ fragmentShader;
    programID = graphicsWrapper->initializeProgram(vertexShader, geometryShader, fragmentShader, uniformMap, outputMap);
}

GraphicsProgram::~GraphicsProgram() {
    graphicsWrapper->destroyProgram(programID);
}


//TODO remove with material editor
void GraphicsProgram::setSamplersAndUBOs() {

    //TODO these will be configurable with material editor
    int diffuseMapAttachPoint = 1;
    int ambientMapAttachPoint = 2;
    int specularMapAttachPoint = 3;
    int opacityMapAttachPoint = 4;
    int normalMapAttachPoint = 5;

    if (!setUniform("diffuseSampler", diffuseMapAttachPoint)) {
        std::cerr << "Uniform \"diffuseSampler\" could not be set" << std::endl;
    }
    if (!setUniform("ambientSampler", ambientMapAttachPoint)) {
        std::cerr << "Uniform \"ambientSampler\" could not be set" << std::endl;
    }
    if (!setUniform("specularSampler", specularMapAttachPoint)) {
        std::cerr << "Uniform \"specularSampler\" could not be set" << std::endl;
    }
    if(uniformMap.find("opacitySampler") != uniformMap.end()) {
        if (!setUniform("opacitySampler", opacityMapAttachPoint)) {
            std::cerr << "Uniform \"opacitySampler\" could not be set" << std::endl;
        }    }
    if (!setUniform("normalSampler", normalMapAttachPoint)) {
        std::cerr << "Uniform \"normalSampler\" could not be set" << std::endl;
    }
    //TODO we should support multi texture on one pass

    if (!setUniform("pre_shadowDirectional", graphicsWrapper->getMaxTextureImageUnits() - 1)) {
        std::cerr << "Uniform \"pre_shadowDirectional\" could not be set" << std::endl;
    }
    if (!setUniform("pre_shadowPoint", graphicsWrapper->getMaxTextureImageUnits() - 2)) {
        std::cerr << "Uniform \"pre_shadowPoint\" could not be set" << std::endl;
    }

    graphicsWrapper->attachModelUBO(getID());
    graphicsWrapper->attachModelIndicesUBO(getID());
}


std::shared_ptr<GraphicsProgram> GraphicsProgram::deserialize(tinyxml2::XMLElement *programNode, GraphicsInterface* graphicsWrapper) {
    std::string vertexShader;
    std::string geometryShader;
    std::string fragmentShader;

    tinyxml2::XMLElement* programNodeAttribute = programNode->FirstChildElement("VertexShader");
    if (programNodeAttribute != nullptr) {
        if(programNodeAttribute->GetText() == nullptr) {
            std::cerr << "GLSL Program vertex shader has no text, this case is not handled!" << std::endl;
            return nullptr;
        } else {
            vertexShader = programNodeAttribute->GetText();
        }
    }

    programNodeAttribute = programNode->FirstChildElement("GeometryShader");
    if (programNodeAttribute != nullptr) {
        if(programNodeAttribute->GetText() == nullptr) {
            std::cout << "GLSL Program geometry shader has no text." << std::endl;
        } else {
            geometryShader = programNodeAttribute->GetText();
        }
    }

    programNodeAttribute = programNode->FirstChildElement("FragmentShader");
    if (programNodeAttribute != nullptr) {
        if(programNodeAttribute->GetText() == nullptr) {
            std::cerr << "GLSL Program vertex shader has no text, this case is not handled!" << std::endl;
            return nullptr;
        } else {
            fragmentShader = programNodeAttribute->GetText();
        }
    }

    bool materialRequired = false;
    programNodeAttribute = programNode->FirstChildElement("MaterialRequired");
    if (programNodeAttribute != nullptr) {
        if(programNodeAttribute->GetText() == nullptr) {
            std::cerr << "GLSL Program material required flag couldn't be read, assuming no!" << std::endl;
        } else {
            std::string materialRequiredString = programNodeAttribute->GetText();
            if(materialRequiredString == "True") {
                materialRequired = true;
            } else if(materialRequiredString == "False") {
                materialRequired = false;
            } else {
                std::cerr << "GLSL Program material required flag is unknown, assuming no!" << std::endl;
            }
        }
    } else {
        std::cerr << "GLSL Program material required flag not found, assuming no!" << std::endl;
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

bool GraphicsProgram::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode) {
    tinyxml2::XMLElement *programNode = document.NewElement("GraphicsProgram");
    parentNode->InsertEndChild(programNode);
    tinyxml2::XMLElement *currentElement = nullptr;

    currentElement = document.NewElement("VertexShader");
    currentElement->SetText(this->vertexShader.c_str());
    programNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("GeometryShader");
    currentElement->SetText(this->geometryShader.c_str());
    programNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("FragmentShader");
    currentElement->SetText(this->fragmentShader.c_str());
    programNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("MaterialRequired");
    if(materialRequired) {
        currentElement->SetText("True");
    } else {
        currentElement->SetText("False");
    }

    if(!presetUniformValues.empty()) {
        currentElement = document.NewElement("PresetValues");
        for(auto uniformEntry: presetUniformValues){
            tinyxml2::XMLElement *uniformNode = document.NewElement("Uniform");
            uniformNode->SetAttribute("Name", uniformEntry.first->name.c_str());
            uniformNode->SetText(uniformEntry.second.c_str());
        }
        programNode->InsertEndChild(currentElement);
    }
    programNode->InsertEndChild(currentElement);
    return true;
}