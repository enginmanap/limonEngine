//
// Created by engin on 25.01.2019.
//

#ifndef LIMONENGINE_WESTERNSTORYNEWGAMEACTION_H
#define LIMONENGINE_WESTERNSTORYNEWGAMEACTION_H


#include <API/TriggerInterface.h>

class WesternStoryNewGameAction : public TriggerInterface {
public:
    WesternStoryNewGameAction(LimonAPI* limonAPI) : TriggerInterface(limonAPI) {}
    void animateThoughts(const std::vector<LimonAPI::ParameterRequest> &emptyParamList);
    void switchWorld(const std::vector<LimonAPI::ParameterRequest> &emptyParamList);

    std::vector<LimonAPI::ParameterRequest> getParameters() override;

    bool run(std::vector<LimonAPI::ParameterRequest> parameters) override;

    std::vector<LimonAPI::ParameterRequest> getResults() override;

    std::string getName() const override;
};


#endif //LIMONENGINE_WESTERNSTORYNEWGAMEACTION_H
