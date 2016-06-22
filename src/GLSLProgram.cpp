//
// Created by Engin Manap on 2.03.2016.
//

#include "GLSLProgram.h"

GLSLProgram::GLSLProgram(GLHelper *glHelper, std::string vertexShader, std::string fragmentShader) :
        glHelper(glHelper), vertexShader(vertexShader), fragmentShader(fragmentShader) {

    programID = glHelper->initializeProgram(vertexShader, fragmentShader, uniformMap);

}