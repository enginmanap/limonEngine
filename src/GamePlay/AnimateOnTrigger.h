//
// Created by engin on 20.05.2018.
//

#ifndef LIMONENGINE_ANIMATONTRIGGER_H
#define LIMONENGINE_ANIMATONTRIGGER_H


#include "API/TriggerInterface.h"

class AnimateOnTrigger : public TriggerInterface {
    static TriggerRegister<AnimateOnTrigger> reg;
public:
    AnimateOnTrigger(LimonAPI *limonAPI);

    std::vector<LimonTypes::GenericParameter> getParameters() override;

    bool run(std::vector<LimonTypes::GenericParameter> parameters) override;


    std::vector<LimonTypes::GenericParameter> getResults() override;

    std::string getName() const override {
        return "AnimateOnTrigger";
    }
};

#endif //LIMONENGINE_ANIMATONTRIGGER_H
