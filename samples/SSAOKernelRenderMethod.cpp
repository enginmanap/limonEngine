//
// Created by engin on 23.09.2019.
//

#include <random>
#include "SSAOKernelRenderMethod.h"


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
    long sampleCount;
    OptionsUtil::Options::Option<long> sampleCountOption = graphicsInterface->getOptions()->getOption<long>(graphicsInterface->getOptions()->getHash("SSAOSampleCount"));
    sampleCount = sampleCountOption.getOrDefault(9);
    std::vector<glm::vec3> kernels = generateSSAOKernels(sampleCount);
    if(!program->setUniform("ssaoKernel[0]", kernels)) {
        std::cerr << "uniform variable \"ssaoKernel\" couldn't be set" << std::endl;
    }
    if(!program->setUniform("ssaoSampleCount", (int32_t)kernels.size())) {
        std::cerr << "uniform variable \"ssaoSampleCount\" couldn't be set" << std::endl;
    }
    generateSSAONoiseTexture();
    if(!program->setUniform("ssaoNoiseSampler", graphicsInterface->getMaxTextureImageUnits()-3)) {
        std::cerr << "uniform variable \"ssaoNoiseSampler\" couldn't be set" << std::endl;
    }
    return false;
}

void SSAOKernelRenderMethod::setupQuad() {
    std::vector<glm::vec3> vertices;

    vertices.emplace_back( -1.0f,  1.0f, 0.0f);
    vertices.emplace_back( -1.0f, -1.0f, 0.0f);
    vertices.emplace_back(glm::vec3(  1.0f,  1.0f, 0.0f));
    vertices.emplace_back(glm::vec3(  1.0f, -1.0f, 0.0f));

    std::vector<glm::mediump_uvec3> faces;

    faces.emplace_back(glm::mediump_uvec3(0,1,2));
    faces.emplace_back(glm::mediump_uvec3(2,1,3));

    std::vector<glm::vec2> textureCoordinates;

    textureCoordinates.emplace_back(glm::vec2(0.0f, 1.0f));
    textureCoordinates.emplace_back(glm::vec2(0.0f, 0.0f));
    textureCoordinates.emplace_back(glm::vec2(1.0f, 1.0f));
    textureCoordinates.emplace_back(glm::vec2(1.0f, 0.0f));

    uint32_t vbo;
    graphicsInterface->bufferVertexData(vertices, faces, vao, vbo, 1, ebo);
    bufferObjects.push_back(vbo);

    graphicsInterface->bufferVertexTextureCoordinates(textureCoordinates, vao, vbo, 2);
    bufferObjects.push_back(vbo);
}

void
SSAOKernelRenderMethod::generateSSAONoiseTexture() {
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
    std::default_random_engine generator;
    // generate noise texture
// ----------------------
    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
        ssaoNoise.push_back(glm::normalize(noise));
    }
    ssaoNoiseTexture = createTexture(4, 4, GraphicsInterface::TextureTypes::T2D, GraphicsInterface::InternalFormatTypes::RGB32F, GraphicsInterface::FormatTypes::RGB, GraphicsInterface::DataTypes::FLOAT, 0);
    setFilterMode(ssaoNoiseTexture, GraphicsInterface::TextureTypes::T2D, GraphicsInterface::FilterModes::LINEAR);
    setWrapMode(ssaoNoiseTexture, GraphicsInterface::TextureTypes::T2D,
                GraphicsInterface::TextureWrapModes::REPEAT, GraphicsInterface::TextureWrapModes::REPEAT, GraphicsInterface::TextureWrapModes::REPEAT);
    loadTextureData(ssaoNoiseTexture, 4, 4, GraphicsInterface::TextureTypes::T2D, GraphicsInterface::InternalFormatTypes::RGB32F, GraphicsInterface::FormatTypes::RGB, GraphicsInterface::DataTypes::FLOAT, 0, ssaoNoise.data(),
                    nullptr, nullptr, nullptr, nullptr, nullptr);
    }

bool SSAOKernelRenderMethod::cleanupRender(std::shared_ptr<GraphicsProgram> program[[gnu::unused]], std::vector<LimonTypes::GenericParameter> parameters [[gnu::unused]]) {
    deleteTexture(ssaoNoiseTexture);
    ssaoNoiseTexture = 0;
    return true;
}



void registerRenderMethods(std::map<std::string, RenderMethodInterface*(*)(GraphicsInterface*)>* renderMethodMap) {
    (*renderMethodMap)["SSAOKernelRenderMethod"] = &createT<SSAOKernelRenderMethod>;
}

