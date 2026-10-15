//
// Created by engin on 21.06.2018.
//

#ifndef LIMONENGINE_MAYANCOINPICKUP_H
#define LIMONENGINE_MAYANCOINPICKUP_H


#include "limonAPI/TriggerInterface.h"


class MayanCoinPickup : public TriggerInterface {
    uint32_t addedTextId;

public:
    static std::vector<std::string> getPackFileAccessList() {
        return {"./Data/Sounds/coinPickup.wav",
                "./Data/Sounds/rumble.wav",
                "./Data/Fonts/Helvetica-Normal.ttf"};
    }

    MayanCoinPickup(LimonAPI *limonAPI);

    bool run(std::vector<LimonTypes::GenericParameter> parameters) override;

    std::vector<LimonTypes::GenericParameter> getResults() override;

    std::string getName() const override {
        return "MayanCoinPickup";
    }

};





#endif //LIMONENGINE_MAYANCOINPICKUP_H
