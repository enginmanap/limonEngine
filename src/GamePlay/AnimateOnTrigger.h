//
// Created by engin on 20.05.2018.
//

#ifndef LIMONENGINE_ANIMATONTRIGGER_H
#define LIMONENGINE_ANIMATONTRIGGER_H


#include "TriggerInterface.h"

class AnimateOnTrigger : public TriggerInterface {
public:
    std::vector<LimonAPI::ParameterRequest> getParameters() override;

    bool run(std::vector<LimonAPI::ParameterRequest> parameters) override;
};




#endif //LIMONENGINE_ANIMATONTRIGGER_H
