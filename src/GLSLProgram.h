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
    GLHelper* glHelper;
    std::string vertexShader;
    std::string fragmentShader;
    std::vector<std::string> uniforms;

    GLuint programID;
    std::map<std::string, GLuint> uniformLocations;

public:
    GLSLProgram(GLHelper* glHelper, std::string vertexShader, std::string fragmentShader, std::vector<std::string> uniforms);


    GLuint getID() const {return programID;}
    bool getUniformLocation(const std::string uniformName, GLuint& location) {
        if (uniformLocations.count(uniformName)) {
            location = uniformLocations[uniformName];
            return true;
        } else {
            std::cerr << "requested uniform[" << uniformName << "] doesn't have a location." << std::endl;
            return false;
        }
    }

};
#endif //UBERGAME_GLSLPROGRAM_H
