//
// Created by Engin Manap on 2.03.2016.
//

#ifndef LIMONENGINE_GRAPHICSPROGRAM_H
#define LIMONENGINE_GRAPHICSPROGRAM_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "API/Graphics/GraphicsInterface.h"
#include "API/Graphics/Uniform.h"

class GraphicsProgramLoader;

class GraphicsProgram {
    friend class GraphicsProgramLoader;
    GraphicsInterface* graphicsWrapper;
    std::string vertexShader;
    std::string geometryShader;
    std::string fragmentShader;
    std::string programName;

    std::unordered_map<std::string, const Uniform *> uniformMap;
    std::unordered_map<std::string, uint32_t> attributesMap;
    std::unordered_map<std::string, std::pair<Uniform::VariableTypes, GraphicsInterface::FrameBufferAttachPoints>>outputMap;
    std::unordered_map<const Uniform *, std::string> presetUniformValues;
    bool materialRequired;
    uint32_t programID;

    GraphicsProgram(GraphicsInterface* graphicsWrapper, std::string vertexShader, std::string fragmentShader, bool isMaterialUsed);
    GraphicsProgram(GraphicsInterface* graphicsWrapper, std::string vertexShader, std::string geometryShader, std::string fragmentShader, bool isMaterialUsed);

    //TODO remove with material editor
    void setSamplersAndUBOs();

public:

    uint32_t getAttributeLocation(const std::string& attributeName) {
        if(attributesMap.find(attributeName) != attributesMap.end()) {
            return attributesMap[attributeName];
        }
        return 0;
    }

    ~GraphicsProgram();

    friend std::shared_ptr<GraphicsProgram> GraphicsInterface::createGraphicsProgramInternal(const std::string &vertexShader, const std::string &fragmentShader, bool isMaterialUsed, std::function<void(GraphicsProgram*)> deleterMethod);
    friend std::shared_ptr<GraphicsProgram> GraphicsInterface::createGraphicsProgramInternal(const std::string &vertexShader, const std::string &geometryShader, const std::string &fragmentShader, bool isMaterialUsed, std::function<void(GraphicsProgram*)> deleterMethod);

    uint32_t getID() const { return programID; }

    const std::unordered_map<std::string, const Uniform *> &getUniformMap() const {
        return uniformMap;
    }

    const std::unordered_map<std::string, std::pair<Uniform::VariableTypes, GraphicsInterface::FrameBufferAttachPoints>> &getOutputMap() const {
        return outputMap;
    }

    bool setUniform(const std::string &uniformName, const glm::mat4 &matrix) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == Uniform::VariableTypes::FLOAT_MAT4) {
            return graphicsWrapper->setUniform(programID, uniformMap[uniformName]->location, matrix);
        }
        return false;
    }

    bool setUniform(const std::string &uniformName, const glm::vec3 &vector) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == Uniform::VariableTypes::FLOAT_VEC3) {
            return graphicsWrapper->setUniform(programID, uniformMap[uniformName]->location, vector);
        }
        return false;
    }

    bool setUniform(const std::string &uniformName, const std::vector<glm::vec3> &vectorArray) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == Uniform::VariableTypes::FLOAT_VEC3) {
            return graphicsWrapper->setUniform(programID, uniformMap[uniformName]->location, vectorArray);
        }
        return false;
    }

    bool setUniform(const std::string &uniformName, const float value) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == Uniform::VariableTypes::FLOAT) {
            return graphicsWrapper->setUniform(programID, uniformMap[uniformName]->location, value);
        }
        return false;
    }
/**
 * This method is used to set samplers, so it can alter int uniforms, and sampler uniforms.
 * @param uniformName
 * @param value
 * @return
 */
    bool setUniform(const std::string &uniformName, const int value) {
        if (uniformMap.count(uniformName) &&
                    (uniformMap[uniformName]->type == Uniform::VariableTypes::INT ||
                     uniformMap[uniformName]->type == Uniform::VariableTypes::CUBEMAP ||
                     uniformMap[uniformName]->type == Uniform::VariableTypes::CUBEMAP_ARRAY ||
                     uniformMap[uniformName]->type == Uniform::VariableTypes::TEXTURE_2D ||
                     uniformMap[uniformName]->type == Uniform::VariableTypes::TEXTURE_2D_ARRAY))  {
            return graphicsWrapper->setUniform(programID, uniformMap[uniformName]->location, value);
        }
        return false;
    }

    bool setUniformArray(const std::string &uniformArrayName, const std::vector<glm::mat4> &matrix) {
        if (uniformMap.count(uniformArrayName) && uniformMap[uniformArrayName]->type == Uniform::VariableTypes::FLOAT_MAT4) {
            //FIXME this should have a control of some sort
            return graphicsWrapper->setUniformArray(programID, uniformMap[uniformArrayName]->location, matrix);
        }
        return false;
    }

    const std::string &getProgramName() const {
        return programName;
    }

    bool IsMaterialRequired() const {
        return materialRequired;
    }

    const std::string &getVertexShader() const {
        return vertexShader;
    }

    const std::string &getGeometryShader() const {
        return geometryShader;
    }

    const std::string &getFragmentShader() const {
        return fragmentShader;
    }

};

#endif //LIMONENGINE_GRAPHICSPROGRAM_H
