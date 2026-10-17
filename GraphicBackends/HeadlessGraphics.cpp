//
// Created by engin on 17.10.2026.
//

#include "HeadlessGraphics.h"

#include <iostream>

void createGraphicsBackend(OptionsUtil::Options* options, std::shared_ptr<GraphicsInterface>* backend) {
    *backend = std::make_shared<HeadlessGraphics>(options);
}

void HeadlessGraphics::exitNotImplemented(const char *methodName) {
    std::cerr << "HeadlessGraphics::" << methodName << " is not implemented in the headless backend. Exiting..." << std::endl;
    exit(-1);
}

//what loading assets uses: textures and meshes get ids, nothing is uploaded

uint32_t HeadlessGraphics::createTexture(int, int, TextureTypes, InternalFormatTypes, FormatTypes, DataTypes, uint32_t) {
    return nextID++;
}

bool HeadlessGraphics::deleteTexture(uint32_t) {
    return true;
}

void HeadlessGraphics::setWrapMode(uint32_t, TextureTypes, TextureWrapModes, TextureWrapModes, TextureWrapModes) {}

void HeadlessGraphics::setTextureBorder(uint32_t, TextureTypes, bool, const std::vector<float> &) {}

void HeadlessGraphics::setFilterMode(uint32_t, TextureTypes, FilterModes) {}

void HeadlessGraphics::loadTextureData(uint32_t, int, int, TextureTypes, InternalFormatTypes, FormatTypes, DataTypes, uint32_t,
                                       void *, void *, void *, void *, void *, void *) {}

void HeadlessGraphics::setTextureLabel(uint32_t, const std::string &) {}

void HeadlessGraphics::bufferVertexData(const std::vector<glm::vec3> &, const std::vector<glm::u16vec3> &, uint32_t &vao, uint32_t &vbo, const uint32_t, uint32_t &ebo) {
    vao = nextID++;
    vbo = nextID++;
    ebo = nextID++;
}

void HeadlessGraphics::bufferNormalData(const std::vector<glm::vec3> &, uint32_t &, uint32_t &vbo, const uint32_t) {
    vbo = nextID++;
}

void HeadlessGraphics::bufferExtraVertexData(const std::vector<glm::vec4> &, uint32_t &, uint32_t &vbo, const uint32_t) {
    vbo = nextID++;
}

void HeadlessGraphics::bufferExtraVertexData(const std::vector<glm::lowp_uvec4> &, uint32_t &, uint32_t &vbo, const uint32_t) {
    vbo = nextID++;
}

void HeadlessGraphics::bufferVertexTextureCoordinates(const std::vector<glm::vec2> &, uint32_t &, uint32_t &vbo, const uint32_t) {
    vbo = nextID++;
}

bool HeadlessGraphics::freeBuffer(const uint32_t) {
    return true;
}

bool HeadlessGraphics::freeVAO(const uint32_t) {
    return true;
}

void HeadlessGraphics::setMaterial(const Material &) {}

OptionsUtil::Options* HeadlessGraphics::getOptions() {
    return options;
}

//everything below needs a context, the headless backend has none

void HeadlessGraphics::updateTextureRegion(uint32_t, int, int, int, int, FormatTypes, DataTypes, const void *) {
    exitNotImplemented("updateTextureRegion");
}

uint32_t HeadlessGraphics::createGraphicsProgram(const std::string &, const std::string &, const std::string &, const std::string &, const std::string &, const std::string &) {
    exitNotImplemented("createGraphicsProgram");
}

const GraphicsInterface::RenderStats& HeadlessGraphics::getFrameStats() const {
    exitNotImplemented("getFrameStats");
}

const GraphicsInterface::RenderStats& HeadlessGraphics::getCurrentFrameStats() const {
    exitNotImplemented("getCurrentFrameStats");
}

void HeadlessGraphics::reportBatch(uint32_t) {
    exitNotImplemented("reportBatch");
}

GraphicsInterface::ContextInformation HeadlessGraphics::getContextInformation() {
    exitNotImplemented("getContextInformation");
}

std::string HeadlessGraphics::getShaderHeader() const {
    exitNotImplemented("getShaderHeader");
}

bool HeadlessGraphics::getFallbackContextInformation(ContextInformation &) {
    exitNotImplemented("getFallbackContextInformation");
}

