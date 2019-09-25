//
// Created by Engin Manap on 10.02.2016.
//

#ifndef LIMONENGINE_GRAPHICSINTERFACE_H
#define LIMONENGINE_GRAPHICSINTERFACE_H


#include <GL/glew.h>

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#else

#include <GL/gl.h>
#endif/*__APPLE__*/

#include <memory>
#include <map>
#include <unordered_map>
#include <vector>


#define NR_POINT_LIGHTS 3
#define NR_TOTAL_LIGHTS 4
#define NR_MAX_MODELS (1000)
#define NR_MAX_MATERIALS 2000

#include "Options.h"
class Material;

class Light;

class GraphicsProgram;
class Texture;

struct Line {
    glm::vec3 from;
    glm::vec3 fromColor;
    int needsCameraTransform;//FIXME This variable is repeated, because it must be per vertex. Maybe we can share int as bytes.
    glm::vec3 to;
    glm::vec3 toColor;
    int needsCameraTransform2;//this is the other one

    Line(const glm::vec3 &from,
         const glm::vec3 &to,
         const glm::vec3 &fromColor,
         const glm::vec3 &toColor,
         const bool needsCameraTransform): from(from), fromColor(fromColor), needsCameraTransform(needsCameraTransform), to(to), toColor(toColor), needsCameraTransform2(needsCameraTransform){};
};

class GraphicsInterface {
public:

    enum class TextureTypes {T2D, T2D_ARRAY, TCUBE_MAP, TCUBE_MAP_ARRAY};//Starting with digits is illegal
    enum class InternalFormatTypes {RED, RGB, RGBA, RGB16F, RGB32F, DEPTH };
    enum class FormatTypes {RED, RGB, RGBA, DEPTH};
    enum class DataTypes {UNSIGNED_BYTE, FLOAT};
    enum class FrameBufferAttachPoints {NONE, COLOR0, COLOR1, COLOR2, COLOR3, COLOR4, COLOR5, COLOR6, DEPTH };
    enum class TextureWrapModes {NONE, REPEAT, BORDER, EDGE};
    enum class FilterModes {NEAREST, LINEAR, TRILINEAR};
    enum class CullModes {FRONT, BACK, NONE, NO_CHANGE};
    enum VariableTypes {
        INT,
        FLOAT,
        FLOAT_VEC2,
        FLOAT_VEC3,
        FLOAT_VEC4,
        FLOAT_MAT4,
        CUBEMAP,
        CUBEMAP_ARRAY,
        TEXTURE_2D,
        TEXTURE_2D_ARRAY,
        UNDEFINED
    };

    class Uniform{
    public:
        unsigned int location;
        std::string name;
        VariableTypes type;
        unsigned int size;

