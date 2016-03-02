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

    GLint transformMatrixLocation;
    GLint cameraMatrixLocation;

    float aspect;

    glm::mat4 cameraTransform;

public:
    GLHelper();
    ~GLHelper();

    GLuint createShader(GLenum, const std::string &);
    GLuint createProgram(const std::vector<GLuint> &);
    GLuint initializeProgram(std::string vertexShaderFile, std::string fragmentShaderFile);

    void bufferVertexData(const GLfloat* vertexData, const GLuint vertexSize,
                          const GLuint* elementData, const GLuint elementSize,
                          GLuint& vao, GLuint& vbo, const GLuint attachPointer, GLuint& ebo);
    void bufferVertexColor(const GLfloat* colorData, const GLuint ColorSize,
                           GLuint& vao, GLuint& vbo, const GLuint attachPointer);
    void bufferVertexTextureCoordinates(const GLfloat* coordinateData, const GLuint coordinateDataSize,
                          GLuint& vao, GLuint& vbo, const GLuint attachPointer, GLuint& ebo);

    void setCamera(const glm::mat4&);

    void clearFrame(){
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
;    }
    void render(const GLuint, const GLuint, const GLuint, const glm::mat4&, const GLuint);
    void reshape(int height, int width);

    GLuint loadTexture(int height, int width, bool alpha, void *data);
    GLuint loadCubeMap(int height, int width, void* right, void* left, void* top, void* bottom, void* back, void* front);
    void attachTexture(GLuint textureID);
    void attachCubeMap(GLuint cubeMapID);

    bool deleteTexture(GLuint textureID);
};

#endif //UBERGAME_GLHELPER_H
