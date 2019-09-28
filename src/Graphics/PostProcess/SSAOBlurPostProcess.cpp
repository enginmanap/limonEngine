//
// Created by engin on 13.12.2018.
//

#include "SSAOBlurPostProcess.h"
#include "API/GraphicsProgram.h"

void SSAOBlurPostProcess::initializeProgram() {
    program = graphicsWrapper->createGraphicsProgram("./Engine/Shaders/SSAOBlur/vertex.glsl",
                                                     "./Engine/Shaders/SSAOBlur/fragment.glsl", false);
}

SSAOBlurPostProcess::SSAOBlurPostProcess(GraphicsInterface* graphicsWrapper) : QuadRenderBase(graphicsWrapper) {
    initializeProgram();
}
