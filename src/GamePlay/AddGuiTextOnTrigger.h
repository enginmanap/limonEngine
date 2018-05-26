//
// Created by engin on 26.05.2018.
//

#ifndef LIMONENGINE_ADDGUITEXTONTRIGGER_H
#define LIMONENGINE_ADDGUITEXTONTRIGGER_H


#include "TriggerInterface.h"

class AddGuiTextOnTrigger: public TriggerInterface {
    static TriggerRegister<AddGuiTextOnTrigger> reg;
public:
    std::vector<LimonAPI::ParameterRequest> getParameters() override;

    bool run(std::vector<LimonAPI::ParameterRequest> parameters) override;

    std::string getName() override {
        return "AddGuiTextOnTrigger";
    }
};

#endif //LIMONENGINE_ADDGUITEXTONTRIGGER_H
