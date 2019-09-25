//
// Created by engin on 24.09.2019.
//

#ifndef LIMONENGINE_OPENGLGRAPHICS_H
#define LIMONENGINE_OPENGLGRAPHICS_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <algorithm>
#include <vector>

#include <fstream>
#include <streambuf>
#include <iostream>
#include <unordered_map>
#include <GL/glew.h>

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#else

#include <GL/gl.h>
#include <memory>
#include <map>

#endif/*__APPLE__*/


#include "Options.h"
#include "GraphicsInterface.h"

class Material;

class Light;

class GraphicsProgram;
class Texture;

class OpenGLGraphics : public GraphicsInterface {
    friend class Texture;
    class OpenglState {
        unsigned int activeProgram;
        unsigned int activeTextureUnit;
        std::vector<unsigned int> textures;

        void attachTexture(GLuint textureID, GLuint textureUnit, GLenum type) {
            if (textures[textureUnit] != textureID) {
                textures[textureUnit] = textureID;
                activateTextureUnit(textureUnit);
                glBindTexture(type, textureID);
            }
        }

    public:
        explicit OpenglState(GLint textureUnitCount) : activeProgram(0) {
            textures.resize(textureUnitCount);
            memset(textures.data(), 0, textureUnitCount * sizeof(int));
            activeTextureUnit = 0;
            glActiveTexture(GL_TEXTURE0);
        }

        void activateTextureUnit(GLuint textureUnit) {
            if (activeTextureUnit != textureUnit) {
                activeTextureUnit = textureUnit;
                //https://www.khronos.org/opengles/sdk/1.1/docs/man/glActiveTexture.xml guarantees below works for texture selection
                glActiveTexture(GL_TEXTURE0 + textureUnit);
            }
        }

        void attachTexture(GLuint textureID, GLuint textureUnit) {
            attachTexture(textureID, textureUnit, GL_TEXTURE_2D);
        }

        void attach2DTextureArray(GLuint textureID, GLuint textureUnit) {
            attachTexture(textureID, textureUnit, GL_TEXTURE_2D_ARRAY);
        }

        void attachCubemap(GLuint textureID, GLuint textureUnit) {
            attachTexture(textureID, textureUnit, GL_TEXTURE_CUBE_MAP);
        }

        bool deleteTexture(GLuint textureID) {
            //clear this textures attachment state
            for (size_t i = 0; i < textures.size(); ++i) {
                if(textures[i] == textureID) {
                    textures[i] = 0;
                }
            }
            if (glIsTexture(textureID)) {
                glDeleteTextures(1, &textureID);
                return true;
            } else {

                return false;
            }
        }

        void attachCubemapArray(GLuint textureID, GLuint textureUnit) {
            attachTexture(textureID, textureUnit, GL_TEXTURE_CUBE_MAP_ARRAY_ARB);
        }


        void setProgram(GLuint program) {
            if (program != this->activeProgram) {
                glUseProgram(program);
                this->activeProgram = program;
            }
        }
    };

private:
    GLenum error;
    uint32_t nextMaterialIndex = 0;//this is used to keep each material in the  GPU memory. imagine it like size of vector
    GLint maxTextureImageUnits;
    OpenglState *state;

    unsigned int screenHeight, screenWidth;
    float aspect;
    std::vector<GLuint> bufferObjects;
    std::vector<GLuint> vertexArrays;
    std::vector<GLuint> modelIndexesTemp;


    GLuint lightUBOLocation;
    GLuint playerUBOLocation;
    GLuint allMaterialsUBOLocation;
    GLuint allModelsUBOLocation;
    GLuint allModelIndexesUBOLocation;

    uint32_t activeMaterialIndex;

    GLuint combineFrameBuffer;

    Options *options;

    const uint_fast32_t lightUniformSize = (sizeof(glm::mat4) * 7) + (4 * sizeof(glm::vec4));
    const uint32_t playerUniformSize = 5 * sizeof(glm::mat4)+ 3* sizeof(glm::vec4);
    int32_t materialUniformSize = 2 * sizeof(glm::vec3) + sizeof(float) + sizeof(GLuint);
    int32_t modelUniformSize = sizeof(glm::mat4);

    glm::mat4 cameraMatrix;
    glm::mat4 perspectiveProjectionMatrix;
    glm::mat4 inverseProjection;
    std::vector<glm::vec4>frustumPlanes;
    glm::mat4 orthogonalProjectionMatrix;
    glm::mat4 lightProjectionMatrixDirectional;
    glm::mat4 lightProjectionMatrixPoint;
    glm::vec3 cameraPosition;
    uint32_t renderTriangleCount;
    uint32_t renderLineCount;
    uint32_t uniformSetCount=0;

    /**
     * This is not keeping shared_ptr because getting a shared_ptr from destructor is not logical
     */
    std::map<std::shared_ptr<GraphicsProgram>, int> loadedPrograms;

public:

    const std::map<std::shared_ptr<GraphicsProgram>, int> &getLoadedPrograms() const override {
        return loadedPrograms;
    }

