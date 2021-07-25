//
// Created by engin on 28.05.2018.
//

#ifndef LIMONENGINE_REMOVEGUITEXTONTRIGGER_H
#define LIMONENGINE_REMOVEGUITEXTONTRIGGER_H


#include "API/TriggerInterface.h"

class RemoveGuiTextOnTrigger: public TriggerInterface {
    static TriggerRegister<RemoveGuiTextOnTrigger> reg;
public:
    RemoveGuiTextOnTrigger(LimonAPI *limonAPI);

    std::vector<LimonTypes::GenericParameter> getParameters() override;

    std::vector<LimonTypes::GenericParameter> getResults() override;

    bool run(std::vector<LimonTypes::GenericParameter> parameters) override;

    std::string getName() const override {
        return "RemoveGuiTextOnTrigger";
    }


};

#endif //LIMONENGINE_REMOVEGUITEXTONTRIGGER_H
