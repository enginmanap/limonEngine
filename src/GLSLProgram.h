//
// Created by Engin Manap on 2.03.2016.
//

#ifndef LIMONENGINE_GLSLPROGRAM_H
#define LIMONENGINE_GLSLPROGRAM_H

#include <string>
#include <vector>
#include <unordered_map>
#include "GLHelper.h"


class GLSLProgram {
    GLHelper *glHelper;
    std::string programName;

    std::string vertexShader;
    std::string fragmentShader;
    std::unordered_map<std::string, GLHelper::Uniform*> uniformMap;
    std::unordered_map<std::string, GLHelper::VariableTypes>outputMap;
    bool materialRequired;
    GLuint programID;

    GLSLProgram(GLHelper *glHelper, std::string vertexShader, std::string fragmentShader, bool isMaterialUsed);
    GLSLProgram(GLHelper *glHelper, std::string vertexShader, std::string geometryShader, std::string fragmentShader, bool isMaterialUsed);

public:

    ~GLSLProgram();

    friend std::shared_ptr<GLSLProgram> GLHelper::createGLSLProgram(const std::string &vertexShader, const std::string &fragmentShader, bool isMaterialUsed);
    friend std::shared_ptr<GLSLProgram> GLHelper::createGLSLProgram(const std::string &vertexShader, const std::string &geometryShader, const std::string &fragmentShader, bool isMaterialUsed);

    GLuint getID() const { return programID; }

    std::unordered_map<std::string, GLHelper::Uniform const * const> getUniformMap() const {
        std::unordered_map<std::string, GLHelper::Uniform const * const> constUniformMap;
        for(auto element:uniformMap) {
            constUniformMap.insert(element);
        }
        return constUniformMap;
    }

    const std::unordered_map<std::string, GLHelper::VariableTypes> &getOutputMap() const {
        return outputMap;
    }

    bool setUniform(const std::string &uniformName, const glm::mat4 &matrix) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == GLHelper::FLOAT_MAT4) {
            return glHelper->setUniform(programID, uniformMap[uniformName]->location, matrix);
        }
        return false;
    }

    bool setUniform(const std::string &uniformName, const glm::vec3 &vector) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == GLHelper::FLOAT_VEC3) {
            return glHelper->setUniform(programID, uniformMap[uniformName]->location, vector);
        }
        return false;
    }

    bool setUniform(const std::string &uniformName, const std::vector<glm::vec3> &vectorArray) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == GLHelper::FLOAT_VEC3) {
            return glHelper->setUniform(programID, uniformMap[uniformName]->location, vectorArray);
        }
        return false;
    }

    bool setUniform(const std::string &uniformName, const float value) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == GLHelper::FLOAT) {
            return glHelper->setUniform(programID, uniformMap[uniformName]->location, value);
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
                    (uniformMap[uniformName]->type == GLHelper::INT ||
                        uniformMap[uniformName]->type == GLHelper::CUBEMAP ||
                        uniformMap[uniformName]->type == GLHelper::CUBEMAP_ARRAY ||
                        uniformMap[uniformName]->type == GLHelper::TEXTURE_2D ||
                        uniformMap[uniformName]->type == GLHelper::TEXTURE_2D_ARRAY))  {
            return glHelper->setUniform(programID, uniformMap[uniformName]->location, value);
        }
        return false;
    }

    bool setUniformArray(const std::string &uniformArrayName, const std::vector<glm::mat4> &matrix) {
        if (uniformMap.count(uniformArrayName) && uniformMap[uniformArrayName]->type == GLHelper::FLOAT_MAT4) {
            //FIXME this should have a control of some sort
            return glHelper->setUniformArray(programID, uniformMap[uniformArrayName]->location, matrix);
        }
        return false;
    }

    const std::string &getProgramName() const {
        return programName;
    }

    bool IsMaterialRequired() const {
        return materialRequired;
    }

};



#endif //LIMONENGINE_GLSLPROGRAM_H
