//
// Created by Engin Manap on 2.03.2016.
//

#ifndef UBERGAME_GLSLPROGRAM_H
#define UBERGAME_GLSLPROGRAM_H

#include <string>
#include <vector>
#include <map>
#include "GLHelper.h"


class GLSLProgram {
    GLHelper *glHelper;
    std::string vertexShader;
    std::string fragmentShader;
    std::map<std::string, GLHelper::Uniform*> uniformMap;

    GLuint programID;

public:
    GLSLProgram(GLHelper *glHelper, std::string vertexShader, std::string fragmentShader);


    GLuint getID() const { return programID; }

    bool setUniform(const std::string uniformName, const glm::mat4 &matrix) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == GLHelper::FLOAT_MAT4) {
            return glHelper->setUniform(programID, uniformMap[uniformName]->location, matrix);
        }
        return false;
    }

    bool setUniform(const std::string uniformName, const glm::vec3 &vector) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == GLHelper::FLOAT_VEC3) {
            return glHelper->setUniform(programID, uniformMap[uniformName]->location, vector);
        }
        return false;
    }

    bool setUniform(const std::string uniformName, const float value) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == GLHelper::FLOAT) {
            return glHelper->setUniform(programID, uniformMap[uniformName]->location, value);
        }
        return false;
    }

    bool setUniform(const std::string uniformName, const int value) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == GLHelper::INT) {
            return glHelper->setUniform(programID, uniformMap[uniformName]->location, value);
        }
        return false;
    }

};

#endif //UBERGAME_GLSLPROGRAM_H
