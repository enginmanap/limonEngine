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
    std::vector<std::string> uniforms;

    GLuint programID;
    std::map<std::string, GLuint> uniformLocations;

public:
    GLSLProgram(GLHelper *glHelper, std::string vertexShader, std::string fragmentShader,
                std::vector<std::string> uniforms);


    GLuint getID() const { return programID; }

    //FIXME is this needed for anything?
    /*
    bool getUniformLocation(const std::string uniformName, GLuint &location) {
        if (uniformLocations.count(uniformName)) {
            location = uniformLocations[uniformName];
            return true;
        } else {
            std::cerr << "requested uniform[" << uniformName << "] doesn't have a location." << std::endl;
            return false;
        }
    }
    */

    bool setUniform(const std::string uniformName, const glm::mat4 &matrix) {
        if (uniformLocations.count(uniformName)) {
            return glHelper->setUniform(programID, uniformLocations[uniformName], matrix);
        }
        return false;
    }

    bool setUniform(const std::string uniformName, const glm::vec3 &vector) {
        if (uniformLocations.count(uniformName)) {
            return glHelper->setUniform(programID, uniformLocations[uniformName], vector);
        }
        return false;
    }

    bool setUniform(const std::string uniformName, const float value) {
        if (uniformLocations.count(uniformName)) {
            return glHelper->setUniform(programID, uniformLocations[uniformName], value);
        }
        return false;
    }

};

#endif //UBERGAME_GLSLPROGRAM_H