    void getRenderTriangleAndLineCount(uint32_t& triangleCount, uint32_t& lineCount) override {
        triangleCount = renderTriangleCount;
        lineCount = renderLineCount;
    }

    const glm::mat4 &getLightProjectionMatrixPoint() const override {
        return lightProjectionMatrixPoint;
    }

    const glm::mat4 &getLightProjectionMatrixDirectional() const override {
        return lightProjectionMatrixDirectional;
    }

private:
    inline bool checkErrors(const std::string &callerFunc __attribute((unused))) {
#ifndef NDEBUG
        GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (fbStatus != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "FB status while " << callerFunc << " is " << fbStatus << std::endl;
        }
        bool hasError = false;
        while ((error = glGetError()) != GL_NO_ERROR) {
            std::cerr << "error found on GL context while " << callerFunc << ":" << error << ": " << gluErrorString(error)
                      << std::endl;
            hasError = true;
        }
        return hasError;
#else
        return false;
#endif
    };

    GLuint createShader(GLenum, const std::string &);

    GLuint createProgram(const std::vector<GLuint> &);

    GLuint generateBuffer(const GLuint number);

    bool deleteBuffer(const GLuint number, const GLuint bufferID);

    GLuint generateVAO(const GLuint number);

    bool deleteVAO(const GLuint number, const GLuint bufferID);

    void fillUniformAndOutputMaps(const GLuint program, std::unordered_map<std::string, const Uniform *> &uniformMap, std::unordered_map<std::string, VariableTypes> &outputMap);

    void attachGeneralUBOs(const GLuint program);
    void bufferExtraVertexData(uint_fast32_t elementPerVertexCount, GLenum elementType, uint_fast32_t dataSize,
                               const void *extraData, uint_fast32_t &vao, uint_fast32_t &vbo,
                               const uint_fast32_t attachPointer);

    void testAndRemoveGLSLProgram(GraphicsProgram *program);


protected:
    uint32_t createTexture(int height, int width, TextureTypes type, InternalFormatTypes internalFormat, FormatTypes format, DataTypes dataType, uint32_t depth) override;

    bool deleteTexture(GLuint textureID) override;

    void setWrapMode(Texture& texture, TextureWrapModes wrapModeS, TextureWrapModes wrapModeT, TextureWrapModes wrapModeR) override;

    void setTextureBorder(Texture& texture) override;

    void setFilterMode(Texture& texture, FilterModes filterMode) override;

    void loadTextureData(uint32_t textureID, int height, int width, TextureTypes type, InternalFormatTypes internalFormat, FormatTypes format, DataTypes dataType, uint32_t depth,
                         void *data, void *data2, void *data3, void *data4, void *data5, void *data6) override;

public:
    explicit OpenGLGraphics(Options *options);

    ~OpenGLGraphics();

    std::shared_ptr<GraphicsProgram> createGLSLProgram(const std::string &vertexShader, const std::string &geometryShader, const std::string &fragmentShader, bool isMaterialUsed) override;
    std::shared_ptr<GraphicsProgram> createGLSLProgram(const std::string &vertexShader, const std::string &fragmentShader, bool isMaterialUsed) override;

    void attachModelUBO(const uint32_t program) override;

    void attachMaterialUBO(const uint32_t program, const uint32_t materialID) override;

    uint32_t getNextMaterialIndex() override{
        return nextMaterialIndex++;
    }

    GLuint initializeProgram(const std::string &vertexShaderFile, const std::string &geometryShaderFile, const std::string &fragmentShaderFile,
                             std::unordered_map<std::string,const Uniform *> &uniformMap, std::unordered_map<std::string, VariableTypes> &outputMap) override;
    void destroyProgram(uint32_t programID) override;

    void bufferVertexData(const std::vector<glm::vec3> &vertices,
                          const std::vector<glm::mediump_uvec3> &faces,
                          uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer, uint_fast32_t &ebo) override;

    void bufferNormalData(const std::vector<glm::vec3> &colors,
                          uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer) override;

    void bufferExtraVertexData(const std::vector<glm::vec4> &extraData,
                               uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer) override;

    void bufferExtraVertexData(const std::vector<glm::lowp_uvec4> &extraData,
                               uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer) override;

    void bufferVertexTextureCoordinates(const std::vector<glm::vec2> &textureCoordinates,
                                        uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer) override;

    bool freeBuffer(const GLuint bufferID) override;

    bool freeVAO(const GLuint VAO) override;

    void clearFrame() {

        //additional depths for Directional is not needed, but depth for point is reqired, because there is no way to clear
        //it per layer, so we are clearing per frame. This also means, lights should not reuse the textures.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);//combining doesn't need depth test either
        glClear(GL_COLOR_BUFFER_BIT);//clear for default

        renderTriangleCount = 0;
        renderLineCount = 0;
        //std::cout << "uniform set count was : " << uniformSetCount << std::endl;
        uniformSetCount = 0;
        checkErrors("clearFrame");
    }

    void render(const GLuint program, const GLuint vao, const GLuint ebo, const GLuint elementCount) override;

    void reshape() override;

