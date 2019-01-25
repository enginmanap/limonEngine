//
// Created by engin on 25.01.2019.
//

#ifndef LIMONENGINE_WESTERNSTORYTRAINACTION_H
#define LIMONENGINE_WESTERNSTORYTRAINACTION_H


#include <API/TriggerInterface.h>

class WesternStoryAtTrainAction : public TriggerInterface {
public:
    WesternStoryAtTrainAction(LimonAPI* limonAPI) : TriggerInterface(limonAPI) {}
    std::vector<LimonAPI::ParameterRequest> getParameters() override;

    bool run(std::vector<LimonAPI::ParameterRequest> parameters) override;

    std::vector<LimonAPI::ParameterRequest> getResults() override;

    std::string getName() const override;
};


#endif //LIMONENGINE_WESTERNSTORYTRAINACTION_H
