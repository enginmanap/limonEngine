//
// Created by engin on 12.12.2018.
//

#include "SSAOPostProcess.h"
#include "../GLSLProgram.h"
#include <random>
#include <glm/glm.hpp>

void SSAOPostProcess::initializeProgram() {
    program = glHelper->createGLSLProgram("./Engine/Shaders/SSAOGeneration/vertex.glsl",
                              "./Engine/Shaders/SSAOGeneration/fragment.glsl", false);
}

float lerp(float first , float second , float factor ) {
    return first + factor * (second - first);
}

void SSAOPostProcess::generateAndSetSSAOKernels(uint32_t kernelSize) const {
// generate sample kernel
// ----------------------
    std::vector<glm::vec3> ssaoKernel;
    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
    std::default_random_engine generator;
    for (unsigned int i = 0; i < kernelSize; ++i){
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
        sample = glm::normalize(sample);
        //sample *= randomFloats(generator);
        float scale = float(i) / kernelSize;

        // scale samples s.t. they're more aligned to center of kernel
        scale = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
        //std::cout << "sampleKernel" << glm::to_string(sample) << std::endl;
    }

    if(!this->program->setUniform("ssaoKernel[0]", ssaoKernel)) {
        std::cerr << "uniform variable \"ssaoKernel\" couldn't be set" << std::endl;
    }
    if(!this->program->setUniform("ssaoSampleCount", (int32_t)ssaoKernel.size())) {
        std::cerr << "uniform variable \"ssaoSampleCount\" couldn't be set" << std::endl;
    }
}