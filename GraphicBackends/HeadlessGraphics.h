//
// Created by engin on 17.10.2026.
//

#ifndef LIMONENGINE_HEADLESSGRAPHICS_H
#define LIMONENGINE_HEADLESSGRAPHICS_H

#include <memory>
#include <string>
#include "limonAPI/Options.h"
#include "limonAPI/Graphics/GraphicsInterface.h"

extern "C" void createGraphicsBackend(OptionsUtil::Options* options, std::shared_ptr<GraphicsInterface>* backend);

//no window and no context, for --pack and --convert. Only what loading assets needs works, the rest exits with its name
class HeadlessGraphics : public GraphicsInterface {
    OptionsUtil::Options *options;
    uint32_t nextID = 1;//ids must stay unique, assets keep them as handles

    [[noreturn]] static void exitNotImplemented(const char *methodName);

protected:
    uint32_t createTexture(int height, int width, TextureTypes type, InternalFormatTypes internalFormat, FormatTypes format, DataTypes dataType, uint32_t textureLayers) override;
    bool deleteTexture(uint32_t textureID) override;
    void setWrapMode(uint32_t textureID, TextureTypes textureType, TextureWrapModes wrapModeS, TextureWrapModes wrapModeT, TextureWrapModes wrapModeR) override;
    void setTextureBorder(uint32_t textureID, TextureTypes textureType, bool isBorderColorSet, const std::vector<float> &borderColors) override;
    void setFilterMode(uint32_t textureID, TextureTypes textureType, FilterModes filterMode) override;
    void loadTextureData(uint32_t textureID, int height, int width, TextureTypes type, InternalFormatTypes internalFormat, FormatTypes format, DataTypes dataType, uint32_t depth,
                         void *data, void *data2, void *data3, void *data4, void *data5, void *data6) override;
    void updateTextureRegion(uint32_t textureID, int x, int y, int width, int height, FormatTypes format, DataTypes dataType, const void *data) override;
    uint32_t createGraphicsProgram(const std::string &vertexShaderContent, const std::string &vertexShaderName, const std::string &geometryShaderContent, const std::string &geometryShaderName,
                                   const std::string &fragmentShaderContent, const std::string &fragmentShaderName) override;

public:
    explicit HeadlessGraphics(OptionsUtil::Options *options) : GraphicsInterface(options), options(options) {}

    const RenderStats& getFrameStats() const override;
    const RenderStats& getCurrentFrameStats() const override;
    void reportBatch(uint32_t materialSwitchCount) override;
    ContextInformation getContextInformation() override;
    std::string getShaderHeader() const override;
    bool getFallbackContextInformation(ContextInformation &fallbackContext) override;
    bool verifyContext() override;
    bool createGraphicsBackend() override;

    void attachModelTexture(const uint32_t program) override;
    void attachRigTexture(const uint32_t program) override;
    void attachMaterialUBO(const uint32_t program) override;
    void initializeProgramAsset(const uint32_t programId, std::unordered_map<std::string, std::shared_ptr<Uniform>> &uniformMap, std::unordered_map<std::string, uint32_t> &attributesMap,
                                std::unordered_map<std::string, std::pair<Uniform::VariableTypes, FrameBufferAttachPoints>> &outputMap) override;
    void destroyProgram(uint32_t programID) override;

    void bufferVertexData(const std::vector<glm::vec3> &vertices, const std::vector<glm::u16vec3> &faces, uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer, uint32_t &ebo) override;
    void bufferNormalData(const std::vector<glm::vec3> &colors, uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer) override;
    void bufferExtraVertexData(const std::vector<glm::vec4> &extraData, uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer) override;
    void bufferExtraVertexData(const std::vector<glm::lowp_uvec4> &extraData, uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer) override;
    void bufferVertexTextureCoordinates(const std::vector<glm::vec2> &textureCoordinates, uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer) override;
    void updateVertexData(const std::vector<glm::vec3> &vertices, const std::vector<glm::u16vec3> &faces, uint32_t &vbo, uint32_t &ebo) override;
    void updateNormalData(const std::vector<glm::vec3> &colors, uint32_t &vbo) override;
    void updateExtraVertexData(const std::vector<glm::vec4> &extraData, uint32_t &vbo) override;
    void updateExtraVertexData(const std::vector<glm::lowp_uvec4> &extraData, uint32_t &vbo) override;
    void updateVertexTextureCoordinates(const std::vector<glm::vec2> &textureCoordinates, uint32_t &vbo) override;
    bool freeBuffer(const uint32_t bufferID) override;
    bool freeVAO(const uint32_t VAO) override;

