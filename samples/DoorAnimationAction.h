//
// Created by engin on 20.05.2018.
//

#ifndef LIMONENGINE_ANIMATONTRIGGER_H
#define LIMONENGINE_ANIMATONTRIGGER_H


#include "API/TriggerInterface.h"

class DoorAnimationAction : public TriggerInterface {
    long stateResetTime = 250;
    bool animationRun = false;
public:
    DoorAnimationAction(LimonAPI *limonAPI);

    std::vector<LimonTypes::GenericParameter> getParameters() override;

    bool run(std::vector<LimonTypes::GenericParameter> parameters) override;

    void resetAnimationRun(const std::vector<LimonTypes::GenericParameter>&);

    std::vector<LimonTypes::GenericParameter> getResults() override;

    std::string getName() const override {
        return "DoorAnimationAction";
    }
};

#endif //LIMONENGINE_ANIMATONTRIGGER_H
