//
// Created by engin on 25.01.2019.
//

#ifndef LIMONENGINE_WESTERNSTORYTRAINACTION_H
#define LIMONENGINE_WESTERNSTORYTRAINACTION_H


#include <limonAPI/TriggerInterface.h>

class WesternStoryAtTrainAction : public TriggerInterface {
public:
    WesternStoryAtTrainAction(LimonAPI* limonAPI) : TriggerInterface(limonAPI) {
        LimonTypes::GenericParameter param;
        param.requestType = LimonTypes::GenericParameter::RequestParameterTypes::TRIGGER;
        param.description = "Check Trigger before";
        param.isSet = false;
        this->parameters.push_back(param);

        LimonTypes::GenericParameter param2;
        param2.requestType = LimonTypes::GenericParameter::RequestParameterTypes::SWITCH;
        param2.description = "Should Trigger be run?";
        param2.isSet = true;
        this->parameters.push_back(param2);

        LimonTypes::GenericParameter param3;
        param3.requestType = LimonTypes::GenericParameter::RequestParameterTypes::FREE_TEXT;
        param3.description = "World to load";
        param3.isSet = false;
        this->parameters.push_back(param3);
    }

    bool run(std::vector<LimonTypes::GenericParameter> parameters) override;

    std::vector<LimonTypes::GenericParameter> getResults() override;

    std::string getName() const override;
};


#endif //LIMONENGINE_WESTERNSTORYTRAINACTION_H
