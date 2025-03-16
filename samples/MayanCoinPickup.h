//
// Created by engin on 21.06.2018.
//

#ifndef LIMONENGINE_MAYANCOINPICKUP_H
#define LIMONENGINE_MAYANCOINPICKUP_H


#include "limonAPI/TriggerInterface.h"


class MayanCoinPickup : public TriggerInterface {
    uint32_t addedTextId;

public:
    MayanCoinPickup(LimonAPI *limonAPI);

    std::vector<LimonTypes::GenericParameter> getParameters() override;

    bool run(std::vector<LimonTypes::GenericParameter> parameters) override;

    std::vector<LimonTypes::GenericParameter> getResults() override;

    std::string getName() const override {
        return "MayanCoinPickup";
    }

};





#endif //LIMONENGINE_MAYANCOINPICKUP_H
