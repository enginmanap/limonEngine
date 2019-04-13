//
// Created by Engin Manap on 2.03.2016.
//

#include "GLSLProgram.h"

GLSLProgram::GLSLProgram(GLHelper *glHelper, std::string vertexShader, std::string fragmentShader, bool isMaterialUsed) :
        glHelper(glHelper), vertexShader(vertexShader), fragmentShader(fragmentShader), materialRequired(isMaterialUsed) {
    programName = vertexShader + fragmentShader;
    //FIXME is passing empty string acceptable?
    programID = glHelper->initializeProgram(vertexShader, "", fragmentShader, uniformMap, outputMap);
}

GLSLProgram::GLSLProgram(GLHelper *glHelper, std::string vertexShader, std::string geometryShader, std::string fragmentShader, bool isMaterialUsed) :
        glHelper(glHelper), vertexShader(vertexShader), fragmentShader(fragmentShader), materialRequired(isMaterialUsed) {
    programName = vertexShader + geometryShader + fragmentShader;
    programID = glHelper->initializeProgram(vertexShader, geometryShader, fragmentShader, uniformMap, outputMap);
}

GLSLProgram::~GLSLProgram() {
    glHelper->destroyProgram(programID);
}