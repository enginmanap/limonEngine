//
// Created by engin on 31.05.2018.
//

#ifndef LIMONENGINE_CHANGEWORLDONTRIGGER_H
#define LIMONENGINE_CHANGEWORLDONTRIGGER_H


#include "limonAPI/TriggerInterface.h"

class ChangeWorldOnTrigger: public TriggerInterface {
    static TriggerRegister<ChangeWorldOnTrigger> reg;
    bool worldChanged = false;
public:
    ChangeWorldOnTrigger(LimonAPI *limonAPI);

    std::vector<LimonTypes::GenericParameter> getParameters() override;

    std::vector<LimonTypes::GenericParameter> getResults() override;

    bool run(std::vector<LimonTypes::GenericParameter> parameters) override;

    std::string getName() const override {
        return "ChangeWorldOnTrigger";
    }

};

#endif //LIMONENGINE_CHANGEWORLDONTRIGGER_H
