//
// Created by engin on 10.10.2020.
//

#ifndef LIMONENGINE_GRAPHICSPROGRAMASSET_H
#define LIMONENGINE_GRAPHICSPROGRAMASSET_H


#include <iostream>
#include <memory>
#include <unordered_map>
#include <cassert>
#include <API/Graphics/Uniform.h>
#include <API/Graphics/GraphicsInterface.h>
#include "Asset.h"


class GraphicsProgramAsset : public Asset {
    GraphicsInterface* graphicsInterface;
    std::string vertexShader;
    std::string geometryShader;
    std::string fragmentShader;
    std::string programName;
    bool initialized = false;

    std::unordered_map<std::string, std::shared_ptr<Uniform>> uniformMap;
    std::unordered_map<std::string, uint32_t> attributesMap;
    std::unordered_map<std::string, std::pair<Uniform::VariableTypes, GraphicsInterface::FrameBufferAttachPoints>>outputMap;
protected:
    void loadInternal() override {};//we don't need to load anything
public:
    GraphicsProgramAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList);
#ifdef CEREAL_SUPPORT
    GraphicsProgramAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList, cereal::BinaryInputArchive& binaryArchive) :
    Asset(assetManager, assetID, fileList, binaryArchive) {
        assert(false && "GraphicsProgramAsset doesn't support Cereal Loading");
    }
#endif
    ~GraphicsProgramAsset() override {
        for (auto uniformEntry:uniformMap) {
            uniformEntry.second = nullptr;
        }
    }



    void lateInitialize(uint32_t programId);

    uint32_t getAttributeLocation(const std::string& attributeName) {
        if(attributesMap.find(attributeName) != attributesMap.end()) {
            return attributesMap[attributeName];
        }
        return 0;
    }

    const std::unordered_map<std::string, std::shared_ptr<Uniform>> &getUniformMap() const {
        return uniformMap;
    }

    const std::unordered_map<std::string, std::pair<Uniform::VariableTypes, GraphicsInterface::FrameBufferAttachPoints>> &getOutputMap() const {
        return outputMap;
    }


    bool setUniform(const uint32_t programID, const std::string &uniformName, const glm::mat4 &matrix) {
        auto uniformIt = uniformMap.find(uniformName);
        if(uniformIt != uniformMap.end() && uniformIt->second->type == Uniform::VariableTypes::FLOAT_MAT4) {
            return graphicsInterface->setUniform(programID, uniformIt->second->location, matrix);
        }
        return false;
    }

    bool setUniform(const uint32_t programID, const std::string &uniformName, const glm::vec3 &vector) {
        auto uniformIt = uniformMap.find(uniformName);
        if(uniformIt != uniformMap.end() && uniformIt->second->type == Uniform::VariableTypes::FLOAT_VEC3) {
            return graphicsInterface->setUniform(programID, uniformIt->second->location, vector);
        }
        return false;
    }

    bool setUniform(const uint32_t programID, const std::string &uniformName, const std::vector<glm::vec3> &vectorArray) {
        auto uniformIt = uniformMap.find(uniformName);
        if(uniformIt != uniformMap.end() && uniformIt->second->type == Uniform::VariableTypes::FLOAT_VEC3) {
            return graphicsInterface->setUniform(programID, uniformIt->second->location, vectorArray);
        }
        return false;
    }

    bool setUniform(const uint32_t programID, const std::string &uniformName, const float value) {
        auto uniformIt = uniformMap.find(uniformName);
        if(uniformIt != uniformMap.end() && uniformIt->second->type == Uniform::VariableTypes::FLOAT) {
            return graphicsInterface->setUniform(programID, uniformIt->second->location, value);
        }
        return false;
    }
/**
 * This method is used to set samplers, so it can alter int uniforms, and sampler uniforms.
 * @param uniformName
 * @param value
 * @return
 */
    bool setUniform(const uint32_t programID, const std::string &uniformName, const int value) {
        auto uniformIt = uniformMap.find(uniformName);
        if(uniformIt != uniformMap.end() &&
            (uniformIt->second->type == Uniform::VariableTypes::BOOL ||
             uniformIt->second->type == Uniform::VariableTypes::INT ||
             uniformIt->second->type == Uniform::VariableTypes::CUBEMAP ||
             uniformIt->second->type == Uniform::VariableTypes::CUBEMAP_ARRAY ||
             uniformIt->second->type == Uniform::VariableTypes::TEXTURE_2D ||
             uniformIt->second->type == Uniform::VariableTypes::TEXTURE_2D_ARRAY))  {
            return graphicsInterface->setUniform(programID, uniformIt->second->location, value);
        }
        return false;
    }

    bool setUniformArray(const uint32_t programID, const std::string &uniformArrayName, const std::vector<glm::mat4> &matrix) {
        auto uniformIt = uniformMap.find(uniformArrayName);
        if(uniformIt != uniformMap.end() && uniformIt->second->type == Uniform::VariableTypes::FLOAT_MAT4) {
            //FIXME this should have a control of some sort
            return graphicsInterface->setUniformArray(programID, uniformIt->second->location, matrix);
        }
        return false;
    }

    const std::string &getVertexShaderFile() const {
        return vertexShader;
    }

    const std::string &getGeometryShaderFile() const {
        return geometryShader;
    }

    const std::string &getFragmentShaderFile() const {
        return fragmentShader;
    }

    const std::string &getProgramName() const {
        return programName;
    }
};


#endif //LIMONENGINE_GRAPHICSPROGRAMASSET_H
