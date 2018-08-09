//
// Created by engin on 31.05.2018.
//

#ifndef LIMONENGINE_UPDATEGUITEXTONTRIGGER_H
#define LIMONENGINE_UPDATEGUITEXTONTRIGGER_H


#include "TriggerInterface.h"

class UpdateGuiTextOnTrigger: public TriggerInterface {
public:
    UpdateGuiTextOnTrigger(LimonAPI *limonAPI);

    std::vector<LimonAPI::ParameterRequest> getParameters() override;

    std::vector<LimonAPI::ParameterRequest> getResults() override;

    bool run(std::vector<LimonAPI::ParameterRequest> parameters) override;

    std::string getName() const override {
        return "UpdateGuiTextOnTrigger";
    }

};

#endif //LIMONENGINE_UPDATEGUITEXTONTRIGGER_H
