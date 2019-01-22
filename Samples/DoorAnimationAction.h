//
// Created by engin on 20.05.2018.
//

#ifndef LIMONENGINE_ANIMATONTRIGGER_H
#define LIMONENGINE_ANIMATONTRIGGER_H


#include "API/TriggerInterface.h"

class DoorAnimationAction : public TriggerInterface {
    long stateResetTime = 250;
    bool animationRun = false;
    static TriggerRegister<DoorAnimationAction> reg;
public:
    DoorAnimationAction(LimonAPI *limonAPI);

    std::vector<LimonAPI::ParameterRequest> getParameters() override;

    bool run(std::vector<LimonAPI::ParameterRequest> parameters) override;

    void resetAnimationRun(const std::vector<LimonAPI::ParameterRequest>&);

    std::vector<LimonAPI::ParameterRequest> getResults() override;

    std::string getName() const override {
        return "DoorAnimationAction";
    }
};

#endif //LIMONENGINE_ANIMATONTRIGGER_H
