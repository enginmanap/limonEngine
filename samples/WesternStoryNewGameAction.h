//
// Created by engin on 25.01.2019.
//

#ifndef LIMONENGINE_WESTERNSTORYNEWGAMEACTION_H
#define LIMONENGINE_WESTERNSTORYNEWGAMEACTION_H


#include <API/TriggerInterface.h>

class WesternStoryNewGameAction : public TriggerInterface {
public:
    WesternStoryNewGameAction(LimonAPI* limonAPI) : TriggerInterface(limonAPI) {}
    void animateThoughts(const std::vector<LimonTypes::GenericParameter> &emptyParamList);
    void switchWorld(const std::vector<LimonTypes::GenericParameter> &emptyParamList);

    std::vector<LimonTypes::GenericParameter> getParameters() override;

    bool run(std::vector<LimonTypes::GenericParameter> parameters) override;

    std::vector<LimonTypes::GenericParameter> getResults() override;

    std::string getName() const override;
};


#endif //LIMONENGINE_WESTERNSTORYNEWGAMEACTION_H
