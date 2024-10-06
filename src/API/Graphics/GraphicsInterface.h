//
// Created by Engin Manap on 10.02.2016.
//

#ifndef LIMONENGINE_GRAPHICSINTERFACE_H
#define LIMONENGINE_GRAPHICSINTERFACE_H


#include <memory>
#include <map>
#include <unordered_map>
#include <vector>
#include <functional>


#define NR_POINT_LIGHTS 3
#define NR_TOTAL_LIGHTS 4
#define NR_MAX_MODELS (1000)
#define NR_MAX_MATERIALS 2000

#include "Options.h"
#include "Uniform.h"

class Material;

class Light;

class GraphicsProgram;
class Texture;
class RenderMethodInterface;


class GraphicsInterface {
    friend class GraphicsProgram; //TODO This is to allow access of protected method createGraphicsProgram we should come up with something better
public:

    enum class TextureTypes {T2D, T2D_ARRAY, TCUBE_MAP, TCUBE_MAP_ARRAY};//Starting with digits is illegal
    enum class InternalFormatTypes {RED, R32F, RGB, RGBA, RGB16F, RGB32F, RGBA32F, DEPTH, COMPRESSED_RGB, COMPRESSED_RGBA };
    enum class FormatTypes {RED, RGB, RGBA, DEPTH};
    enum class DataTypes {UNSIGNED_BYTE, FLOAT, HALF_FLOAT, UNSIGNED_SHORT, UNSIGNED_INT};
    enum class FrameBufferAttachPoints {NONE, COLOR0, COLOR1, COLOR2, COLOR3, COLOR4, COLOR5, COLOR6, DEPTH };
    enum class TextureWrapModes {NONE, REPEAT, BORDER, EDGE};
    enum class FilterModes {NEAREST, LINEAR, TRILINEAR};
    enum class CullModes {FRONT, BACK, NONE, NO_CHANGE};

protected:
    friend class Texture;
    friend class RenderMethodInterface;
    virtual uint32_t createTexture(int height, int width, TextureTypes type, InternalFormatTypes internalFormat, FormatTypes format, DataTypes dataType, uint32_t textureLayers) = 0;

    virtual bool deleteTexture(uint32_t textureID) = 0;

    virtual void setWrapMode(uint32_t textureID, TextureTypes textureType, TextureWrapModes wrapModeS,
                             TextureWrapModes wrapModeT, TextureWrapModes wrapModeR) = 0;

    virtual void setTextureBorder(uint32_t textureID, TextureTypes textureType, bool isBorderColorSet,
                                  const std::vector<float> &borderColors) = 0;

    virtual void setFilterMode(uint32_t textureID, TextureTypes textureType, FilterModes filterMode) = 0;

    virtual void loadTextureData(uint32_t textureID, int height, int width, TextureTypes type, InternalFormatTypes internalFormat, FormatTypes format, DataTypes dataType, uint32_t depth,
                         void *data, void *data2, void *data3, void *data4, void *data5, void *data6) = 0;

    //Should be used by GraphicsProgramOnly
    virtual uint32_t createGraphicsProgram(const std::string &vertexShaderFile, const std::string &geometryShaderFile, const std::string &fragmentShaderFile) = 0;
public:

    struct ContextInformation {
        int SDL_GL_ACCELERATED_VISUAL = 1;
        int SDL_GL_CONTEXT_MAJOR_VERSION = 3;
        int SDL_GL_CONTEXT_MINOR_VERSION = 3;
        int SDL_GL_CONTEXT_PROFILE_MASK = 1;
        int SDL_GL_CONTEXT_FLAGS = 1;
    };

    virtual void getRenderTriangleAndLineCount(uint32_t& triangleCount, uint32_t& lineCount) = 0;
    explicit GraphicsInterface(Options *options [[gnu::unused]]) {};
    virtual ContextInformation getContextInformation() = 0;
    virtual bool createGraphicsBackend() = 0;
    virtual ~GraphicsInterface() {};

    virtual void attachModelUBO(const uint32_t program) = 0;
    virtual void attachMaterialUBO(const uint32_t program, const uint32_t materialID) = 0;

    virtual uint32_t getNextMaterialIndex() = 0;

    virtual void initializeProgramAsset(const uint32_t programId,
                                        std::unordered_map<std::string, std::shared_ptr<Uniform>> &uniformMap, std::unordered_map<std::string, uint32_t> &attributesMap,
                                        std::unordered_map<std::string, std::pair<Uniform::VariableTypes, FrameBufferAttachPoints>> &outputMap) = 0;
    virtual void destroyProgram(uint32_t programID) = 0;

    virtual void bufferVertexData(const std::vector<glm::vec3> &vertices,
                          const std::vector<glm::mediump_uvec3> &faces,
                          uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer, uint32_t &ebo) = 0;
    virtual void bufferNormalData(const std::vector<glm::vec3> &colors,
                                  uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer) = 0;
    virtual void bufferExtraVertexData(const std::vector<glm::vec4> &extraData,
                                       uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer) = 0;
    virtual void bufferExtraVertexData(const std::vector<glm::lowp_uvec4> &extraData,
                                       uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer) = 0;
    virtual void bufferVertexTextureCoordinates(const std::vector<glm::vec2> &textureCoordinates,
                                                uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer) = 0;
    virtual void updateVertexData(const std::vector<glm::vec3> &vertices, const std::vector<glm::mediump_uvec3> &faces,
                                  uint32_t &vbo, uint32_t &ebo) = 0;
    virtual void updateNormalData(const std::vector<glm::vec3> &colors, uint32_t &vbo) = 0;
    virtual void updateExtraVertexData(const std::vector<glm::vec4> &extraData, uint32_t &vbo) = 0;
    virtual void updateExtraVertexData(const std::vector<glm::lowp_uvec4> &extraData, uint32_t &vbo) = 0;
    virtual void updateVertexTextureCoordinates(const std::vector<glm::vec2> &textureCoordinates, uint32_t &vbo) = 0;
    virtual bool freeBuffer(const uint32_t bufferID) = 0;

