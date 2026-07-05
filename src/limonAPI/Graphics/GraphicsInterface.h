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


#define NR_MAX_MODELS (4096)
#define NR_MAX_MATERIALS 200
#define NR_BONE 128

#include "limonAPI/Options.h"
#include "Uniform.h"
#include <sstream>
#include <iomanip>

class Material;

class Light;

class GraphicsProgram;
class Texture;
class RenderMethodInterface;


class GraphicsInterface {
    friend class GraphicsProgram; //TODO This is to allow access of protected method createGraphicsProgram we should come up with something better
public:

    enum class TextureTypes {T2D, T2D_ARRAY, TCUBE_MAP, TCUBE_MAP_ARRAY};//Starting with digits is illegal
    enum class InternalFormatTypes {RED, RG8, R32F, RGB, RGBA, RGB16F, RGBA16F, RGB32F, RGBA32F, DEPTH, COMPRESSED_RGB, COMPRESSED_RGBA };
    enum class FormatTypes {RED, RG, RGB, RGBA, DEPTH};
    enum class DataTypes {UNSIGNED_BYTE, FLOAT, HALF_FLOAT, UNSIGNED_SHORT, UNSIGNED_INT};
    enum class FrameBufferAttachPoints {NONE, COLOR0, COLOR1, COLOR2, COLOR3, COLOR4, COLOR5, COLOR6, DEPTH };
    enum class TextureWrapModes {NONE, REPEAT, BORDER, EDGE};
    enum class FilterModes {NEAREST, LINEAR, TRILINEAR};
    enum class CullModes {FRONT, BACK, NONE, NO_CHANGE};

    // Fixed engine-reserved texture-unit layout, shared so GraphicsProgram (which binds these engine-wide)
    // and the render pipeline builder (PipelineExtension, which assigns the remaining "pre_" inputs above
    // them) agree on what is already spoken for. The layout is contiguous, so it is fully described by its
    // boundaries: each constant is the FIRST unit of a band, and a band occupies [its start, the next
    // start). GraphicsInterface legitimately owns these because they map 1:1 to responsibilities it already
    // implements: setModel/setBoneTransforms (model/bone transform textures), setLight + the shadow passes
    // (shadow maps), and setMaterial (material samplers).
    //
    //   [1, 3)  model/bone transform : allModelTransformsTexture (unit 1), allBoneTransformsTexture (unit 2)
    //   [3, 5)  shadow maps          : pre_shadowDirectional (unit 3), pre_shadowPoint (unit 4)
    //   [5,10)  material samplers    : diffuse, ambient, specular, opacity, normal (units 5-9)
    //   10 ..   FIRST_ASSIGNABLE_TEXTURE_UNIT: first unit above the whole fixed region, free for the
    //           pipeline builder to hand out to a stage's ordinary "pre_" inputs.
    //
    // Every value is a small, FIXED, absolute unit counted up from unit 1 (unit 0 is a transient/default
    // working unit, see activateTextureUnit(0) in OpenGLGraphics.cpp) - never computed relative to
    // getMaxTextureImageUnits(). This is deliberate: these numbers get baked into the serialized pipeline
    // (GraphicsPipelineStage "Input Index" / GraphicsProgram "PresetValues" written by
    // PipelineExtension::buildRenderPipelineRecursive), and the machine that builds a pipeline is not
    // guaranteed to be the one that runs it, so a value anchored to any particular hardware maximum is only
    // as portable as that assumption; a small fixed number needs no such assumption at all.
    //
    // Model/bone transforms are reserved for EVERY program regardless of whether a shader declares the
    // uniforms, because they are bound once, outside any GraphicsPipelineStage's own "inputs" map, and
    // nothing re-establishes that binding per stage/frame - so nothing else may EVER touch units 1/2.
    // (The two are always declared together in the shared ModelRendering.vert header, so a single flag
    // tracks both - see GraphicsProgram::isModelBoneTransformUsed().) The shadow and material bands are
    // only actually reserved for programs that use them - see PipelineExtension's per-stage floor logic.
    static constexpr int32_t MODEL_BONE_TRANSFORM_TEXTURE_UNIT_START = 1;
    static constexpr int32_t SHADOW_MAP_TEXTURE_UNIT_START      = MODEL_BONE_TRANSFORM_TEXTURE_UNIT_START + 2; // +2: model, bone
    static constexpr int32_t MATERIAL_SAMPLER_TEXTURE_UNIT_START = SHADOW_MAP_TEXTURE_UNIT_START + 2;           // +2: directional, point
    static constexpr int32_t FIRST_ASSIGNABLE_TEXTURE_UNIT      = MATERIAL_SAMPLER_TEXTURE_UNIT_START + 5;      // +5: diffuse, ambient, specular, opacity, normal