    void clearFrame() override;
    void render(const uint32_t program, const uint32_t vao, const uint32_t ebo, const uint32_t elementCount) override;
    void render(const uint32_t program, const uint32_t vao, const uint32_t ebo, const uint32_t elementCount, const uint16_t *startIndex) override;
    void reshape() override;

    uint32_t createFrameBuffer(uint32_t width, uint32_t height) override;
    void deleteFrameBuffer(uint32_t frameBufferID) override;
    void attachDrawTextureToFrameBuffer(uint32_t frameBufferID, TextureTypes textureType, uint32_t textureID, FrameBufferAttachPoints attachPoint, int32_t layer, bool clear) override;

    void attachTexture(unsigned int textureID, unsigned int attachPoint) override;
    void attach2DArrayTexture(unsigned int textureID, unsigned int attachPoint) override;
    void attachCubeMap(unsigned int cubeMapID, unsigned int attachPoint) override;
    void attachCubeMapArrayTexture(unsigned int textureID, unsigned int attachPoint) override;
    void setTextureLabel(uint32_t textureID, const std::string &label) override;

    bool getUniformLocation(const uint32_t programID, const std::string &uniformName, uint32_t &location) override;
    const glm::mat4& getGUIOrthogonalProjectionMatrix() const override;
    void createDebugVAOVBO(uint32_t &vao, uint32_t &vbo, uint32_t bufferSize) override;
    void drawLines(GraphicsProgram &program, uint32_t vao, uint32_t vbo, const std::vector<Line> &lines) override;
    void clearDepthBuffer() override;

    bool setUniform(const uint32_t programID, const uint32_t uniformID, const glm::mat4 &matrix) override;
    bool setUniform(const uint32_t programID, const uint32_t uniformID, const glm::vec3 &vector) override;
    bool setUniform(const uint32_t programID, const uint32_t uniformID, const std::vector<glm::vec3> &vectorArray) override;
    bool setUniform(const uint32_t programID, const uint32_t uniformID, const float value) override;
    bool setUniform(const uint32_t programID, const uint32_t uniformID, const int value) override;
    bool setUniformArray(const uint32_t programID, const uint32_t uniformID, const std::vector<glm::mat4> &matrixArray) override;

    uint32_t createUniformBuffer() override;
    void deleteUniformBuffer(uint32_t bufferID) override;
    void writeUniformBuffer(uint32_t bufferID, const UniformBlockData &data) override;
    void bindUniformBuffer(uint32_t bufferID, uint32_t bindingPoint) override;
    uint32_t createFrameBufferedTexture(int height, int width, InternalFormatTypes internalFormat, FormatTypes format, DataTypes dataType) override;
    void writeFrameBufferedTexture(uint32_t textureID, int x, int y, int width, int height, FormatTypes format, DataTypes dataType, const void *data) override;
    void deleteFrameBufferedTexture(uint32_t textureID) override;
    void swapFrameResources() override;

    void switchRenderStage(uint32_t width, uint32_t height, uint32_t frameBufferID, bool blendEnabled, bool depthTestEnabled, bool depthWriteEnabled, bool scissorEnabled, bool framebufferSRGBEnabled,
                           bool clearColor, bool clearDepth, CullModes cullMode, std::map<uint32_t, std::shared_ptr<Texture>> &inputs, const std::string &name) override;
    void switchRenderStage(uint32_t width, uint32_t height, uint32_t frameBufferID, bool blendEnabled, bool depthTestEnabled, bool depthWriteEnabled, bool scissorEnabled, bool framebufferSRGBEnabled,
                           bool clearColor, bool clearDepth, CullModes cullMode, const std::map<uint32_t, std::shared_ptr<Texture>> &inputs,
                           const std::map<std::shared_ptr<Texture>, std::pair<FrameBufferAttachPoints, int>> &attachmentLayerMap, const std::string &name) override;

    int getMaxTextureImageUnits() const override;
    void setMaterial(const Material &material) override;
    void setModelIndexesUBO(const std::vector<glm::uvec4> &modelIndicesList) override;
    void attachModelIndicesUBO(const uint32_t programID) override;
    uint32_t getModelIndexBatchCapacity() const override;
    void renderInstanced(uint32_t program, uint32_t VAO, uint32_t EBO, uint32_t elementCount, uint32_t instanceCount) override;
    void renderInstanced(uint32_t program, uint32_t VAO, uint32_t EBO, uint32_t elementCount, uint32_t startOffset, uint32_t instanceCount) override;
    void setScissorRect(int32_t x, int32_t y, uint32_t width, uint32_t height) override;
    void backupCurrentState() override;
    void restoreLastState() override;

    OptionsUtil::Options* getOptions() override;
};

#endif //LIMONENGINE_HEADLESSGRAPHICS_H
