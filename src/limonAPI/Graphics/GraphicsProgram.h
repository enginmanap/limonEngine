//
// Created by Engin Manap on 2.03.2016.
//

#ifndef LIMONENGINE_GRAPHICSPROGRAM_H
#define LIMONENGINE_GRAPHICSPROGRAM_H

#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <Assets/GraphicsProgramAsset.h>
#include "limonAPI/Graphics/GraphicsInterface.h"
#include "limonAPI/Graphics/Uniform.h"

class GraphicsProgramLoader;

class GraphicsProgram {
    friend class GraphicsProgramLoader;
    AssetManager* assetManager;
    GraphicsInterface* graphicsWrapper;
    std::shared_ptr<GraphicsProgramAsset> graphicsProgramAsset;
    std::unordered_map<std::shared_ptr<Uniform>, std::string> presetUniformValues;
    std::unordered_map<std::string, int32_t> textureUnits;//filled by GraphicsPipeline::assignTextureUnits, for samplers the render method binds itself
    bool materialRequired;
    bool modelBoneTransformUsed;
    bool shadowDirectionalUsed;
    bool shadowPointUsed;
    uint32_t programID;
    std::string vertexShaderContent, geometryShaderContent, fragmentShaderContent;

    void setMaterialRequired();
    //detects, from the reflected uniform map, whether this program reads the model/bone transform textures or shadow maps
    void detectReservedTextureUnitUsage();

public:
    GraphicsProgram(AssetManager* assetManager, const std::string& vertexShader, const std::string& fragmentShader);
    GraphicsProgram(AssetManager* assetManager, const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader);

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

    bool isMaterialRequired() const {
        return materialRequired;
    }

    //Since model transform matrix and bone transform matrix are used together, this flag means both
    bool isModelBoneTransformUsed() const {
        return modelBoneTransformUsed;
    }

    bool isShadowDirectionalUsed() const {
        return shadowDirectionalUsed;
    }

    bool isShadowPointUsed() const {
        return shadowPointUsed;
    }

    const std::string &getVertexShaderFile() const {
        return graphicsProgramAsset->getVertexShaderFile();
    }

    const std::string &getGeometryShaderFile() const {
        return graphicsProgramAsset->getGeometryShaderFile();
    }

    const std::string &getFragmentShaderFile() const {
        return graphicsProgramAsset->getFragmentShaderFile();
    }

    void setFragmentShaderContent(const std::string &fragmentShaderContent) {
        GraphicsProgram::fragmentShaderContent = fragmentShaderContent;
    }

    void setGeometryShaderContent(const std::string &geometryShaderContent) {
        GraphicsProgram::geometryShaderContent = geometryShaderContent;
    }

    void setVertexShaderContent(const std::string &vertexShaderContent) {
        GraphicsProgram::vertexShaderContent = vertexShaderContent;
    }

    bool addPresetValue(const std::string& uniformName, const std::string& value);

    void setTextureUnit(const std::string& samplerName, int32_t textureUnit) {
        textureUnits[samplerName] = textureUnit;
    }

    //-1 if the pipeline didn't assign one, a program used outside a pipeline has none
    int32_t getTextureUnit(const std::string& samplerName) const {
        std::unordered_map<std::string, int32_t>::const_iterator unitIterator = textureUnits.find(samplerName);
        if (unitIterator == textureUnits.end()) {
            std::cerr << "No texture unit assigned for sampler " << samplerName << " of program " << getProgramName() << ", it won't be bound." << std::endl;
            return -1;
        }
        return unitIterator->second;
    }

};

#endif //LIMONENGINE_GRAPHICSPROGRAM_H
