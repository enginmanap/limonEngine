//
// Created by engin on 6.06.2018.
//

#ifndef LIMONENGINE_COINPICKUPONTRIGGER_H
#define LIMONENGINE_COINPICKUPONTRIGGER_H

#include "limonAPI/TriggerInterface.h"

class CoinPickUpOnTrigger : public TriggerInterface {
public:
    static std::vector<std::string> getPackFileAccessList() {
        return {"./Data/Fonts/Helvetica-Normal.ttf",
                "./Data/Sounds/coinPickup.wav"};
    }

    CoinPickUpOnTrigger(LimonAPI *limonAPI);

    bool run(std::vector<LimonTypes::GenericParameter> parameters) override;

    std::vector<LimonTypes::GenericParameter> getResults() override;

    std::string getName() const override {
        return "CoinPickUpOnTrigger";
    }

};

extern "C" void registerTriggerTypes(std::map<std::string, TriggerInterface::Registration>* triggerMap);


#endif //LIMONENGINE_COINPICKUPONTRIGGER_H
