//
// Created by Engin Manap on 10.02.2016.
//

#ifndef UBERGAME_GLHELPER_H
#define UBERGAME_GLHELPER_H

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <GL/glew.h>
#include <GL/gl.h>

#include <string>
#include <algorithm>
#include <vector>

#include <fstream>
#include <streambuf>
#include <iostream>

class GLHelper {
    GLuint gpuProgram;
    GLuint vao;
    GLuint vbo;
    GLuint ebo;

    GLint modelMatrixLocation;
    GLint projectionMatrixLocation;

    float aspect;

    glm::mat4 model_matrix;
public:
    GLHelper();
    ~GLHelper();

    GLuint createShader(GLenum, const std::string &);
    GLuint createProgram(const std::vector<GLuint> &);
    GLuint initializeProgram();

    void render();
    void reshape(int width, int height);
};

#endif //UBERGAME_GLHELPER_H
