//
// Created by engin on 12.12.2018.
//

#ifndef LIMONENGINE_SSAOPOSTPROCESS_H
#define LIMONENGINE_SSAOPOSTPROCESS_H


#include "QuadRenderBase.h"

class SSAOPostProcess : public QuadRenderBase {
    void generateAndSetSSAOKernels(uint32_t kernelSize) const ;
    void initializeProgram() override;

public:
    SSAOPostProcess(GLHelper* glHelper) : QuadRenderBase(glHelper) {
        initializeProgram();
    }
};


#endif //LIMONENGINE_SSAOPOSTPROCESS_H
