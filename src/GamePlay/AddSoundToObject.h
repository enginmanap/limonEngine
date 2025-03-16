//
// Created by engin on 21.07.2018.
//

#ifndef LIMONENGINE_ADDSOUNDTOOBJECT_H
#define LIMONENGINE_ADDSOUNDTOOBJECT_H


#include "limonAPI/TriggerInterface.h"

class AddSoundToObject : public TriggerInterface {
    static TriggerRegister<AddSoundToObject> reg;
public:
    AddSoundToObject(LimonAPI *limonAPI);

    std::vector<LimonTypes::GenericParameter> getParameters() override;

    bool run(std::vector<LimonTypes::GenericParameter> parameters) override;


    std::vector<LimonTypes::GenericParameter> getResults() override;

    std::string getName() const override {
        return "AddSoundToObject";
    }
};

#endif //LIMONENGINE_ADDSOUNDTOOBJECT_H
