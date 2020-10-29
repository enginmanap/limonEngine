//
// Created by Engin Manap on 2.03.2016.
//

#ifndef LIMONENGINE_GRAPHICSPROGRAM_H
#define LIMONENGINE_GRAPHICSPROGRAM_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <Assets/GraphicsProgramAsset.h>
#include "API/Graphics/GraphicsInterface.h"
#include "API/Graphics/Uniform.h"

class GraphicsProgramLoader;

class GraphicsProgram {
    friend class GraphicsProgramLoader;
    AssetManager* assetManager;
    GraphicsInterface* graphicsWrapper;
    GraphicsProgramAsset* graphicsProgramAsset;
    std::unordered_map<std::shared_ptr<Uniform>, std::string> presetUniformValues;
    bool materialRequired;
    uint32_t programID;

    //TODO remove with material editor
    void setSamplersAndUBOs();

public:

    GraphicsProgram(AssetManager* assetManager, const std::string& vertexShader, const std::string& fragmentShader, bool isMaterialUsed);
    GraphicsProgram(AssetManager* assetManager, const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader, bool isMaterialUsed);

    uint32_t getAttributeLocation(const std::string& attributeName) {
        return graphicsProgramAsset->getAttributeLocation(attributeName);
    }

    ~GraphicsProgram();

    uint32_t getID() const { return programID; }

    const std::unordered_map<std::string, std::shared_ptr<Uniform>> &getUniformMap() const {
        return graphicsProgramAsset->getUniformMap();
    }

    const std::unordered_map<std::string, std::pair<Uniform::VariableTypes, GraphicsInterface::FrameBufferAttachPoints>> &getOutputMap() const {
        return graphicsProgramAsset->getOutputMap();
    }

    bool setUniform(const std::string &uniformName, const glm::mat4 &matrix) {
        return graphicsProgramAsset->setUniform(this->programID, uniformName, matrix);
    }

    bool setUniform(const std::string &uniformName, const glm::vec3 &vector) {
        return graphicsProgramAsset->setUniform(this->programID, uniformName, vector);
    }

    bool setUniform(const std::string &uniformName, const std::vector<glm::vec3> &vectorArray) {
        return graphicsProgramAsset->setUniform(this->programID, uniformName, vectorArray);
    }

    bool setUniform(const std::string &uniformName, const float value) {
        return graphicsProgramAsset->setUniform(this->programID, uniformName, value);
    }
/**
 * This method is used to set samplers, so it can alter int uniforms, and sampler uniforms.
 * @param uniformName
 * @param value
 * @return
 */
    bool setUniform(const std::string &uniformName, const int value) {
        return graphicsProgramAsset->setUniform(this->programID, uniformName, value);
    }

    bool setUniformArray(const std::string &uniformArrayName, const std::vector<glm::mat4> &matrix) {
        return graphicsProgramAsset->setUniformArray(this->programID, uniformArrayName, matrix);
    }

    const std::string &getProgramName() const {
        return graphicsProgramAsset->getProgramName();
    }

    bool IsMaterialRequired() const {
        return materialRequired;
    }

    const std::string &getVertexShader() const {
        return graphicsProgramAsset->getVertexShader();
    }

    const std::string &getGeometryShader() const {
        return graphicsProgramAsset->getGeometryShader();
    }

    const std::string &getFragmentShader() const {
        return graphicsProgramAsset->getFragmentShader();
    }

    bool addPresetValue(const std::string& uniformName, const std::string& value);

};

#endif //LIMONENGINE_GRAPHICSPROGRAM_H
