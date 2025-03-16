//
// Created by engin on 25.01.2019.
//

#ifndef LIMONENGINE_WESTERNSTORYTRAINACTION_H
#define LIMONENGINE_WESTERNSTORYTRAINACTION_H


#include <limonAPI/TriggerInterface.h>

class WesternStoryAtTrainAction : public TriggerInterface {
public:
    WesternStoryAtTrainAction(LimonAPI* limonAPI) : TriggerInterface(limonAPI) {}
    std::vector<LimonTypes::GenericParameter> getParameters() override;

    bool run(std::vector<LimonTypes::GenericParameter> parameters) override;

    std::vector<LimonTypes::GenericParameter> getResults() override;

    std::string getName() const override;
};


#endif //LIMONENGINE_WESTERNSTORYTRAINACTION_H
