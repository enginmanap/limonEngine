//
// Created by Engin Manap on 2.03.2016.
//

#include "GraphicsProgram.h"
#include "Assets/AssetManager.h"
#include "GraphicsProgramPreprocessor.h"

GraphicsProgram::GraphicsProgram(AssetManager* assetManager, const std::string& vertexShader, const std::string& fragmentShader, bool isMaterialUsed) :
        assetManager(assetManager), graphicsWrapper(assetManager->getGraphicsWrapper()), materialRequired(isMaterialUsed) {
    graphicsProgramAsset = assetManager->loadAsset<GraphicsProgramAsset>({vertexShader, fragmentShader});
    GraphicsProgramPreprocessor::preprocess(this, assetManager->getGraphicsWrapper()->getContextInformation().shaderHeader, assetManager->getGraphicsWrapper()->getOptions()->getAllOptions());
    programID = graphicsWrapper->createGraphicsProgram(vertexShaderContent, "", fragmentShaderContent);
    graphicsProgramAsset->lateInitialize(programID);
    if(materialRequired) {
        setSamplersAndUBOs();
    }
    graphicsWrapper->attachModelUBO(getID());
    graphicsWrapper->attachModelIndicesUBO(getID());
}

GraphicsProgram::GraphicsProgram(AssetManager* assetManager, const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader, bool isMaterialUsed) :
        assetManager(assetManager), graphicsWrapper(assetManager->getGraphicsWrapper()), materialRequired(isMaterialUsed) {
    graphicsProgramAsset = assetManager->loadAsset<GraphicsProgramAsset>({vertexShader, geometryShader, fragmentShader});
    GraphicsProgramPreprocessor::preprocess(this, assetManager->getGraphicsWrapper()->getContextInformation().shaderHeader, assetManager->getGraphicsWrapper()->getOptions()->getAllOptions());

    programID = graphicsWrapper->createGraphicsProgram(vertexShaderContent, geometryShaderContent, fragmentShaderContent);
    graphicsProgramAsset->lateInitialize(programID);
    if(materialRequired) {
        setSamplersAndUBOs();
    }
    graphicsWrapper->attachModelUBO(getID());
    graphicsWrapper->attachModelIndicesUBO(getID());
}

GraphicsProgram::~GraphicsProgram() {
    if(graphicsProgramAsset->getGeometryShaderFile().empty()) {
        assetManager->freeAsset({graphicsProgramAsset->getVertexShaderFile(), graphicsProgramAsset->getFragmentShaderFile()});
    } else {
        assetManager->freeAsset({graphicsProgramAsset->getVertexShaderFile(), graphicsProgramAsset->getGeometryShaderFile(), graphicsProgramAsset->getFragmentShaderFile()});
    }

    graphicsWrapper->destroyProgram(programID);
}


//TODO remove with material editor
void GraphicsProgram::setSamplersAndUBOs() {
    graphicsWrapper->attachMaterialUBO(getID());
    graphicsWrapper->attachBoneTransformsUBO(getID());

    //TODO these will be configurable with material editor
    int diffuseMapAttachPoint = 1;
    int ambientMapAttachPoint = 2;
    int specularMapAttachPoint = 3;
    int opacityMapAttachPoint = 4;
    int normalMapAttachPoint = 5;

    if (!setUniform("diffuseSampler", diffuseMapAttachPoint)) {
        std::cerr << "Uniform \"diffuseSampler\" could not be set for " << this->getProgramName() << std::endl;
    }
    if (!setUniform("ambientSampler", ambientMapAttachPoint)) {
        std::cerr << "Uniform \"ambientSampler\" could not be set for " << this->getProgramName() << std::endl;
    }
    if (!setUniform("specularSampler", specularMapAttachPoint)) {
        std::cerr << "Uniform \"specularSampler\" could not be set for " << this->getProgramName() << std::endl;
    }
    auto uniformMap = graphicsProgramAsset->getUniformMap();
    if(uniformMap.find("opacitySampler") != uniformMap.end()) {
        if (!setUniform("opacitySampler", opacityMapAttachPoint)) {
            std::cerr << "Uniform \"opacitySampler\" could not be set for " << this->getProgramName() << std::endl;
        }    }
    if (!setUniform("normalSampler", normalMapAttachPoint)) {
        std::cerr << "Uniform \"normalSampler\" could not be set for " << this->getProgramName() << std::endl;
    }
    //TODO we should support multi texture on one pass

    if (!setUniform("pre_shadowDirectional", graphicsWrapper->getMaxTextureImageUnits() - 1)) {
        std::cerr << "Uniform \"pre_shadowDirectional\" could not be set for " << this->getProgramName() << std::endl;
    }
    if (!setUniform("pre_shadowPoint", graphicsWrapper->getMaxTextureImageUnits() - 2)) {
        std::cerr << "Uniform \"pre_shadowPoint\" could not be set for " << this->getProgramName() << std::endl;
    }
}

bool GraphicsProgram::addPresetValue(const std::string& uniformName, const std::string& value) {
    auto processingUniformIt = getUniformMap().find(uniformName);
    if(processingUniformIt == getUniformMap().end()) {
        return false;
    }
    this->presetUniformValues[processingUniformIt->second] = value;
    switch (processingUniformIt->second->type) {
        case Uniform::VariableTypes::BOOL:
        case Uniform::VariableTypes::INT:
        case Uniform::VariableTypes::CUBEMAP:
        case Uniform::VariableTypes::CUBEMAP_ARRAY:
        case Uniform::VariableTypes::TEXTURE_2D:
        case Uniform::VariableTypes::TEXTURE_2D_ARRAY:
            setUniform(uniformName, std::stoi(value));
            return true;
        case Uniform::VariableTypes::FLOAT:
            setUniform(uniformName, std::stof(value));
            return true;
        case Uniform::VariableTypes::FLOAT_VEC2:
        case Uniform::VariableTypes::FLOAT_VEC3:
        case Uniform::VariableTypes::FLOAT_VEC4:
        case Uniform::VariableTypes::FLOAT_MAT4:
        case Uniform::VariableTypes::UNDEFINED:
        default:
            std::cerr << "Deserializing the given type is not implemented! name: " << uniformName << ", value: " << value << std::endl;
            return false;
    }

}
