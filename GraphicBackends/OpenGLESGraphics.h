//
// Created by engin on 24.09.2019.
//

#ifndef LIMONENGINE_OPENGLESGRAPHICS_H
#define LIMONENGINE_OPENGLESGRAPHICS_H

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
#include "API/Graphics/GraphicsInterface.h"

class Material;

class Light;

class GraphicsProgram;
class Texture;

extern "C" std::shared_ptr<GraphicsInterface> createGraphicsBackend(Options* options);


class OpenGLESGraphics : public GraphicsInterface {
    friend class Texture;
    class OpenglState {
        unsigned int activeProgram;
        unsigned int activeTextureUnit;
        std::vector<unsigned int> textures;

        /* backup and restore part */
        GLenum last_active_texture;
        GLint last_program;
        GLint last_texture;
        GLint last_Texture2DArray;
        GLint last_sampler;
        GLint last_array_buffer;
        GLint last_element_array_buffer;
        GLint last_vertex_array;
        GLint last_polygon_mode[2];
        GLint last_viewport[4];
        GLint last_scissor_box[4];
        GLenum last_blend_src_rgb;
        GLenum last_blend_dst_rgb;
        GLenum last_blend_src_alpha;
        GLenum last_blend_dst_alpha;
        GLenum last_blend_equation_rgb;
        GLenum last_blend_equation_alpha;
        GLboolean last_enable_blend;
        GLboolean last_enable_cull_face;
        GLboolean last_enable_depth_test;
        GLboolean last_enable_scissor_test;
        /* backup and restore part */

        void attachTexture(GLuint textureID, GLuint textureUnit, GLenum type) {
            if (textures[textureUnit] != textureID) {
                textures[textureUnit] = textureID;
                activateTextureUnit(textureUnit);
                glBindTexture(type, textureID);
            }
        }

    public:

