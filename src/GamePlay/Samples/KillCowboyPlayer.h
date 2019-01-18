//
// Created by engin on 18.01.2019.
//

#ifndef LIMONENGINE_KILLPLAYER_H
#define LIMONENGINE_KILLPLAYER_H


#include <API/TriggerInterface.h>

class KillCowboyPlayer : public TriggerInterface {
public:
    KillCowboyPlayer(LimonAPI *limonAPI) : TriggerInterface(limonAPI) {}
    std::vector<LimonAPI::ParameterRequest> getParameters() override;

    bool run(std::vector<LimonAPI::ParameterRequest> parameters) override;

    std::vector<LimonAPI::ParameterRequest> getResults() override;

    std::string getName() const override;
};


#endif //LIMONENGINE_KILLPLAYER_H
