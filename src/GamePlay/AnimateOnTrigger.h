//
// Created by engin on 20.05.2018.
//

#ifndef LIMONENGINE_ANIMATONTRIGGER_H
#define LIMONENGINE_ANIMATONTRIGGER_H


#include "TriggerInterface.h"

class AnimateOnTrigger : public TriggerInterface {
    static TriggerRegister<AnimateOnTrigger> reg;
public:
    std::vector<LimonAPI::ParameterRequest> getParameters() override;

    bool run(std::vector<LimonAPI::ParameterRequest> parameters) override;


    std::vector<LimonAPI::ParameterRequest> getResults() override;

    std::string getName() override {
        return "AnimateOnTrigger";
    }
};




#endif //LIMONENGINE_ANIMATONTRIGGER_H