        void backupState() {
            glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&last_active_texture);
            glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
            glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &last_Texture2DArray);
            glGetIntegerv(GL_SAMPLER_BINDING, &last_sampler);
            glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
            glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
            glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
            glGetIntegerv(GL_VIEWPORT, last_viewport);
            glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
            glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&last_blend_src_rgb);
            glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&last_blend_dst_rgb);
            glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&last_blend_src_alpha);
            glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&last_blend_dst_alpha);
            glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&last_blend_equation_rgb);
            glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&last_blend_equation_alpha);
            last_enable_blend = glIsEnabledi(GL_BLEND, 0);
            last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
            last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
            last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
        }

        void restoreState() {
            // Restore modified GL state
            glUseProgram(last_program);
            glBindTexture(GL_TEXTURE_2D, last_texture);
            glBindTexture(GL_TEXTURE_2D_ARRAY, last_Texture2DArray);
            glBindSampler(0, last_sampler);
            glActiveTexture(last_active_texture);
            activeTextureUnit = last_active_texture;
            glBindVertexArray(last_vertex_array);
            glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
            glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
            glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
            if (last_enable_blend) glEnablei(GL_BLEND,0); else glDisablei(GL_BLEND,0);
            if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
            if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
            if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, last_polygon_mode[0]);
            glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
            glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
        }
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

        void attach1DTexture(GLuint textureID, GLuint textureUnit) {
            attachTexture(textureID, textureUnit, GL_TEXTURE_1D);
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
    GLuint allModelIndexesUBOLocation;

    uint32_t activeMaterialIndex;

    GLuint combineFrameBuffer;

    Options *options;

    const uint32_t lightUniformSize = (sizeof(glm::mat4) * 7) + (4 * sizeof(glm::vec4));
    const uint32_t playerUniformSize = 6 * sizeof(glm::mat4) + 3 * sizeof(glm::vec4);
    int32_t materialUniformSize = 2 * sizeof(glm::vec3) + sizeof(float) + sizeof(GLuint);
    int32_t modelUniformSize = sizeof(glm::mat4);

    GLuint allModelTransformsTexture;

    glm::mat4 cameraMatrix;
    glm::mat4 perspectiveProjectionMatrix;
    glm::mat4 inverseProjection;
    std::vector<glm::vec4>frustumPlanes;
    glm::mat4 orthogonalProjectionMatrix;
    glm::vec3 cameraPosition;
    uint32_t renderTriangleCount;
    uint32_t renderLineCount;
    uint32_t uniformSetCount=0;

    bool isProgramInterfaceQuerySupported = false;
    bool isFrameBufferParameterSupported = false;
    bool isDebugOutputSupported = false;

public:

    void getRenderTriangleAndLineCount(uint32_t& triangleCount, uint32_t& lineCount) override {
        triangleCount = renderTriangleCount;
        lineCount = renderLineCount;
    }

    bool getFrameBufferParameterSupported() const {
        return isFrameBufferParameterSupported;
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

    inline void pushDebugGroup(const std::string& groupName) {
#ifndef NDEBUG
        if(isDebugOutputSupported) {
            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, groupName.length(), groupName.c_str());
        }
#endif
    }
    inline void popDebugGroup() {
#ifndef NDEBUG
        if(isDebugOutputSupported) {
            glPopDebugGroup();
        }
#endif
    }

    GLuint createShader(GLenum, const std::string &);

    GLuint createProgram(const std::vector<GLuint> &);

    GLuint generateBuffer(const GLuint number);

    bool deleteBuffer(const GLuint number, const GLuint bufferID);

    GLuint generateVAO(const GLuint number);

    bool deleteVAO(const GLuint number, const GLuint bufferID);

    void fillUniformAndOutputMaps(const GLuint program,
            std::unordered_map<std::string, std::shared_ptr<Uniform>> &uniformMap,
            std::unordered_map<std::string, uint32_t> &attributesMap,
            std::unordered_map<std::string, std::pair<Uniform::VariableTypes, FrameBufferAttachPoints>> &outputMap);

    void attachGeneralUBOs(const GLuint program);
    void bufferExtraVertexData(uint32_t elementPerVertexCount, GLenum elementType, uint32_t dataSize,
                               const void *extraData, uint32_t &vao, uint32_t &vbo,
                               const uint32_t attachPointer);


    Uniform::VariableTypes getVariableType(const GLenum typeEnum) const;

    Uniform::VariableTypes getSamplerVariableType(const GLint *queryResults) const;
protected:
    uint32_t createTexture(int height, int width, TextureTypes type, InternalFormatTypes internalFormat, FormatTypes format, DataTypes dataType, uint32_t textureLayers) override;

    bool deleteTexture(GLuint textureID) override;

    void setWrapMode(uint32_t textureID, TextureTypes textureType, TextureWrapModes wrapModeS,
                     TextureWrapModes wrapModeT, TextureWrapModes wrapModeR) override;

    void setTextureBorder(uint32_t textureID, TextureTypes textureType, bool isBorderColorSet,
                          const std::vector<float> &borderColors) override;

    void setFilterMode(uint32_t textureID, TextureTypes textureType, FilterModes filterMode) override;

    void loadTextureData(uint32_t textureID, int height, int width, TextureTypes type, InternalFormatTypes internalFormat, FormatTypes format, DataTypes dataType, uint32_t depth,
                         void *data, void *data2, void *data3, void *data4, void *data5, void *data6) override;

    uint32_t createGraphicsProgram(const std::string &vertexShaderFile, const std::string &geometryShaderFile, const std::string &fragmentShaderFile) override;

public:


    explicit OpenGLESGraphics(Options *options);

    GraphicsInterface::ContextInformation getContextInformation();
    bool createGraphicsBackend();

    ~OpenGLESGraphics();

    void attachModelUBO(const uint32_t program) override;

    void attachMaterialUBO(const uint32_t program, const uint32_t materialID) override;

    uint32_t getNextMaterialIndex() override{
        return nextMaterialIndex++;
    }

    void initializeProgramAsset(const uint32_t programId,
                                std::unordered_map<std::string, std::shared_ptr<Uniform>> &uniformMap,
                                std::unordered_map<std::string, uint32_t> &attributesMap,
                                std::unordered_map<std::string, std::pair<Uniform::VariableTypes, FrameBufferAttachPoints>> &outputMap) override;

    void destroyProgram(uint32_t programID) override;

    void bufferVertexData(const std::vector<glm::vec3> &vertices,
                          const std::vector<glm::mediump_uvec3> &faces,
                          uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer, uint32_t &ebo) override;

    void bufferNormalData(const std::vector<glm::vec3> &colors,
                          uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer) override;

    void bufferExtraVertexData(const std::vector<glm::vec4> &extraData,
                               uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer) override;

    void bufferExtraVertexData(const std::vector<glm::lowp_uvec4> &extraData,
                               uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer) override;

    void bufferVertexTextureCoordinates(const std::vector<glm::vec2> &textureCoordinates,
                                        uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer) override;
    void updateVertexData(const std::vector<glm::vec3> &vertices, const std::vector<glm::mediump_uvec3> &faces,
                          uint32_t &vbo, uint32_t &ebo);
    void updateNormalData(const std::vector<glm::vec3> &normals, uint32_t &vbo);
    void updateExtraVertexData(const std::vector<glm::vec4> &extraData, uint32_t &vbo);
    void updateExtraVertexData(const std::vector<glm::lowp_uvec4> &extraData, uint32_t &vbo);
    void updateVertexTextureCoordinates(const std::vector<glm::vec2> &textureCoordinates, uint32_t &vbo);
    bool freeBuffer(const uint32_t bufferID) override;

    bool freeVAO(const uint32_t VAO) override;

    void backupCurrentState();

    void restoreLastState();

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

    void render(const uint32_t program, const uint32_t vao, const uint32_t ebo, const uint32_t elementCount) {
        this->render(program, vao, ebo, elementCount, nullptr);
    }

    void render(const uint32_t program, const uint32_t vao, const uint32_t ebo, const uint32_t elementCount, const uint32_t* startIndex) override;

    void reshape() override;

    uint32_t createFrameBuffer(uint32_t width, uint32_t height) override;
    void deleteFrameBuffer(uint32_t frameBufferID) override;
    void attachDrawTextureToFrameBuffer(uint32_t frameBufferID, TextureTypes textureType, uint32_t textureID,
                                        FrameBufferAttachPoints attachPoint, int32_t layer = 0 , bool clear = false) override;

    void attachTexture(unsigned int textureID, unsigned int attachPoint) override;
    void attach2DArrayTexture(unsigned int textureID, unsigned int attachPoint) override;

    void attachCubeMap(unsigned int cubeMapID, unsigned int attachPoint) override;
    void attachCubeMapArrayTexture(unsigned int textureID, unsigned int attachPoint) override;


    bool getUniformLocation(const uint32_t programID, const std::string &uniformName, uint32_t &location) override;

    const glm::mat4& getCameraMatrix() const override { return cameraMatrix; };

    const glm::vec3& getCameraPosition() const override { return cameraPosition; };

    const glm::mat4& getProjectionMatrix() const override { return perspectiveProjectionMatrix; };

    const glm::mat4& getOrthogonalProjectionMatrix() const override { return orthogonalProjectionMatrix; }

    void createDebugVAOVBO(uint32_t &vao, uint32_t &vbo, uint32_t bufferSize) override;

    void drawLines(GraphicsProgram &program, uint32_t vao, uint32_t vbo, const std::vector<Line> &lines) override;

    void clearDepthBuffer() override {
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    bool setUniform(const uint32_t programID, const uint32_t uniformID, const glm::mat4 &matrix) override;

    bool setUniform(const uint32_t programID, const uint32_t uniformID, const glm::vec3 &vector) override;

    bool setUniform(const uint32_t programID, const uint32_t uniformID, const std::vector<glm::vec3> &vectorArray) override;

    bool setUniform(const uint32_t programID, const uint32_t uniformID, const float value) override;

    bool setUniform(const uint32_t programID, const uint32_t uniformID, const int value) override;

    bool setUniformArray(const uint32_t programID, const uint32_t uniformID, const std::vector<glm::mat4> &matrixArray) override;

    void setLight(const int lightIndex,
                  const glm::vec3& attenuation,
                  const glm::mat4* shadowMatrices,
                  const glm::vec3& position,
                  const glm::vec3& color,
                  const glm::vec3& ambientColor,
                  const int32_t lightType,
                  const float farPlane) override;

    void removeLight(const int i) override {
        GLint temp = 0;
        glBindBuffer(GL_UNIFORM_BUFFER, lightUBOLocation);
        glBufferSubData(GL_UNIFORM_BUFFER, i * lightUniformSize + sizeof(glm::mat4) * 6 + sizeof(glm::vec4) + sizeof(glm::vec3),
                        sizeof(GLint), &temp);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        checkErrors("removeLight");
    }

    void setPlayerMatrices(const glm::vec3 &cameraPosition, const glm::mat4 &cameraMatrix, long currentTime) override;

    void switchRenderStage(uint32_t width, uint32_t height, uint32_t frameBufferID, bool blendEnabled, bool depthTestEnabled, bool depthWriteEnabled, bool scissorEnabled,
                           bool clearColor, bool clearDepth, CullModes cullMode, std::map<uint32_t, std::shared_ptr<Texture>> &inputs, const std::string &name) override;
    void switchRenderStage(uint32_t width, uint32_t height, uint32_t frameBufferID, bool blendEnabled, bool depthTestEnabled, bool depthWriteEnabled, bool scissorEnabled,
                           bool clearColor, bool clearDepth, CullModes cullMode, const std::map<uint32_t, std::shared_ptr<Texture>> &inputs,
                           const std::map<std::shared_ptr<Texture>,
                                   std::pair<FrameBufferAttachPoints, int>> &attachmentLayerMap, const std::string &name) override;

    int getMaxTextureImageUnits() const override {
        return maxTextureImageUnits;
    }

    void setMaterial(const Material& material) override;

    void setModel(const uint32_t modelID, const glm::mat4 &worldTransform) override;

    void setModelIndexesUBO(const std::vector<uint32_t> &modelIndicesList) override;

    void attachModelIndicesUBO(const uint32_t programID) override;

    void renderInstanced(uint32_t program, uint32_t VAO, uint32_t EBO, uint32_t triangleCount,
                         uint32_t instanceCount) override;

    void renderInstanced(uint32_t program, uint32_t VAO, uint32_t EBO, uint32_t triangleCount, uint32_t startOffset,
                         uint32_t instanceCount) override;

    void setScissorRect(int32_t x, int32_t y, uint32_t width, uint32_t height) {
        glScissor(x,y,width,height);
    }

    Options* getOptions() {
        return options;
    }
};


#endif //LIMONENGINE_OPENGLESGRAPHICS_H
