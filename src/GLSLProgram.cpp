//
// Created by Engin Manap on 2.03.2016.
//

#include "GLSLProgram.h"

GLSLProgram::GLSLProgram(GLHelper *glHelper, std::string vertexShader, std::string fragmentShader) :
        glHelper(glHelper), vertexShader(vertexShader), fragmentShader(fragmentShader) {

    programID = glHelper->initializeProgram(vertexShader, fragmentShader);


    GLint i;
    GLint count;

    GLint size; // size of the variable
    GLenum type; // type of the variable (float, vec3 or mat4, etc)

    GLint maxLength;

    glGetProgramiv(programID, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);

    GLchar name[maxLength]; // variable name in GLSL
    GLsizei length; // name length

    glGetProgramiv(programID, GL_ACTIVE_UNIFORMS, &count);
    printf("Active Uniforms: %d\n", count);

    VariableTypes variableType = UNDEFINED;
    for (i = 0; i < count; i++)
    {
        glGetActiveUniform(programID, (GLuint)i, maxLength, &length, &size, &type, name);

        //std::cout << "Uniform "<< i << " Type: " << type << " Name: " << name << std::endl;

        switch (type){
            case GL_FLOAT: variableType = FLOAT;
                break;
            case GL_FLOAT_VEC2: variableType = FLOAT_VEC2;
                break;
            case GL_FLOAT_VEC3: variableType = FLOAT_VEC3;
                break;
            case GL_FLOAT_VEC4: variableType = FLOAT_VEC4;
                break;
            case GL_FLOAT_MAT4: variableType = FLOAT_MAT4;
                break;
            default:
                variableType = UNDEFINED;
        }
        uniformMap[name] = new Uniform(i,name,variableType,size);


    }

}