//
// Created by engin on 12.12.2018.
//

#include "CombinePostProcess.h"
#include "../GLSLProgram.h"

CombinePostProcess::CombinePostProcess(GLHelper* glHelper) : QuadRenderBase(glHelper) {
        initializeProgram();
}

void CombinePostProcess::initializeProgram() {
        program = new GLSLProgram(glHelper, "./Engine/Shaders/CombineAll/vertex.glsl",
                                  "./Engine/Shaders/CombineAll/fragment.glsl", false);
}
