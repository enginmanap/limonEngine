//
// Created by engin on 12.12.2018.
//

#include "CombinePostProcess.h"
#include "Graphics/GraphicsProgram.h"

CombinePostProcess::CombinePostProcess(GraphicsInterface* graphicsWrapper, bool isSSAOEnabled) : QuadRenderBase(graphicsWrapper), isSSAOEnabled(isSSAOEnabled) {
        initializeProgram();
}

void CombinePostProcess::initializeProgram() {
        if(isSSAOEnabled) {
                program = graphicsWrapper->createGLSLProgram("./Engine/Shaders/CombineColorsWithSSAO/vertex.glsl",
                                          "./Engine/Shaders/CombineColorsWithSSAO/fragment.glsl", false);
        } else {
                program = graphicsWrapper->createGLSLProgram("./Engine/Shaders/CombineColors/vertex.glsl",
                                          "./Engine/Shaders/CombineColors/fragment.glsl", false);
        }

}
