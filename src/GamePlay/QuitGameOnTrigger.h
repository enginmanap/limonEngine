//
// Created by engin on 3.08.2018.
//

#ifndef LIMONENGINE_QUITGAMEONTRIGGER_H
#define LIMONENGINE_QUITGAMEONTRIGGER_H

#include "TriggerInterface.h"

class QuitGameOnTrigger : public TriggerInterface {
    static TriggerRegister<QuitGameOnTrigger> reg;
public:
    QuitGameOnTrigger(LimonAPI *limonAPI);

    std::vector<LimonAPI::ParameterRequest> getParameters() override;

    std::vector<LimonAPI::ParameterRequest> getResults() override;

    bool run(std::vector<LimonAPI::ParameterRequest> parameters) override;

    std::string getName() const override {
        return "QuitGameOnTrigger";
    }
};





#endif //LIMONENGINE_QUITGAMEONTRIGGER_H