    // NOTE (intentionally not enumerated here): a few units at the TOP of the range
    // (getMaxTextureImageUnits() - k) are used for transient, per-frame texture binds that are NOT
    // render-pipeline stage inputs - e.g. UI/overlay image samplers and particle sprites. Which specific
    // subsystem owns which top unit is NOT GraphicsInterface's concern and is deliberately kept out of
    // this low-level interface. Two rules the rest of the engine must follow, though: (1) such a transient
    // binder MUST use a high unit, never a small fixed one - a low fixed band (esp. model/bone at 1/2) is
    // bound once and would be silently clobbered, making objects flicker every other frame; (2) those top
    // units are set fresh right before each draw and are never baked into a pipeline file. A proper
    // allocator to formalize and enforce this coordination is planned (see the texture-unit design notes).

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
    virtual uint32_t createGraphicsProgram(const std::string &vertexShaderContent, const std::string &vertexShaderName, const std::string &geometryShaderContent, const std::string &geometryShaderName, const std::string &fragmentShaderContent, const std::string &fragmentShaderName) = 0;
public:

    static std::string formatShaderCode(const std::string& shaderCode) {
        std::istringstream shaderStream(shaderCode);
        std::ostringstream formattedStream;
        std::string line;
        int lineNum = 1;
        while (std::getline(shaderStream, line)) {
            formattedStream << std::setw(4) << lineNum++ << " | " << line << "\n";
        }
        return formattedStream.str();
    }

    struct ContextInformation {
        int SDL_GL_ACCELERATED_VISUAL = 1;
        int SDL_GL_CONTEXT_MAJOR_VERSION = 3;
        int SDL_GL_CONTEXT_MINOR_VERSION = 3;
        int SDL_GL_CONTEXT_PROFILE_MASK = 1;
        int SDL_GL_CONTEXT_FLAGS = 1;
        std::string shaderHeader;
    };

    virtual void getRenderTriangleAndLineCount(uint32_t& triangleCount, uint32_t& lineCount) = 0;
    explicit GraphicsInterface(OptionsUtil::Options *options [[gnu::unused]]) {};
    virtual ContextInformation getContextInformation() = 0;
    virtual bool getFallbackContextInformation(ContextInformation& fallbackContext) = 0;
    virtual bool verifyContext() = 0;
    virtual bool createGraphicsBackend() = 0;
    virtual ~GraphicsInterface() {};

    virtual void attachModelTexture(const uint32_t program) = 0;
    virtual void attachRigTexture(const uint32_t program) = 0;
    virtual void attachMaterialUBO(const uint32_t program) = 0;
    virtual void initializeProgramAsset(const uint32_t programId,
                                        std::unordered_map<std::string, std::shared_ptr<Uniform>> &uniformMap, std::unordered_map<std::string, uint32_t> &attributesMap,
                                        std::unordered_map<std::string, std::pair<Uniform::VariableTypes, FrameBufferAttachPoints>> &outputMap) = 0;
    virtual void destroyProgram(uint32_t programID) = 0;

    virtual void bufferVertexData(const std::vector<glm::vec3> &vertices,
                          const std::vector<glm::u16vec3> &faces,
                          uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer, uint32_t &ebo) = 0;
    virtual void bufferNormalData(const std::vector<glm::vec3> &colors,
                                  uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer) = 0;
    virtual void bufferExtraVertexData(const std::vector<glm::vec4> &extraData,
                                       uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer) = 0;
    virtual void bufferExtraVertexData(const std::vector<glm::lowp_uvec4> &extraData,
                                       uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer) = 0;
    virtual void bufferVertexTextureCoordinates(const std::vector<glm::vec2> &textureCoordinates,
                                                uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer) = 0;
    virtual void updateVertexData(const std::vector<glm::vec3> &vertices, const std::vector<glm::u16vec3> &faces,
                                  uint32_t &vbo, uint32_t &ebo) = 0;
    virtual void updateNormalData(const std::vector<glm::vec3> &colors, uint32_t &vbo) = 0;
    virtual void updateExtraVertexData(const std::vector<glm::vec4> &extraData, uint32_t &vbo) = 0;
    virtual void updateExtraVertexData(const std::vector<glm::lowp_uvec4> &extraData, uint32_t &vbo) = 0;
    virtual void updateVertexTextureCoordinates(const std::vector<glm::vec2> &textureCoordinates, uint32_t &vbo) = 0;
    virtual bool freeBuffer(const uint32_t bufferID) = 0;

