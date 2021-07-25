//
// Created by engin on 26.05.2018.
//

#ifndef LIMONENGINE_REMOVEGUITEXTONTRIGGER_H
#define LIMONENGINE_REMOVEGUITEXTONTRIGGER_H


#include "API/TriggerInterface.h"

class AddGuiTextOnTrigger: public TriggerInterface {
    static TriggerRegister<AddGuiTextOnTrigger> reg;
    std::vector<LimonTypes::GenericParameter> result;
public:
    AddGuiTextOnTrigger(LimonAPI *limonAPI);

    std::vector<LimonTypes::GenericParameter> getParameters() override;

    bool run(std::vector<LimonTypes::GenericParameter> parameters) override;


    std::vector<LimonTypes::GenericParameter> getResults() override;

    std::string getName() const override {
        return "AddGuiTextOnTrigger";
    }
};

#endif //LIMONENGINE_REMOVEGUITEXTONTRIGGER_H
