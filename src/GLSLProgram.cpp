//
// Created by Engin Manap on 2.03.2016.
//

#include "GLSLProgram.h"

GLSLProgram::GLSLProgram(GLHelper *glHelper, std::string vertexShader, std::string fragmentShader,
                         std::vector<std::string> uniforms) :
        glHelper(glHelper), vertexShader(vertexShader), fragmentShader(fragmentShader), uniforms(uniforms) {

    programID = glHelper->initializeProgram(vertexShader, fragmentShader);
    GLuint location;
    for (std::vector<std::string>::iterator i = uniforms.begin(); i != uniforms.end(); i++) {
        if (glHelper->getUniformLocation(programID, *i, location)) {
            uniformLocations[*i] = location;
        } else {
            std::cerr << "Error in program " << vertexShader << ", " << fragmentShader << std::endl;
            std::cerr << "Uniform " << *i << " can not be found in program. " << std::endl;
        }
    }
}