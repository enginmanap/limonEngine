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

    enum VariableTypes {
    FLOAT,
    FLOAT_VEC2,
    FLOAT_VEC3,
    FLOAT_VEC4,
    FLOAT_MAT4,
    UNDEFINED };

    class Uniform{
    public:
        unsigned int location;
        std::string name;
        VariableTypes type;
        unsigned int size;

        Uniform(unsigned int location, const std::string &name, VariableTypes type, unsigned int size) : location(location), name(name), type(type), size(size) { }
    };

    GLHelper *glHelper;
    std::string vertexShader;
    std::string fragmentShader;
    std::map<std::string, Uniform*> uniformMap;

    GLuint programID;

public:
    GLSLProgram(GLHelper *glHelper, std::string vertexShader, std::string fragmentShader);


    GLuint getID() const { return programID; }

    bool setUniform(const std::string uniformName, const glm::mat4 &matrix) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == FLOAT_MAT4) {
            return glHelper->setUniform(programID, uniformMap[uniformName]->location, matrix);
        }
        return false;
    }

    bool setUniform(const std::string uniformName, const glm::vec3 &vector) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == FLOAT_VEC3) {
            return glHelper->setUniform(programID, uniformMap[uniformName]->location, vector);
        }
        return false;
    }

    bool setUniform(const std::string uniformName, const float value) {
        if (uniformMap.count(uniformName) && uniformMap[uniformName]->type == FLOAT) {
            return glHelper->setUniform(programID, uniformMap[uniformName]->location, value);
        }
        return false;
    }

};

#endif //UBERGAME_GLSLPROGRAM_H
