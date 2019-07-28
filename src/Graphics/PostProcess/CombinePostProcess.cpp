//
// Created by engin on 12.12.2018.
//

#include "CombinePostProcess.h"
#include "Graphics/GLSLProgram.h"

CombinePostProcess::CombinePostProcess(GLHelper* glHelper, bool isSSAOEnabled) : QuadRenderBase(glHelper), isSSAOEnabled(isSSAOEnabled) {
        initializeProgram();
}

void CombinePostProcess::initializeProgram() {
        if(isSSAOEnabled) {
                program = glHelper->createGLSLProgram("./Engine/Shaders/CombineColorsWithSSAO/vertex.glsl",
                                          "./Engine/Shaders/CombineColorsWithSSAO/fragment.glsl", false);
        } else {
                program = glHelper->createGLSLProgram("./Engine/Shaders/CombineColors/vertex.glsl",
                                          "./Engine/Shaders/CombineColors/fragment.glsl", false);
        }

}
