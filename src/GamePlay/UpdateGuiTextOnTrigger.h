//
// Created by engin on 31.05.2018.
//

#ifndef LIMONENGINE_UPDATEGUITEXTONTRIGGER_H
#define LIMONENGINE_UPDATEGUITEXTONTRIGGER_H


#include "API/TriggerInterface.h"

class UpdateGuiTextOnTrigger: public TriggerInterface {
    static TriggerRegister<UpdateGuiTextOnTrigger> reg;
public:
    UpdateGuiTextOnTrigger(LimonAPI *limonAPI);

    std::vector<LimonTypes::GenericParameter> getParameters() override;

    std::vector<LimonTypes::GenericParameter> getResults() override;

    bool run(std::vector<LimonTypes::GenericParameter> parameters) override;

    std::string getName() const override {
        return "UpdateGuiTextOnTrigger";
    }

};

#endif //LIMONENGINE_UPDATEGUITEXTONTRIGGER_H