        Uniform(unsigned int location, const std::string &name, GLenum typeEnum, unsigned int size) : location(
                location), name(name), size(size) {
            switch (typeEnum) {
                case GL_SAMPLER_CUBE:
                    type = CUBEMAP;
                    break;
                case GL_SAMPLER_CUBE_MAP_ARRAY_ARB:
                    type = CUBEMAP_ARRAY;
                    break;
                case GL_SAMPLER_2D:
                    type = TEXTURE_2D;
                    break;
                case GL_SAMPLER_2D_ARRAY:
                    type = TEXTURE_2D_ARRAY;
                    break;
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

    enum FrustumSide
    {
        RIGHT	= 0,		// The RIGHT side of the frustum
        LEFT	= 1,		// The LEFT	 side of the frustum
        BOTTOM	= 2,		// The BOTTOM side of the frustum
        TOP		= 3,		// The TOP side of the frustum
        BACK	= 4,		// The BACK	side of the frustum
        FRONT	= 5			// The FRONT side of the frustum
    };

protected:
    friend class Texture;
    virtual uint32_t createTexture(int height, int width, TextureTypes type, InternalFormatTypes internalFormat, FormatTypes format, DataTypes dataType, uint32_t depth) = 0;

    virtual bool deleteTexture(GLuint textureID) = 0;

    virtual void setWrapMode(Texture& texture, TextureWrapModes wrapModeS, TextureWrapModes wrapModeT, TextureWrapModes wrapModeR) = 0;

    virtual void setTextureBorder(Texture& texture) = 0;

    virtual void setFilterMode(Texture& texture, FilterModes filterMode) = 0;

    virtual void loadTextureData(uint32_t textureID, int height, int width, TextureTypes type, InternalFormatTypes internalFormat, FormatTypes format, DataTypes dataType, uint32_t depth,
                         void *data, void *data2, void *data3, void *data4, void *data5, void *data6) = 0;

public:

    virtual const std::map<std::shared_ptr<GraphicsProgram>, int> &getLoadedPrograms() const = 0;

    virtual void getRenderTriangleAndLineCount(uint32_t& triangleCount, uint32_t& lineCount) = 0;
    virtual const glm::mat4 &getLightProjectionMatrixPoint() const = 0;
    virtual const glm::mat4 &getLightProjectionMatrixDirectional() const = 0;
    explicit GraphicsInterface(Options *options [[gnu::unused]]) {};
    virtual ~GraphicsInterface() {};

    virtual std::shared_ptr<GraphicsProgram> createGLSLProgram(const std::string &vertexShader, const std::string &geometryShader, const std::string &fragmentShader, bool isMaterialUsed) = 0;
    virtual std::shared_ptr<GraphicsProgram> createGLSLProgram(const std::string &vertexShader, const std::string &fragmentShader, bool isMaterialUsed) = 0;

    virtual void attachModelUBO(const uint32_t program) = 0;
    virtual void attachMaterialUBO(const uint32_t program, const uint32_t materialID) = 0;

    virtual uint32_t getNextMaterialIndex() = 0;

    virtual GLuint initializeProgram(const std::string &vertexShaderFile, const std::string &geometryShaderFile, const std::string &fragmentShaderFile,
                             std::unordered_map<std::string,const Uniform *> &uniformMap, std::unordered_map<std::string, VariableTypes> &outputMap) = 0;
    virtual void destroyProgram(uint32_t programID) = 0;

    virtual void bufferVertexData(const std::vector<glm::vec3> &vertices,
                          const std::vector<glm::mediump_uvec3> &faces,
                          uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer, uint_fast32_t &ebo) = 0;
    virtual void bufferNormalData(const std::vector<glm::vec3> &colors,
                          uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer) = 0;
    virtual void bufferExtraVertexData(const std::vector<glm::vec4> &extraData,
                               uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer) = 0;
    virtual void bufferExtraVertexData(const std::vector<glm::lowp_uvec4> &extraData,
                               uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer) = 0;
    virtual void bufferVertexTextureCoordinates(const std::vector<glm::vec2> &textureCoordinates,
                                        uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer) = 0;
    virtual bool freeBuffer(const GLuint bufferID) = 0;

    virtual bool freeVAO(const GLuint VAO) = 0;

    virtual void clearFrame() = 0;

    virtual void render(const GLuint program, const GLuint vao, const GLuint ebo, const GLuint elementCount) = 0;

    virtual void reshape() = 0;

    virtual uint32_t createFrameBuffer(uint32_t width, uint32_t height) = 0;
    virtual void deleteFrameBuffer(uint32_t frameBufferID) = 0;
    virtual void attachDrawTextureToFrameBuffer(uint32_t frameBufferID, TextureTypes textureType, uint32_t textureID,
                                        FrameBufferAttachPoints attachPoint, int32_t layer = 0 , bool clear = false) = 0;

    virtual void attachTexture(unsigned int textureID, unsigned int attachPoint) = 0;
    virtual void attachCubeMap(unsigned int cubeMapID, unsigned int attachPoint) = 0;

    virtual bool getUniformLocation(const GLuint programID, const std::string &uniformName, GLuint &location) = 0;

    virtual const glm::mat4& getCameraMatrix() const = 0;

    virtual const glm::vec3& getCameraPosition() const = 0;

    virtual const glm::mat4& getProjectionMatrix() const  = 0;

    virtual const glm::mat4& getOrthogonalProjectionMatrix() const  = 0;

    virtual void createDebugVAOVBO(uint32_t &vao, uint32_t &vbo, uint32_t bufferSize) = 0;

    virtual void drawLines(GraphicsProgram &program, uint32_t vao, uint32_t vbo, const std::vector<Line> &lines) = 0;

    virtual void clearDepthBuffer() = 0; //FIXME this should be removed

    virtual bool setUniform(const GLuint programID, const GLuint uniformID, const glm::mat4 &matrix) = 0;
    virtual bool setUniform(const GLuint programID, const GLuint uniformID, const glm::vec3 &vector) = 0;
    virtual bool setUniform(const GLuint programID, const GLuint uniformID, const std::vector<glm::vec3> &vectorArray) = 0;
    virtual bool setUniform(const GLuint programID, const GLuint uniformID, const float value) = 0;
    virtual bool setUniform(const GLuint programID, const GLuint uniformID, const int value) = 0;
    virtual bool setUniformArray(const GLuint programID, const GLuint uniformID, const std::vector<glm::mat4> &matrixArray) = 0;

    virtual void setLight(const Light &light, const int i) = 0;

    virtual void removeLight(const int i) = 0;

    virtual void setPlayerMatrices(const glm::vec3 &cameraPosition, const glm::mat4 &cameraMatrix) = 0;

    virtual void switchRenderStage(uint32_t width, uint32_t height, uint32_t frameBufferID, bool blendEnabled, bool clearColor, bool clearDepth, CullModes cullMode,
                               std::map<uint32_t, std::shared_ptr<Texture>> &inputs) = 0;
    virtual void switchRenderStage(uint32_t width, uint32_t height, uint32_t frameBufferID, bool blendEnabled, bool clearColor, bool clearDepth, CullModes cullMode,
                               const std::map<uint32_t, std::shared_ptr<Texture>> &inputs,
                               const std::map<std::shared_ptr<Texture>, std::pair<FrameBufferAttachPoints, int>> &attachmentLayerMap) = 0;

    virtual int getMaxTextureImageUnits() const = 0;

    virtual void calculateFrustumPlanes(const glm::mat4 &cameraMatrix, const glm::mat4 &projectionMatrix,
                                std::vector<glm::vec4> &planes) const = 0;
    virtual  bool isInFrustum(const glm::vec3& aabbMin, const glm::vec3& aabbMax) const = 0;
    virtual bool isInFrustum(const glm::vec3& aabbMin, const glm::vec3& aabbMax, const std::vector<glm::vec4>& frustumPlaneVector) const = 0;

    virtual void setMaterial(std::shared_ptr<const Material>material) = 0;
    virtual void setModel(const uint32_t modelID, const glm::mat4 &worldTransform) = 0;
    virtual void setModelIndexesUBO(std::vector<uint32_t> &modelIndicesList) = 0;
    virtual void attachModelIndicesUBO(const uint32_t programID) = 0;

    virtual void renderInstanced(GLuint program, uint_fast32_t VAO, uint_fast32_t EBO, uint_fast32_t triangleCount,
                         uint32_t instanceCount) = 0;
};

#endif //LIMONENGINE_GRAPHICSINTERFACE_H