bool HeadlessGraphics::verifyContext() {
    exitNotImplemented("verifyContext");
}

bool HeadlessGraphics::createGraphicsBackend() {
    exitNotImplemented("createGraphicsBackend");
}

void HeadlessGraphics::attachModelTexture(const uint32_t) {
    exitNotImplemented("attachModelTexture");
}

void HeadlessGraphics::attachRigTexture(const uint32_t) {
    exitNotImplemented("attachRigTexture");
}

void HeadlessGraphics::attachMaterialUBO(const uint32_t) {
    exitNotImplemented("attachMaterialUBO");
}

void HeadlessGraphics::initializeProgramAsset(const uint32_t, std::unordered_map<std::string, std::shared_ptr<Uniform>> &, std::unordered_map<std::string, uint32_t> &,
                                              std::unordered_map<std::string, std::pair<Uniform::VariableTypes, FrameBufferAttachPoints>> &) {
    exitNotImplemented("initializeProgramAsset");
}

void HeadlessGraphics::destroyProgram(uint32_t) {
    exitNotImplemented("destroyProgram");
}

void HeadlessGraphics::updateVertexData(const std::vector<glm::vec3> &, const std::vector<glm::u16vec3> &, uint32_t &, uint32_t &) {
    exitNotImplemented("updateVertexData");
}

void HeadlessGraphics::updateNormalData(const std::vector<glm::vec3> &, uint32_t &) {
    exitNotImplemented("updateNormalData");
}

void HeadlessGraphics::updateExtraVertexData(const std::vector<glm::vec4> &, uint32_t &) {
    exitNotImplemented("updateExtraVertexData");
}

void HeadlessGraphics::updateExtraVertexData(const std::vector<glm::lowp_uvec4> &, uint32_t &) {
    exitNotImplemented("updateExtraVertexData");
}

void HeadlessGraphics::updateVertexTextureCoordinates(const std::vector<glm::vec2> &, uint32_t &) {
    exitNotImplemented("updateVertexTextureCoordinates");
}

void HeadlessGraphics::clearFrame() {
    exitNotImplemented("clearFrame");
}

void HeadlessGraphics::render(const uint32_t, const uint32_t, const uint32_t, const uint32_t) {
    exitNotImplemented("render");
}

void HeadlessGraphics::render(const uint32_t, const uint32_t, const uint32_t, const uint32_t, const uint16_t *) {
    exitNotImplemented("render");
}

void HeadlessGraphics::reshape() {
    exitNotImplemented("reshape");
}

uint32_t HeadlessGraphics::createFrameBuffer(uint32_t, uint32_t) {
    exitNotImplemented("createFrameBuffer");
}

void HeadlessGraphics::deleteFrameBuffer(uint32_t) {
    exitNotImplemented("deleteFrameBuffer");
}

void HeadlessGraphics::attachDrawTextureToFrameBuffer(uint32_t, TextureTypes, uint32_t, FrameBufferAttachPoints, int32_t, bool) {
    exitNotImplemented("attachDrawTextureToFrameBuffer");
}

void HeadlessGraphics::attachTexture(unsigned int, unsigned int) {
    exitNotImplemented("attachTexture");
}

void HeadlessGraphics::attach2DArrayTexture(unsigned int, unsigned int) {
    exitNotImplemented("attach2DArrayTexture");
}

void HeadlessGraphics::attachCubeMap(unsigned int, unsigned int) {
    exitNotImplemented("attachCubeMap");
}

void HeadlessGraphics::attachCubeMapArrayTexture(unsigned int, unsigned int) {
    exitNotImplemented("attachCubeMapArrayTexture");
}

bool HeadlessGraphics::getUniformLocation(const uint32_t, const std::string &, uint32_t &) {
    exitNotImplemented("getUniformLocation");
}

const glm::mat4& HeadlessGraphics::getGUIOrthogonalProjectionMatrix() const {
    exitNotImplemented("getGUIOrthogonalProjectionMatrix");
}

void HeadlessGraphics::createDebugVAOVBO(uint32_t &, uint32_t &, uint32_t) {
    exitNotImplemented("createDebugVAOVBO");
}

