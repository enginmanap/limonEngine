//
// Created by engin on 31.05.2018.
//

#ifndef LIMONENGINE_CHANGEWORLDONTRIGGER_H
#define LIMONENGINE_CHANGEWORLDONTRIGGER_H


#include "TriggerInterface.h"

class ChangeWorldOnTrigger: public TriggerInterface {
    static TriggerRegister<ChangeWorldOnTrigger> reg;
    bool worldChanged = false;
public:
    ChangeWorldOnTrigger(LimonAPI *limonAPI);

    std::vector<LimonAPI::ParameterRequest> getParameters() override;

    std::vector<LimonAPI::ParameterRequest> getResults() override;

    bool run(std::vector<LimonAPI::ParameterRequest> parameters) override;

    std::string getName() const override {
        return "ChangeWorldOnTrigger";
    }

};

#endif //LIMONENGINE_CHANGEWORLDONTRIGGER_H
