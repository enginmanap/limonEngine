//
// Created by engin on 3.08.2018.
//

#ifndef LIMONENGINE_QUITGAMEONTRIGGER_H
#define LIMONENGINE_QUITGAMEONTRIGGER_H

#include "API/TriggerInterface.h"

class QuitGameOnTrigger : public TriggerInterface {
    static TriggerRegister<QuitGameOnTrigger> reg;
public:
    QuitGameOnTrigger(LimonAPI *limonAPI);

    std::vector<LimonTypes::GenericParameter> getParameters() override;

    std::vector<LimonTypes::GenericParameter> getResults() override;

    bool run(std::vector<LimonTypes::GenericParameter> parameters) override;

    std::string getName() const override {
        return "QuitGameOnTrigger";
    }
};





#endif //LIMONENGINE_QUITGAMEONTRIGGER_H
