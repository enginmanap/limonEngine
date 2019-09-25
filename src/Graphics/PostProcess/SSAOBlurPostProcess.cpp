//
// Created by engin on 13.12.2018.
//

#include "SSAOBlurPostProcess.h"
#include "Graphics/GLSLProgram.h"

void SSAOBlurPostProcess::initializeProgram() {
    program = graphicsWrapper->createGLSLProgram("./Engine/Shaders/SSAOBlur/vertex.glsl",
                              "./Engine/Shaders/SSAOBlur/fragment.glsl", false);
}

SSAOBlurPostProcess::SSAOBlurPostProcess(GraphicsInterface* graphicsWrapper) : QuadRenderBase(graphicsWrapper) {
    initializeProgram();
}