    virtual bool freeVAO(const uint32_t VAO) = 0;

    virtual void clearFrame() = 0;

    virtual void render(const uint32_t program, const uint32_t vao, const uint32_t ebo, const uint32_t elementCount) = 0;
    virtual void render(const uint32_t program, const uint32_t vao, const uint32_t ebo, const uint32_t elementCount, const uint16_t *startIndex) = 0;

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

    virtual const glm::vec3& getCameraPosition() const = 0;

    virtual const glm::mat4& getGUIOrthogonalProjectionMatrix() const  = 0;

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
                          const std::vector<glm::mat4>& shadowMatrices,
                          const glm::vec3& position,
                          const glm::vec3& color,
                          const glm::vec3& ambientColor,
                          const int32_t lightType,
                          const float farPlane) = 0;

    virtual void removeLight(const int i) = 0;

    virtual void setPlayerMatrices(const glm::vec3 &cameraPosition, const glm::mat4 &cameraMatrix, const glm::mat4 &cameraProjection, long currentTime) = 0;

    virtual void switchRenderStage(uint32_t width, uint32_t height, uint32_t frameBufferID, bool blendEnabled, bool depthTestEnabled, bool depthWriteEnabled, bool scissorEnabled,
                                   bool clearColor, bool clearDepth, CullModes cullMode, std::map<uint32_t, std::shared_ptr<Texture>> &inputs, const std::string &name) = 0;
    virtual void switchRenderStage(uint32_t width, uint32_t height, uint32_t frameBufferID, bool blendEnabled, bool depthTestEnabled, bool depthWriteEnabled, bool scissorEnabled,
                                   bool clearColor, bool clearDepth, CullModes cullMode, const std::map<uint32_t, std::shared_ptr<Texture>> &inputs,
                                   const std::map<std::shared_ptr<Texture>,
                                           std::pair<FrameBufferAttachPoints, int>> &attachmentLayerMap, const std::string &name) = 0;

    virtual int getMaxTextureImageUnits() const = 0;

    virtual void setMaterial(const Material& material) = 0;
    virtual void setBoneTransforms(uint32_t index, const std::vector<glm::mat4>& boneTransforms) = 0;
    virtual void setModel(const uint32_t modelID, const glm::mat4 &worldTransform) = 0;
    virtual void setModelIndexesUBO(const std::vector<glm::uvec4> & modelIndicesList) = 0;
    virtual void attachModelIndicesUBO(const uint32_t programID) = 0;

    virtual void renderInstanced(uint32_t program, uint32_t VAO, uint32_t EBO, uint32_t triangleCount,
                                 uint32_t instanceCount) = 0;

    virtual void renderInstanced(uint32_t program, uint32_t VAO, uint32_t EBO, uint32_t triangleCount, uint32_t startOffset,
                                 uint32_t instanceCount) = 0;

    virtual void setScissorRect(int32_t x, int32_t y, uint32_t width, uint32_t height) = 0;

    virtual void backupCurrentState() = 0;
    virtual void restoreLastState() = 0;

    virtual OptionsUtil::Options* getOptions() = 0;

    // GPU profiling hooks. Default no-ops so backends without GPU profiling support compile unchanged.
    // Vulkan backend should override these using TracyVkZone / vkCmdWriteTimestamp internally.
    virtual void initGpuContext() {}
    virtual void beginGpuProfileZone(const char* name [[gnu::unused]], bool active [[gnu::unused]]) {}
    virtual void endGpuProfileZone() {}
    virtual void collectGpuProfilingData() {}
};

#endif //LIMONENGINE_GRAPHICSINTERFACE_H
