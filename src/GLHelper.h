//
// Created by Engin Manap on 10.02.2016.
//

#ifndef UBERGAME_GLHELPER_H
#define UBERGAME_GLHELPER_H

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "Light.h"

#include <GL/glew.h>

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#else

#  include <GL/gl.h>

#endif/*__APPLE__*/

#define NR_POINT_LIGHTS 4

#include <string>
#include <algorithm>
#include <vector>

#include <fstream>
#include <streambuf>
#include <iostream>
#include <map>

class GLHelper {
public:
    enum VariableTypes {
        INT,
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

        Uniform(unsigned int location, const std::string &name, GLenum typeEnum, unsigned int size) : location(
                location), name(name), size(size) {
            switch (typeEnum) {
                case GL_SAMPLER_CUBE: //these are because sampler takes a int as texture unit
                case GL_SAMPLER_2D:
                case GL_INT:
                    type = INT;
                    break;
                case GL_FLOAT:
                    type = FLOAT;
                    break;
                case GL_FLOAT_VEC2:
                    type = FLOAT_VEC2;
                    break;
                case GL_FLOAT_VEC3:
                    type = FLOAT_VEC3;
                    break;
                case GL_FLOAT_VEC4:
                    type = FLOAT_VEC4;
                    break;
                case GL_FLOAT_MAT4:
                    type = FLOAT_MAT4;
                    break;
                default:
                    type = UNDEFINED;
            }

        }
    };

private:
    GLenum error;

    unsigned int screenHeight, screenWidth;
    float aspect;
    std::vector<GLuint> bufferObjects;
    std::vector<GLuint> vertexArrays;


    glm::vec3 cameraPosition;
    GLuint lightUBOLocation;
    GLuint playerUBOLocation;

    GLuint depthOnlyFrameBuffer;
    GLuint depthMap;

    const GLuint SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048; //TODO these values should be parameters
    const GLfloat lightOrthogonalProjectionNearPlane = 1.0f;
    const GLfloat lightOrthogonalProjectionFarPlane = 100.0f;
    const glm::vec4 lightOrthogonalProjectionValues = glm::vec4(-100.0f, 100.0f, -100.0f, 100.0f);

    glm::mat4 cameraMatrix;
    glm::mat4 perspectiveProjectionMatrix;
    glm::mat4 orthogonalProjectionMatrix;
    glm::mat4 lightProjection;

    bool checkErrors(std::string callerFunc);

    GLuint createShader(GLenum, const std::string &);

    GLuint createProgram(const std::vector<GLuint> &);

    GLuint generateBuffer(const GLuint number);

    bool deleteBuffer(const GLuint number, const GLuint bufferID);

    GLuint generateVAO(const GLuint number);

    bool deleteVAO(const GLuint number, const GLuint bufferID);

    void fillUniformMap(const GLuint program, std::map<std::string, Uniform *> &uniformMap) const;

    void attachUBOs(const GLuint program) const;

public:
    GLHelper();

    ~GLHelper();

    GLuint initializeProgram(std::string vertexShaderFile, std::string fragmentShaderFile, std::map<std::string, Uniform*>&);

    void bufferVertexData(const std::vector<glm::vec3> &vertices,
                          const std::vector<glm::mediump_uvec3> &faces,
                          GLuint &vao, GLuint &vbo, const GLuint attachPointer, GLuint &ebo);

    void bufferNormalData(const std::vector<glm::vec3> &colors,
                          GLuint &vao, GLuint &vbo, const GLuint attachPointer);

    void bufferVertexColor(const std::vector<glm::vec4> &colors,
                           GLuint &vao, GLuint &vbo, const GLuint attachPointer);


    void bufferVertexTextureCoordinates(const std::vector<glm::vec2> &textureCoordinates,
                                        GLuint &vao, GLuint &vbo, const GLuint attachPointer, GLuint &ebo);

    bool freeBuffer(const GLuint bufferID);

    bool freeVAO(const GLuint VAO);

    void setCamera(const glm::vec3 &, const glm::mat4 &);

    void clearFrame() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void render(const GLuint, const GLuint, const GLuint, const GLuint);

    void reshape(unsigned int height, unsigned int width);

    GLuint loadTexture(int height, int width, GLenum format, void *data);

    GLuint loadCubeMap(int height, int width, void *right, void *left, void *top, void *bottom, void *back,
                       void *front);

    void attachTexture(unsigned int textureID, unsigned int attachPoint);

    void attachCubeMap(unsigned int cubeMapID, unsigned int attachPoint);

    bool deleteTexture(GLuint textureID);

    bool getUniformLocation(const GLuint programID, const std::string &uniformName, GLuint &location);

    glm::mat4 getCameraMatrix() const { return cameraMatrix; };

    glm::vec3 getCameraPosition() const { return cameraPosition; };

    glm::mat4 getProjectionMatrix() const { return perspectiveProjectionMatrix; };

    glm::mat4 getOrthogonalProjectionMatrix() const { return orthogonalProjectionMatrix; }

    void drawLine(const glm::vec3 &from, const glm::vec3 &to,
                  const glm::vec3 &fromColor, const glm::vec3 &toColor, bool willTransform);

    void clearDepthBuffer() {
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    bool setUniform(const GLuint programID, const GLuint uniformID, const glm::mat4 &matrix);

    bool setUniform(const GLuint programID, const GLuint uniformID, const glm::vec3 &vector);

    bool setUniform(const GLuint programID, const GLuint uniformID, const float value);

    bool setUniform(const GLuint programID, const GLuint uniformID, const int value);

    void setLight(const Light &light, const int i);

    void setPlayerMatrices();

    void switchFrameBufferToShadowMap(const unsigned int index);

    void switchFrameBufferToDefault();
};

#endif //UBERGAME_GLHELPER_H