    virtual bool freeVAO(const uint32_t VAO) = 0;

    virtual void clearFrame() = 0;

    virtual void render(const uint32_t program, const uint32_t vao, const uint32_t ebo, const uint32_t elementCount) = 0;
    virtual void render(const uint32_t program, const uint32_t vao, const uint32_t ebo, const uint32_t elementCount, const uint32_t* startIndex) = 0;

    virtual void reshape() = 0;

    virtual uint32_t createFrameBuffer(uint32_t width, uint32_t height) = 0;
    virtual void deleteFrameBuffer(uint32_t frameBufferID) = 0;
    virtual void attachDrawTextureToFrameBuffer(uint32_t frameBufferID, TextureTypes textureType, uint32_t textureID,
                                        FrameBufferAttachPoints attachPoint, int32_t layer = 0 , bool clear = false) = 0;

    virtual void attachTexture(unsigned int textureID, unsigned int attachPoint) = 0;
    virtual void attach2DArrayTexture(unsigned int textureID, unsigned int attachPoint) = 0;
    virtual void attachCubeMap(unsigned int cubeMapID, unsigned int attachPoint) = 0;
    virtual void attachCubeMapArrayTexture(unsigned int textureID, unsigned int attachPoint) = 0;

    virtual bool getUniformLocation(const uint32_t programID, const std::string &uniformName, uint32_t &location) = 0;

    virtual const glm::mat4& getCameraMatrix() const = 0;
    virtual const glm::vec3& getCameraPosition() const = 0;
    virtual const glm::mat4& getProjectionMatrix() const  = 0;
    virtual const glm::mat4& getOrthogonalProjectionMatrix() const  = 0;

    virtual void createDebugVAOVBO(uint32_t &vao, uint32_t &vbo, uint32_t bufferSize) = 0;
    virtual void drawLines(GraphicsProgram &program, uint32_t vao, uint32_t vbo, const std::vector<Line> &lines) = 0;

    virtual void clearDepthBuffer() = 0; //FIXME this should be removed

    virtual bool setUniform(const uint32_t programID, const uint32_t uniformID, const glm::mat4 &matrix) = 0;
    virtual bool setUniform(const uint32_t programID, const uint32_t uniformID, const glm::vec3 &vector) = 0;
    virtual bool setUniform(const uint32_t programID, const uint32_t uniformID, const std::vector<glm::vec3> &vectorArray) = 0;
    virtual bool setUniform(const uint32_t programID, const uint32_t uniformID, const float value) = 0;
    virtual bool setUniform(const uint32_t programID, const uint32_t uniformID, const int value) = 0;
    virtual bool setUniformArray(const uint32_t programID, const uint32_t uniformID, const std::vector<glm::mat4> &matrixArray) = 0;

    virtual void setLight(const int lightIndex,
                          const glm::vec3& attenuation,
                          const glm::mat4* shadowMatrices,
                          const glm::mat4& lightSpaceMatrix,
                          const glm::vec3& position,
                          const glm::vec3& color,
                          const glm::vec3& ambientColor,
                          const int32_t lightType,
                          const float farPlane) = 0;

    virtual void removeLight(const int i) = 0;

    virtual void setPlayerMatrices(const glm::vec3 &cameraPosition, const glm::mat4 &cameraMatrix, long currentTime) = 0;

    virtual void switchRenderStage(uint32_t width, uint32_t height, uint32_t frameBufferID, bool blendEnabled, bool depthTestEnabled, bool depthWriteEnabled, bool scissorEnabled,
                                   bool clearColor, bool clearDepth, CullModes cullMode, std::map<uint32_t, std::shared_ptr<Texture>> &inputs, const std::string &name) = 0;
    virtual void switchRenderStage(uint32_t width, uint32_t height, uint32_t frameBufferID, bool blendEnabled, bool depthTestEnabled, bool depthWriteEnabled, bool scissorEnabled,
                                   bool clearColor, bool clearDepth, CullModes cullMode, const std::map<uint32_t, std::shared_ptr<Texture>> &inputs,
                                   const std::map<std::shared_ptr<Texture>,
                                           std::pair<FrameBufferAttachPoints, int>> &attachmentLayerMap, const std::string &name) = 0;

    virtual int getMaxTextureImageUnits() const = 0;

    virtual void setMaterial(const Material& material) = 0;
    virtual void setModel(const uint32_t modelID, const glm::mat4 &worldTransform) = 0;
    virtual void setModelIndexesUBO(const std::vector<uint32_t> &modelIndicesList) = 0;
    virtual void attachModelIndicesUBO(const uint32_t programID) = 0;

    virtual void renderInstanced(uint32_t program, uint32_t VAO, uint32_t EBO, uint32_t triangleCount,
                                 uint32_t instanceCount) = 0;

    virtual void renderInstanced(uint32_t program, uint32_t VAO, uint32_t EBO, uint32_t triangleCount, uint32_t startOffset,
                                 uint32_t instanceCount) = 0;

    virtual void setScissorRect(int32_t x, int32_t y, uint32_t width, uint32_t height) = 0;

    virtual void backupCurrentState() = 0;
    virtual void restoreLastState() = 0;

    virtual Options* getOptions() = 0;
};

#endif //LIMONENGINE_GRAPHICSINTERFACE_H
