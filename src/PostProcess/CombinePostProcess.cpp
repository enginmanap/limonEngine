//
// Created by engin on 12.12.2018.
//

#include "CombinePostProcess.h"
#include "../GLSLProgram.h"

CombinePostProcess::CombinePostProcess(GLHelper* glHelper, bool isSSAOEnabled) : QuadRenderBase(glHelper), isSSAOEnabled(isSSAOEnabled) {
        initializeProgram();
}

void CombinePostProcess::initializeProgram() {
        if(isSSAOEnabled) {
                program = glHelper->createGLSLProgram("./Engine/Shaders/CombineAll/vertex.glsl",
                                          "./Engine/Shaders/CombineAll/fragmentWithSSAO.glsl", false);
        } else {
                program = glHelper->createGLSLProgram("./Engine/Shaders/CombineAll/vertex.glsl",
                                          "./Engine/Shaders/CombineAll/fragment.glsl", false);
        }

}
