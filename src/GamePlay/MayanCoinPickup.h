//
// Created by engin on 21.06.2018.
//

#ifndef LIMONENGINE_MAYANCOINPICKUP_H
#define LIMONENGINE_MAYANCOINPICKUP_H


#include "TriggerInterface.h"


class MayanCoinPickup : public TriggerInterface {
    uint32_t addedTextId;

public:
    MayanCoinPickup(LimonAPI *limonAPI);

    std::vector<LimonAPI::ParameterRequest> getParameters() override;

    bool run(std::vector<LimonAPI::ParameterRequest> parameters) override;

    std::vector<LimonAPI::ParameterRequest> getResults() override;

    std::string getName() const override {
        return "MayanCoinPickup";
    }

};





#endif //LIMONENGINE_MAYANCOINPICKUP_H
