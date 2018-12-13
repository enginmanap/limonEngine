//
// Created by engin on 12.12.2018.
//

#ifndef LIMONENGINE_COMBINEPOSTPROCESS_H
#define LIMONENGINE_COMBINEPOSTPROCESS_H

#include "QuadRenderBase.h"

class CombinePostProcess : public QuadRenderBase {
    bool isSSAOEnabled;
    void initializeProgram() override;

public:
    CombinePostProcess(GLHelper* glHelper, bool isSSAOEnabled);

    bool isIsSSAOEnabled() const {
        return isSSAOEnabled;
    }
};


#endif //LIMONENGINE_COMBINEPOSTPROCESS_H
