//
// Created by engin on 13.12.2018.
//

#include "SSAOBlurPostProcess.h"
#include "../GLSLProgram.h"

void SSAOBlurPostProcess::initializeProgram() {
    program = glHelper->createGLSLProgram("./Engine/Shaders/SSAOBlur/vertex.glsl",
                              "./Engine/Shaders/SSAOBlur/fragment.glsl", false);
}

SSAOBlurPostProcess::SSAOBlurPostProcess(GLHelper *glHelper) : QuadRenderBase(glHelper) {
    initializeProgram();
}
