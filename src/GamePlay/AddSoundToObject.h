//
// Created by engin on 21.07.2018.
//

#ifndef LIMONENGINE_ADDSOUNDTOOBJECT_H
#define LIMONENGINE_ADDSOUNDTOOBJECT_H


#include "TriggerInterface.h"

class AddSoundToObject : public TriggerInterface {
    static TriggerRegister<AddSoundToObject> reg;
public:
    AddSoundToObject(LimonAPI *limonAPI);

    std::vector<LimonAPI::ParameterRequest> getParameters() override;

    bool run(std::vector<LimonAPI::ParameterRequest> parameters) override;


    std::vector<LimonAPI::ParameterRequest> getResults() override;

    std::string getName() const override {
        return "AddSoundToObject";
    }
};

#endif //LIMONENGINE_ADDSOUNDTOOBJECT_H
