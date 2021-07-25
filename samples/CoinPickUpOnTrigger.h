//
// Created by engin on 6.06.2018.
//

#ifndef LIMONENGINE_COINPICKUPONTRIGGER_H
#define LIMONENGINE_COINPICKUPONTRIGGER_H

#include "API/TriggerInterface.h"

class CoinPickUpOnTrigger : public TriggerInterface {
public:
    CoinPickUpOnTrigger(LimonAPI *limonAPI);

    std::vector<LimonTypes::GenericParameter> getParameters() override;

    bool run(std::vector<LimonTypes::GenericParameter> parameters) override;

    std::vector<LimonTypes::GenericParameter> getResults() override;

    std::string getName() const override {
        return "CoinPickUpOnTrigger";
    }

};

extern "C" void registerAsTrigger(std::map<std::string, TriggerInterface*(*)(LimonAPI*)>* triggerMap);


#endif //LIMONENGINE_COINPICKUPONTRIGGER_H
