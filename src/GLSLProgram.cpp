//
// Created by Engin Manap on 2.03.2016.
//

#include "GLSLProgram.h"

GLSLProgram::GLSLProgram(GLHelper *glHelper, std::string vertexShader, std::string fragmentShader, bool isMaterialUsed) :
        glHelper(glHelper), vertexShader(vertexShader), fragmentShader(fragmentShader), materialRequired(isMaterialUsed) {
    programName = vertexShader + fragmentShader;
    programID = glHelper->initializeProgram(vertexShader, fragmentShader, uniformMap);

}
