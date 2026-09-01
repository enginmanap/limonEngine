//
// Created by Engin Manap on 2.03.2016.
//

#include "GraphicsProgram.h"
#include "Assets/AssetManager.h"
#include "GraphicsProgramPreprocessor.h"

GraphicsProgram::GraphicsProgram(AssetManager* assetManager, const std::string& vertexShader, const std::string& fragmentShader) :
        assetManager(assetManager), graphicsWrapper(assetManager->getGraphicsWrapper()), materialRequired(false),
        modelBoneTransformUsed(false), shadowDirectionalUsed(false), shadowPointUsed(false) {
    graphicsProgramAsset = assetManager->loadAsset<GraphicsProgramAsset>({vertexShader, fragmentShader});
    GraphicsProgramPreprocessor::preprocess(this, assetManager->getGraphicsWrapper()->getShaderHeader(), assetManager->getGraphicsWrapper()->getOptions()->getAllOptions());
    programID = graphicsWrapper->createGraphicsProgram(vertexShaderContent, graphicsProgramAsset->getVertexShaderFile(), "", "", fragmentShaderContent, graphicsProgramAsset->getFragmentShaderFile());
    graphicsProgramAsset->lateInitialize(programID);
    this->setMaterialRequired();
    this->detectReservedTextureUnitUsage();
    graphicsWrapper->attachModelTexture(getID());
    graphicsWrapper->attachRigTexture(getID());
    graphicsWrapper->attachModelIndicesUBO(getID());
}

GraphicsProgram::GraphicsProgram(AssetManager* assetManager, const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader) :
        assetManager(assetManager), graphicsWrapper(assetManager->getGraphicsWrapper()), materialRequired(false),
        modelBoneTransformUsed(false), shadowDirectionalUsed(false), shadowPointUsed(false) {
    graphicsProgramAsset = assetManager->loadAsset<GraphicsProgramAsset>({vertexShader, geometryShader, fragmentShader});

    GraphicsProgramPreprocessor::preprocess(this, assetManager->getGraphicsWrapper()->getShaderHeader(), assetManager->getGraphicsWrapper()->getOptions()->getAllOptions());

    programID = graphicsWrapper->createGraphicsProgram(vertexShaderContent, graphicsProgramAsset->getVertexShaderFile(), geometryShaderContent, graphicsProgramAsset->getGeometryShaderFile(), fragmentShaderContent, graphicsProgramAsset->getFragmentShaderFile());
    graphicsProgramAsset->lateInitialize(programID);
    this->setMaterialRequired();
    this->detectReservedTextureUnitUsage();
    graphicsWrapper->attachModelTexture(getID());
    graphicsWrapper->attachRigTexture(getID());
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


void GraphicsProgram::setMaterialRequired() {
    const auto& uniformMap = graphicsProgramAsset->getUniformMap();
    for (const auto& pair : uniformMap) {
        if (pair.first.rfind("MaterialInformationBlock", 0) == 0) { // We use MaterialInformationBlock name for the ubo
            materialRequired = true;
            break;
        }
    }
    if (materialRequired) {
        graphicsWrapper->attachMaterialUBO(getID());
    }
}

void GraphicsProgram::detectReservedTextureUnitUsage() {
    const auto& uniformMap = graphicsProgramAsset->getUniformMap();
    //allModelTransformsTexture and allBoneTransformsTexture are declared together only (both live
    //solely in the shared ModelRendering.vert header), so either one found implies both are present.
    modelBoneTransformUsed = uniformMap.find("allModelTransformsTexture") != uniformMap.end()
                              || uniformMap.find("allBoneTransformsTexture") != uniformMap.end();
    shadowDirectionalUsed = uniformMap.find("pre_shadowDirectional") != uniformMap.end();
    shadowPointUsed = uniformMap.find("pre_shadowPoint") != uniformMap.end();
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
