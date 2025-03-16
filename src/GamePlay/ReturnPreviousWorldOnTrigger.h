//
// Created by engin on 6.08.2018.
//

#ifndef LIMONENGINE_RETURNPREVIOUSWORLDONTRIGGER_H
#define LIMONENGINE_RETURNPREVIOUSWORLDONTRIGGER_H


#include "limonAPI/TriggerInterface.h"

class ReturnPreviousWorldOnTrigger : public TriggerInterface {
    static TriggerRegister<ReturnPreviousWorldOnTrigger> reg;
public:
    ReturnPreviousWorldOnTrigger(LimonAPI *limonAPI);

    std::vector<LimonTypes::GenericParameter> getParameters() override;

    std::vector<LimonTypes::GenericParameter> getResults() override;

    bool run(std::vector<LimonTypes::GenericParameter> parameters) override;

    std::string getName() const override {
        return "ReturnPreviousWorldOnTrigger";
    }
};


#endif //LIMONENGINE_RETURNPREVIOUSWORLDONTRIGGER_H
