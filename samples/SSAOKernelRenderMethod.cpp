//
// Created by engin on 23.09.2019.
//

#include <random>
#include <algorithm>
#include "SSAOKernelRenderMethod.h"
#include "WaterRenderMethod.h"


float lerp(float first , float second , float factor ) {
    return first + factor * (second - first);
}

std::vector<glm::vec3> SSAOKernelRenderMethod::generateSSAOKernels(uint32_t kernelSize) {
// generate sample kernel
// ----------------------
    std::vector<glm::vec3> ssaoKernel;
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
    std::default_random_engine generator;
    for (unsigned int i = 0; i < kernelSize; ++i){
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = float(i) / kernelSize;

        // scale samples s.t. they're more aligned to center of kernel
        scale = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
        //std::cout << "sampleKernel" << glm::to_string(sample) << std::endl;
    }
    return ssaoKernel;
}


bool SSAOKernelRenderMethod::initRender(std::shared_ptr<GraphicsProgram> program, std::vector<LimonTypes::GenericParameter> parameters [[gnu::unused]]) {
    setupQuad();
    // The shader loops over ssao_sampleCount at compile time, so the kernel has to come from the same option, a different count would read unset kernel entries
    OptionsUtil::Options* options = graphicsInterface->getOptions();
    long sampleCount = options->getOption<long>(options->getHash("ssao_sampleCount")).getOrDefault(9);
    std::vector<glm::vec3> kernels = generateSSAOKernels(sampleCount);
    if(!program->setUniform("ssaoKernel[0]", kernels)) {
        std::cerr << "uniform variable \"ssaoKernel\" couldn't be set" << std::endl;
    }
    generateSSAONoiseTexture();
    return false;
}

void SSAOKernelRenderMethod::setupQuad() {
    std::vector<glm::vec3> vertices;

    vertices.emplace_back( -1.0f,  1.0f, 0.0f);
    vertices.emplace_back( -1.0f, -1.0f, 0.0f);
    vertices.emplace_back( 1.0f,  1.0f, 0.0f);
    vertices.emplace_back( 1.0f, -1.0f, 0.0f);

    std::vector<glm::u16vec3> faces;

    faces.emplace_back(glm::u16vec3(0,1,2));
    faces.emplace_back(glm::u16vec3(2,1,3));

    std::vector<glm::vec2> textureCoordinates;

    textureCoordinates.emplace_back(0.0f, 1.0f);
    textureCoordinates.emplace_back(0.0f, 0.0f);
    textureCoordinates.emplace_back(1.0f, 1.0f);
    textureCoordinates.emplace_back(1.0f, 0.0f);

    uint32_t vbo;
    graphicsInterface->bufferVertexData(vertices, faces, vao, vbo, 1, ebo);
    bufferObjects.push_back(vbo);

    graphicsInterface->bufferVertexTextureCoordinates(textureCoordinates, vao, vbo, 2);
    bufferObjects.push_back(vbo);
}

void
SSAOKernelRenderMethod::generateSSAONoiseTexture() {
    // Each AO texel rotates the kernel by its texel of this tile, and the blur averages exactly one tile to cancel the noise.
    // The tile covers ~4x4 screen pixels at any AO resolution (4x4 texels at full, 2x2 at half): a fixed 4x4 at half
    // resolution makes the blur span 8x8 screen pixels and visibly smears detail.
    // Same formula as SSAO_ROTATION_TILE in the SSAOBlur fragment shader, change both
    OptionsUtil::Options* options = graphicsInterface->getOptions();
    long ssaoWidth = options->getOption<long>(options->getHash("ssao_width")).getOrDefault(960);
    long displayWidth = options->getOption<long>(options->getHash("display_width")).getOrDefault(1920);
    long tileSize = std::clamp((4 * ssaoWidth + displayWidth / 2) / std::max(displayWidth, 1L), 1L, 4L);

    std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
    std::default_random_engine generator;
    std::vector<glm::vec3> ssaoNoise;
    for (long i = 0; i < tileSize * tileSize; i++) {
        glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
        ssaoNoise.push_back(glm::normalize(noise));
    }
    ssaoNoiseTexture = createTexture(tileSize, tileSize, GraphicsInterface::TextureTypes::T2D, GraphicsInterface::InternalFormatTypes::RGB32F, GraphicsInterface::FormatTypes::RGB, GraphicsInterface::DataTypes::FLOAT, 0);
    setFilterMode(ssaoNoiseTexture, GraphicsInterface::TextureTypes::T2D, GraphicsInterface::FilterModes::NEAREST);
    setWrapMode(ssaoNoiseTexture, GraphicsInterface::TextureTypes::T2D,
                GraphicsInterface::TextureWrapModes::REPEAT, GraphicsInterface::TextureWrapModes::REPEAT, GraphicsInterface::TextureWrapModes::REPEAT);
    loadTextureData(ssaoNoiseTexture, tileSize, tileSize, GraphicsInterface::TextureTypes::T2D, GraphicsInterface::InternalFormatTypes::RGB32F, GraphicsInterface::FormatTypes::RGB, GraphicsInterface::DataTypes::FLOAT, 0, ssaoNoise.data(),
                    nullptr, nullptr, nullptr, nullptr, nullptr);
    }

bool SSAOKernelRenderMethod::cleanupRender(std::shared_ptr<GraphicsProgram> program[[gnu::unused]], std::vector<LimonTypes::GenericParameter> parameters [[gnu::unused]]) {
    deleteTexture(ssaoNoiseTexture);
    ssaoNoiseTexture = 0;
    return true;
}



void registerRenderMethodTypes(std::map<std::string, RenderMethodInterface::Registration>* renderMethodMap) {
    (*renderMethodMap)["SSAOKernelRenderMethod"] = RenderMethodInterface::registration<SSAOKernelRenderMethod>();
    (*renderMethodMap)["WaterRenderMethod"] = RenderMethodInterface::registration<WaterRenderMethod>();
}

