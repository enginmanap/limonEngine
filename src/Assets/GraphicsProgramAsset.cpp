//
// Created by engin on 10.10.2020.
//

#include "GraphicsProgramAsset.h"
#include "AssetManager.h"
#include "API/Graphics/GraphicsProgram.h"

GraphicsProgramAsset::GraphicsProgramAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList) :
        Asset(assetManager, assetID, fileList) {
    this->graphicsInterface = assetManager->getGraphicsWrapper();
    if (fileList.size() < 2) {
        std::cerr << "Graphics Program load failed because file name vector does not have 2 elements." << std::endl;
        exit(-1);
    }
    vertexShader = fileList[0];
    if (fileList.size() == 2) {
        geometryShader = "";
        fragmentShader = fileList[1];
        programName = vertexShader +"|"+ fragmentShader;

    } else if (fileList.size() == 3) {
        geometryShader = fileList[1];
        fragmentShader = fileList[2];
        programName = vertexShader +"|"+ geometryShader +"|"+ fragmentShader;
    } else {
        std::cerr << "Graphics Program load failed because file name vector has more than 3 elements." << std::endl;
        exit(-1);
    }
}

void GraphicsProgramAsset::lateInitialize(uint32_t programId) {
    if(!initialized) {
        assetManager->getGraphicsWrapper()->initializeProgramAsset(programId, uniformMap, attributesMap, outputMap);
        initialized =true;
    }
}