    uint32_t createFrameBuffer(uint32_t width, uint32_t height) override;
    void deleteFrameBuffer(uint32_t frameBufferID) override;
    void attachDrawTextureToFrameBuffer(uint32_t frameBufferID, TextureTypes textureType, uint32_t textureID,
                                        FrameBufferAttachPoints attachPoint, int32_t layer = 0 , bool clear = false) override;

    void attachTexture(unsigned int textureID, unsigned int attachPoint) override;

    void attachCubeMap(unsigned int cubeMapID, unsigned int attachPoint) override;

    bool getUniformLocation(const GLuint programID, const std::string &uniformName, GLuint &location) override;

    const glm::mat4& getCameraMatrix() const override { return cameraMatrix; };

    const glm::vec3& getCameraPosition() const override { return cameraPosition; };

    const glm::mat4& getProjectionMatrix() const override { return perspectiveProjectionMatrix; };

    const glm::mat4& getOrthogonalProjectionMatrix() const override { return orthogonalProjectionMatrix; }

    void createDebugVAOVBO(uint32_t &vao, uint32_t &vbo, uint32_t bufferSize) override;

    void drawLines(GraphicsProgram &program, uint32_t vao, uint32_t vbo, const std::vector<Line> &lines) override;

    void clearDepthBuffer() override {
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    bool setUniform(const GLuint programID, const GLuint uniformID, const glm::mat4 &matrix) override;

    bool setUniform(const GLuint programID, const GLuint uniformID, const glm::vec3 &vector) override;

    bool setUniform(const GLuint programID, const GLuint uniformID, const std::vector<glm::vec3> &vectorArray) override;

    bool setUniform(const GLuint programID, const GLuint uniformID, const float value) override;

    bool setUniform(const GLuint programID, const GLuint uniformID, const int value) override;

    bool setUniformArray(const GLuint programID, const GLuint uniformID, const std::vector<glm::mat4> &matrixArray) override;

    void setLight(const Light &light, const int i) override;

    void removeLight(const int i) override {
        GLint temp = 0;
        glBindBuffer(GL_UNIFORM_BUFFER, lightUBOLocation);
        glBufferSubData(GL_UNIFORM_BUFFER, i * lightUniformSize + sizeof(glm::mat4) * 7 + sizeof(glm::vec4) + sizeof(glm::vec3),
                        sizeof(GLint), &temp);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        checkErrors("removeLight");
    }

    void setPlayerMatrices(const glm::vec3 &cameraPosition, const glm::mat4 &cameraMatrix) override;

    void switchRenderStage(uint32_t width, uint32_t height, uint32_t frameBufferID, bool blendEnabled, bool clearColor, bool clearDepth, CullModes cullMode,
                           std::map<uint32_t, std::shared_ptr<Texture>> &inputs) override;
    void switchRenderStage(uint32_t width, uint32_t height, uint32_t frameBufferID, bool blendEnabled, bool clearColor, bool clearDepth, CullModes cullMode,
                           const std::map<uint32_t, std::shared_ptr<Texture>> &inputs,
                           const std::map<std::shared_ptr<Texture>, std::pair<FrameBufferAttachPoints, int>> &attachmentLayerMap) override;

    int getMaxTextureImageUnits() const override {
        return maxTextureImageUnits;
    }

    void calculateFrustumPlanes(const glm::mat4 &cameraMatrix, const glm::mat4 &projectionMatrix,
                                std::vector<glm::vec4> &planes) const override;

    inline bool isInFrustum(const glm::vec3& aabbMin, const glm::vec3& aabbMax) const override {
        return isInFrustum(aabbMin, aabbMax, frustumPlanes);
    }

    inline bool isInFrustum(const glm::vec3& aabbMin, const glm::vec3& aabbMax, const std::vector<glm::vec4>& frustumPlaneVector) const override {
        bool inside = true;
        //test all 6 frustum planes
        for (int i = 0; i<6; i++) {
            //pick closest point to plane and check if it behind the plane
            //if yes - object outside frustum
            float d =   std::fmax(aabbMin.x * frustumPlaneVector[i].x, aabbMax.x * frustumPlaneVector[i].x)
                        + std::fmax(aabbMin.y * frustumPlaneVector[i].y, aabbMax.y * frustumPlaneVector[i].y)
                        + std::fmax(aabbMin.z * frustumPlaneVector[i].z, aabbMax.z * frustumPlaneVector[i].z)
                        + frustumPlaneVector[i].w;
            inside &= d > 0;
            //return false; //with flag works faster
        }
        return inside;
    }

    void setMaterial(std::shared_ptr<const Material>material) override;

    void setModel(const uint32_t modelID, const glm::mat4 &worldTransform) override;

    void setModelIndexesUBO(std::vector<uint32_t> &modelIndicesList) override;

    void attachModelIndicesUBO(const uint32_t programID) override;

    void renderInstanced(GLuint program, uint_fast32_t VAO, uint_fast32_t EBO, uint_fast32_t triangleCount,
                         uint32_t instanceCount) override;

};


#endif //LIMONENGINE_OPENGLGRAPHICS_H
