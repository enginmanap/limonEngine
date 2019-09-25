//
// Created by engin on 13.12.2018.
//

#ifndef LIMONENGINE_SSAOBLURPOSTPROCESS_H
#define LIMONENGINE_SSAOBLURPOSTPROCESS_H


#include "QuadRenderBase.h"

class SSAOBlurPostProcess : public QuadRenderBase {
    void initializeProgram() override;
public:
    SSAOBlurPostProcess(GraphicsInterface* glHelper);
};


#endif //LIMONENGINE_SSAOBLURPOSTPROCESS_H
