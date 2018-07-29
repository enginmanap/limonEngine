//
// Created by engin on 21.06.2018.
//

#ifndef LIMONENGINE_MAYANLEVER_H
#define LIMONENGINE_MAYANLEVER_H

#include "TriggerInterface.h"


class MayanLever : public TriggerInterface {

public:
    MayanLever(LimonAPI *limonAPI);

    std::vector<LimonAPI::ParameterRequest> getParameters() override;

    bool run(std::vector<LimonAPI::ParameterRequest> parameters) override;

    std::vector<LimonAPI::ParameterRequest> getResults() override;

    std::string getName() const override {
        return "MayanLever";
    }

};



#endif //LIMONENGINE_MAYANLEVER_H
