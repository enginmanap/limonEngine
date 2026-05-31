//
// Created by engin on 25.01.2019.
//

#ifndef LIMONENGINE_WESTERNSTORYATGRAVEACTION_H
#define LIMONENGINE_WESTERNSTORYATGRAVEACTION_H


#include <limonAPI/TriggerInterface.h>

class WesternStoryAtGraveAction : public TriggerInterface {
    uint32_t textID1,textID2;
    bool hasRun = false;
public:
    WesternStoryAtGraveAction(LimonAPI* limonAPI) : TriggerInterface(limonAPI) {
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
    }
    void showMessages1(const std::vector<LimonTypes::GenericParameter> &emptyParamList);
    void showMessages2(const std::vector<LimonTypes::GenericParameter> &emptyParamList);

    bool run(std::vector<LimonTypes::GenericParameter> parameters) override;

    std::vector<LimonTypes::GenericParameter> getResults() override;

    std::string getName() const override;
};


#endif //LIMONENGINE_WESTERNSTORYATGRAVEACTION_H
