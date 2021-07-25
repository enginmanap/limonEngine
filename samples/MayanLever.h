//
// Created by engin on 21.06.2018.
//

#ifndef LIMONENGINE_MAYANLEVER_H
#define LIMONENGINE_MAYANLEVER_H

#include "API/TriggerInterface.h"


class MayanLever : public TriggerInterface {

public:
    MayanLever(LimonAPI *limonAPI);

    std::vector<LimonTypes::GenericParameter> getParameters() override;

    bool run(std::vector<LimonTypes::GenericParameter> parameters) override;

    std::vector<LimonTypes::GenericParameter> getResults() override;

    std::string getName() const override {
        return "MayanLever";
    }

};



#endif //LIMONENGINE_MAYANLEVER_H