void HeadlessGraphics::drawLines(GraphicsProgram &, uint32_t, uint32_t, const std::vector<Line> &) {
    exitNotImplemented("drawLines");
}

void HeadlessGraphics::clearDepthBuffer() {
    exitNotImplemented("clearDepthBuffer");
}

bool HeadlessGraphics::setUniform(const uint32_t, const uint32_t, const glm::mat4 &) {
    exitNotImplemented("setUniform");
}

bool HeadlessGraphics::setUniform(const uint32_t, const uint32_t, const glm::vec3 &) {
    exitNotImplemented("setUniform");
}

bool HeadlessGraphics::setUniform(const uint32_t, const uint32_t, const std::vector<glm::vec3> &) {
    exitNotImplemented("setUniform");
}

bool HeadlessGraphics::setUniform(const uint32_t, const uint32_t, const float) {
    exitNotImplemented("setUniform");
}

bool HeadlessGraphics::setUniform(const uint32_t, const uint32_t, const int) {
    exitNotImplemented("setUniform");
}

bool HeadlessGraphics::setUniformArray(const uint32_t, const uint32_t, const std::vector<glm::mat4> &) {
    exitNotImplemented("setUniformArray");
}

uint32_t HeadlessGraphics::createUniformBuffer() {
    exitNotImplemented("createUniformBuffer");
}

void HeadlessGraphics::deleteUniformBuffer(uint32_t) {
    exitNotImplemented("deleteUniformBuffer");
}

void HeadlessGraphics::writeUniformBuffer(uint32_t, const UniformBlockData &) {
    exitNotImplemented("writeUniformBuffer");
}

void HeadlessGraphics::bindUniformBuffer(uint32_t, uint32_t) {
    exitNotImplemented("bindUniformBuffer");
}

uint32_t HeadlessGraphics::createFrameBufferedTexture(int, int, InternalFormatTypes, FormatTypes, DataTypes) {
    exitNotImplemented("createFrameBufferedTexture");
}

void HeadlessGraphics::writeFrameBufferedTexture(uint32_t, int, int, int, int, FormatTypes, DataTypes, const void *) {
    exitNotImplemented("writeFrameBufferedTexture");
}

void HeadlessGraphics::deleteFrameBufferedTexture(uint32_t) {
    exitNotImplemented("deleteFrameBufferedTexture");
}

void HeadlessGraphics::swapFrameResources() {
    exitNotImplemented("swapFrameResources");
}

void HeadlessGraphics::switchRenderStage(uint32_t, uint32_t, uint32_t, bool, bool, bool, bool, bool, bool, CullModes, std::map<uint32_t, std::shared_ptr<Texture>> &, const std::string &) {
    exitNotImplemented("switchRenderStage");
}

void HeadlessGraphics::switchRenderStage(uint32_t, uint32_t, uint32_t, bool, bool, bool, bool, bool, bool, CullModes, const std::map<uint32_t, std::shared_ptr<Texture>> &,
                                         const std::map<std::shared_ptr<Texture>, std::pair<FrameBufferAttachPoints, int>> &, const std::string &) {
    exitNotImplemented("switchRenderStage");
}

int HeadlessGraphics::getMaxTextureImageUnits() const {
    exitNotImplemented("getMaxTextureImageUnits");
}

void HeadlessGraphics::setModelIndexesUBO(const std::vector<glm::uvec4> &) {
    exitNotImplemented("setModelIndexesUBO");
}

void HeadlessGraphics::attachModelIndicesUBO(const uint32_t) {
    exitNotImplemented("attachModelIndicesUBO");
}

uint32_t HeadlessGraphics::getModelIndexBatchCapacity() const {
    exitNotImplemented("getModelIndexBatchCapacity");
}

void HeadlessGraphics::renderInstanced(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {
    exitNotImplemented("renderInstanced");
}

void HeadlessGraphics::renderInstanced(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {
    exitNotImplemented("renderInstanced");
}

void HeadlessGraphics::setScissorRect(int32_t, int32_t, uint32_t, uint32_t) {
    exitNotImplemented("setScissorRect");
}

void HeadlessGraphics::backupCurrentState() {
    exitNotImplemented("backupCurrentState");
}

void HeadlessGraphics::restoreLastState() {
    exitNotImplemented("restoreLastState");
}
