//
// Created by Engin Manap on 2.03.2016.
//

#include "GraphicsProgram.h"
#include "Assets/AssetManager.h"

GraphicsProgram::GraphicsProgram(AssetManager* assetManager, const std::string& vertexShader, const std::string& fragmentShader, bool isMaterialUsed) :
        assetManager(assetManager), graphicsWrapper(assetManager->getGraphicsWrapper()), materialRequired(isMaterialUsed) {
    graphicsProgramAsset = assetManager->loadAsset<GraphicsProgramAsset>({vertexShader, fragmentShader});
    programID = graphicsWrapper->createGraphicsProgram(vertexShader, "", fragmentShader);
    graphicsProgramAsset->lateInitialize(programID);
}

GraphicsProgram::GraphicsProgram(AssetManager* assetManager, const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader, bool isMaterialUsed) :
        assetManager(assetManager), graphicsWrapper(assetManager->getGraphicsWrapper()), materialRequired(isMaterialUsed) {
    graphicsProgramAsset = assetManager->loadAsset<GraphicsProgramAsset>({vertexShader, geometryShader, fragmentShader});
    programID = graphicsWrapper->createGraphicsProgram(vertexShader, geometryShader, fragmentShader);
    graphicsProgramAsset->lateInitialize(programID);

}

GraphicsProgram::~GraphicsProgram() {
    if(graphicsProgramAsset->getGeometryShader().empty()) {
        assetManager->freeAsset({graphicsProgramAsset->getVertexShader(), graphicsProgramAsset->getFragmentShader()});
    } else {
        assetManager->freeAsset({graphicsProgramAsset->getVertexShader(), graphicsProgramAsset->getGeometryShader(), graphicsProgramAsset->getFragmentShader()});
    }

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
    auto uniformMap = graphicsProgramAsset->getUniformMap();
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